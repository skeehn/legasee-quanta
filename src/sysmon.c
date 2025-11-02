#include "sysmon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <ctype.h>

/* Get current time in milliseconds */
static uint64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* Parse uint64 from string */
static uint64_t parse_uint64(const char *str) {
    return strtoull(str, NULL, 10);
}

/* Parse CPU line from /proc/stat */
static bool parse_cpu_line(const char *line, CPUStats *stats) {
    char cpu_name[16];
    int cpu_id = -1;

    int n = sscanf(line, "%s %llu %llu %llu %llu %llu %llu %llu",
                   cpu_name,
                   (unsigned long long*)&stats->user,
                   (unsigned long long*)&stats->nice,
                   (unsigned long long*)&stats->system,
                   (unsigned long long*)&stats->idle,
                   (unsigned long long*)&stats->iowait,
                   (unsigned long long*)&stats->irq,
                   (unsigned long long*)&stats->softirq);

    if (n < 5) return false;

    /* Parse CPU ID */
    if (strcmp(cpu_name, "cpu") == 0) {
        cpu_id = -1;  /* Total */
    } else if (strncmp(cpu_name, "cpu", 3) == 0) {
        cpu_id = atoi(cpu_name + 3);
    } else {
        return false;
    }

    stats->cpu_id = cpu_id;
    stats->total = stats->user + stats->nice + stats->system +
                   stats->idle + stats->iowait + stats->irq + stats->softirq;

    return true;
}

/* Calculate CPU usage percentage from delta */
static void calculate_cpu_usage(const CPUStats *prev, CPUStats *curr) {
    uint64_t total_delta = curr->total - prev->total;
    uint64_t idle_delta = curr->idle - prev->idle;

    if (total_delta > 0) {
        curr->usage_percent = 100.0f * (total_delta - idle_delta) / total_delta;
    } else {
        curr->usage_percent = 0.0f;
    }
}

/* Update CPU statistics */
Error sysmon_update_cpu(SystemMonitor *mon) {
    ERROR_CHECK_NULL(mon, "System monitor");

    FILE *f = fopen("/proc/stat", "r");
    if (!f) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to open /proc/stat");
    }

    char line[256];
    int cpu_index = 0;

    /* Save previous stats */
    mon->prev_cpu_total = mon->cpu_total;
    memcpy(mon->prev_cpus, mon->cpus, sizeof(mon->cpus));

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "cpu", 3) == 0) {
            CPUStats stats = {0};
            if (parse_cpu_line(line, &stats)) {
                if (stats.cpu_id == -1) {
                    /* Total CPU */
                    mon->cpu_total = stats;
                    if (mon->initialized) {
                        calculate_cpu_usage(&mon->prev_cpu_total, &mon->cpu_total);
                    }
                } else if (stats.cpu_id < SYSMON_MAX_CPUS) {
                    /* Individual CPU */
                    mon->cpus[stats.cpu_id] = stats;
                    if (mon->initialized) {
                        calculate_cpu_usage(&mon->prev_cpus[stats.cpu_id],
                                          &mon->cpus[stats.cpu_id]);
                    }
                    if (stats.cpu_id >= mon->num_cpus) {
                        mon->num_cpus = stats.cpu_id + 1;
                    }
                    cpu_index++;
                }
            }
        }
    }

    fclose(f);
    return (Error){SUCCESS};
}

/* Update memory statistics */
Error sysmon_update_memory(SystemMonitor *mon) {
    ERROR_CHECK_NULL(mon, "System monitor");

    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to open /proc/meminfo");
    }

    char line[256];
    char name[64];
    uint64_t value;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%s %llu", name, (unsigned long long*)&value) == 2) {
            if (strcmp(name, "MemTotal:") == 0) {
                mon->memory.total_kb = value;
            } else if (strcmp(name, "MemFree:") == 0) {
                mon->memory.free_kb = value;
            } else if (strcmp(name, "MemAvailable:") == 0) {
                mon->memory.available_kb = value;
            } else if (strcmp(name, "Buffers:") == 0) {
                mon->memory.buffers_kb = value;
            } else if (strcmp(name, "Cached:") == 0) {
                mon->memory.cached_kb = value;
            } else if (strcmp(name, "SwapTotal:") == 0) {
                mon->memory.swap_total_kb = value;
            } else if (strcmp(name, "SwapFree:") == 0) {
                mon->memory.swap_free_kb = value;
            }
        }
    }

    /* Calculate percentages */
    if (mon->memory.total_kb > 0) {
        uint64_t used = mon->memory.total_kb - mon->memory.available_kb;
        mon->memory.usage_percent = 100.0f * used / mon->memory.total_kb;
    }

    if (mon->memory.swap_total_kb > 0) {
        uint64_t used = mon->memory.swap_total_kb - mon->memory.swap_free_kb;
        mon->memory.swap_percent = 100.0f * used / mon->memory.swap_total_kb;
    }

    fclose(f);
    return (Error){SUCCESS};
}

/* Update network statistics */
Error sysmon_update_network(SystemMonitor *mon) {
    ERROR_CHECK_NULL(mon, "System monitor");

    FILE *f = fopen("/proc/net/dev", "r");
    if (!f) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to open /proc/net/dev");
    }

    /* Save previous stats */
    memcpy(mon->prev_networks, mon->networks, sizeof(mon->networks));
    uint64_t prev_time = mon->last_sample_time_ms;
    mon->last_sample_time_ms = get_time_ms();
    uint64_t time_delta_ms = mon->last_sample_time_ms - prev_time;

    char line[512];
    int index = 0;

    /* Skip header lines */
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f) && index < SYSMON_MAX_NET_DEVICES) {
        char *colon = strchr(line, ':');
        if (!colon) continue;

        *colon = '\0';
        char *name_start = line;
        while (*name_start && isspace(*name_start)) name_start++;

        /* Skip loopback */
        if (strcmp(name_start, "lo") == 0) continue;

        NetworkStats *stats = &mon->networks[index];
        strncpy(stats->name, name_start, SYSMON_NAME_LEN - 1);

        char *data = colon + 1;
        if (sscanf(data, "%llu %llu %llu %*u %*u %*u %*u %*u %llu %llu %llu",
                   (unsigned long long*)&stats->rx_bytes,
                   (unsigned long long*)&stats->rx_packets,
                   (unsigned long long*)&stats->rx_errors,
                   (unsigned long long*)&stats->tx_bytes,
                   (unsigned long long*)&stats->tx_packets,
                   (unsigned long long*)&stats->tx_errors) == 6) {

            /* Calculate rates if we have previous data */
            if (mon->initialized && time_delta_ms > 0) {
                for (int i = 0; i < mon->num_networks; i++) {
                    if (strcmp(mon->prev_networks[i].name, stats->name) == 0) {
                        uint64_t rx_delta = stats->rx_bytes - mon->prev_networks[i].rx_bytes;
                        uint64_t tx_delta = stats->tx_bytes - mon->prev_networks[i].tx_bytes;

                        /* Convert bytes/ms to Mbps */
                        stats->rx_mbps = (rx_delta * 8.0f / 1000000.0f) / (time_delta_ms / 1000.0f);
                        stats->tx_mbps = (tx_delta * 8.0f / 1000000.0f) / (time_delta_ms / 1000.0f);
                        break;
                    }
                }
            }

            index++;
        }
    }

    mon->num_networks = index;
    fclose(f);
    return (Error){SUCCESS};
}

/* Update process statistics */
Error sysmon_update_processes(SystemMonitor *mon, int max_processes) {
    ERROR_CHECK_NULL(mon, "System monitor");

    if (max_processes > SYSMON_MAX_PROCESSES) {
        max_processes = SYSMON_MAX_PROCESSES;
    }

    DIR *proc = opendir("/proc");
    if (!proc) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to open /proc");
    }

    struct dirent *entry;
    int index = 0;

    while ((entry = readdir(proc)) != NULL && index < max_processes) {
        /* Skip non-numeric entries */
        if (!isdigit(entry->d_name[0])) continue;

        int pid = atoi(entry->d_name);
        char stat_path[256];
        snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

        FILE *f = fopen(stat_path, "r");
        if (!f) continue;

        ProcessStats *stats = &mon->processes[index];
        stats->pid = pid;

        /* Parse /proc/[pid]/stat */
        char comm[256];
        if (fscanf(f, "%d %s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu",
                   &stats->pid, comm, &stats->state,
                   (unsigned long long*)&stats->utime,
                   (unsigned long long*)&stats->stime) == 5) {

            /* Remove parentheses from command name */
            int len = strlen(comm);
            if (len > 2 && comm[0] == '(' && comm[len-1] == ')') {
                comm[len-1] = '\0';
                strncpy(stats->name, comm + 1, SYSMON_NAME_LEN - 1);
            } else {
                strncpy(stats->name, comm, SYSMON_NAME_LEN - 1);
            }

            /* TODO: Calculate CPU and memory percentages */
            stats->cpu_percent = 0.0f;
            stats->mem_percent = 0.0f;

            index++;
        }

        fclose(f);
    }

    mon->num_processes = index;
    closedir(proc);
    return (Error){SUCCESS};
}

/* Update all metrics */
Error sysmon_update(SystemMonitor *mon) {
    ERROR_CHECK_NULL(mon, "System monitor");

    Error err;

    err = sysmon_update_cpu(mon);
    if (err.code != SUCCESS) return err;

    err = sysmon_update_memory(mon);
    if (err.code != SUCCESS) return err;

    err = sysmon_update_network(mon);
    if (err.code != SUCCESS) return err;

    err = sysmon_update_processes(mon, 20);
    if (err.code != SUCCESS) return err;

    mon->sample_count++;
    mon->initialized = true;

    return (Error){SUCCESS};
}

/* Create system monitor */
SystemMonitor* sysmon_create(void) {
    SystemMonitor *mon = calloc(1, sizeof(SystemMonitor));
    if (!mon) return NULL;

    mon->last_sample_time_ms = get_time_ms();
    return mon;
}

/* Destroy system monitor */
void sysmon_destroy(SystemMonitor *mon) {
    free(mon);
}

/* Getters */
const CPUStats* sysmon_get_cpu_total(const SystemMonitor *mon) {
    return mon ? &mon->cpu_total : NULL;
}

const CPUStats* sysmon_get_cpu(const SystemMonitor *mon, int cpu_id) {
    if (!mon || cpu_id < 0 || cpu_id >= mon->num_cpus) return NULL;
    return &mon->cpus[cpu_id];
}

int sysmon_get_num_cpus(const SystemMonitor *mon) {
    return mon ? mon->num_cpus : 0;
}

const MemoryStats* sysmon_get_memory(const SystemMonitor *mon) {
    return mon ? &mon->memory : NULL;
}

const NetworkStats* sysmon_get_network(const SystemMonitor *mon, int index) {
    if (!mon || index < 0 || index >= mon->num_networks) return NULL;
    return &mon->networks[index];
}

int sysmon_get_num_networks(const SystemMonitor *mon) {
    return mon ? mon->num_networks : 0;
}

const ProcessStats* sysmon_get_process(const SystemMonitor *mon, int index) {
    if (!mon || index < 0 || index >= mon->num_processes) return NULL;
    return &mon->processes[index];
}

int sysmon_get_num_processes(const SystemMonitor *mon) {
    return mon ? mon->num_processes : 0;
}

/* Utility functions */
const char* sysmon_process_state_name(char state) {
    switch (state) {
        case 'R': return "Running";
        case 'S': return "Sleeping";
        case 'D': return "Disk Sleep";
        case 'Z': return "Zombie";
        case 'T': return "Stopped";
        case 't': return "Tracing";
        case 'X': return "Dead";
        default: return "Unknown";
    }
}

void sysmon_print_summary(const SystemMonitor *mon) {
    if (!mon) return;

    printf("=== SYSTEM MONITOR ===\n");
    printf("Sample: %llu\n\n", (unsigned long long)mon->sample_count);

    /* CPU */
    printf("CPU Total: %.1f%%\n", mon->cpu_total.usage_percent);
    for (int i = 0; i < mon->num_cpus && i < 8; i++) {
        printf("  CPU%d: %.1f%%\n", i, mon->cpus[i].usage_percent);
    }
    printf("\n");

    /* Memory */
    printf("Memory: %.1f%% (%.1f / %.1f GB)\n",
           mon->memory.usage_percent,
           (mon->memory.total_kb - mon->memory.available_kb) / 1024.0f / 1024.0f,
           mon->memory.total_kb / 1024.0f / 1024.0f);
    printf("Swap: %.1f%%\n\n", mon->memory.swap_percent);

    /* Network */
    for (int i = 0; i < mon->num_networks; i++) {
        const NetworkStats *net = &mon->networks[i];
        printf("Network %s: RX %.2f Mbps, TX %.2f Mbps\n",
               net->name, net->rx_mbps, net->tx_mbps);
    }
    printf("\n");

    /* Top processes */
    printf("Top processes: %d\n", mon->num_processes);
    for (int i = 0; i < mon->num_processes && i < 5; i++) {
        const ProcessStats *proc = &mon->processes[i];
        printf("  [%d] %s (%c)\n", proc->pid, proc->name,
               proc->state);
    }
}
