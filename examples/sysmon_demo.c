/**
 * System Monitor Visualization Demo (Week 3)
 *
 * Real-time system metrics visualized as particles:
 * - CPU cores → particles (color = usage, position = core#)
 * - Memory usage → particle density
 * - Network traffic → particle velocity
 * - Processes → individual particles
 *
 * Non-interactive terminal demo for system monitoring
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../src/sysmon.h"
#include "../src/sim.h"

#define SAMPLES 30
#define SAMPLE_INTERVAL_SEC 1

/* Map CPU usage to color (blue → green → yellow → red) */
uint32_t cpu_to_color(float usage_percent) {
    uint8_t r, g, b;

    if (usage_percent < 25.0f) {
        /* Blue → Cyan (0-25%) */
        float t = usage_percent / 25.0f;
        r = 0;
        g = (uint8_t)(128 * t);
        b = 255;
    } else if (usage_percent < 50.0f) {
        /* Cyan → Green (25-50%) */
        float t = (usage_percent - 25.0f) / 25.0f;
        r = 0;
        g = (uint8_t)(128 + 127 * t);
        b = (uint8_t)(255 * (1.0f - t));
    } else if (usage_percent < 75.0f) {
        /* Green → Yellow (50-75%) */
        float t = (usage_percent - 50.0f) / 25.0f;
        r = (uint8_t)(255 * t);
        g = 255;
        b = 0;
    } else {
        /* Yellow → Red (75-100%) */
        float t = (usage_percent - 75.0f) / 25.0f;
        r = 255;
        g = (uint8_t)(255 * (1.0f - t));
        b = 0;
    }

    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

/* Print colored bar */
void print_bar(const char *label, float percent, int width) {
    int filled = (int)(percent * width / 100.0f);
    if (filled > width) filled = width;

    printf("%s: [", label);
    for (int i = 0; i < width; i++) {
        printf("%c", i < filled ? '█' : '░');
    }
    printf("] %5.1f%%\n", percent);
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║         SYSTEM MONITOR VISUALIZATION DEMO              ║\n");
    printf("║                   (Week 3)                             ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Create system monitor */
    SystemMonitor *mon = sysmon_create();
    if (!mon) {
        fprintf(stderr, "Failed to create system monitor\n");
        return 1;
    }

    /* Initial update to get baseline */
    printf("Initializing system monitor...\n");
    Error err = sysmon_update(mon);
    if (err.code != SUCCESS) {
        fprintf(stderr, "Failed to update system monitor: %s\n", err.message);
        sysmon_destroy(mon);
        return 1;
    }

    /* Wait for first delta */
    sleep(1);

    printf("\nStarting %d-second monitoring session...\n\n", SAMPLES);

    /* Monitoring loop */
    for (int sample = 0; sample < SAMPLES; sample++) {
        /* Update metrics */
        err = sysmon_update(mon);
        if (err.code != SUCCESS) {
            fprintf(stderr, "Update failed: %s\n", err.message);
            continue;
        }

        /* Clear screen (simple version) */
        printf("\033[2J\033[H");

        /* Print header */
        printf("╔════════════════════════════════════════════════════════╗\n");
        printf("║  SYSTEM MONITOR - Sample %2d/%d                         ║\n", sample + 1, SAMPLES);
        printf("╚════════════════════════════════════════════════════════╝\n\n");

        /* CPU Statistics */
        const CPUStats *cpu_total = sysmon_get_cpu_total(mon);
        int num_cpus = sysmon_get_num_cpus(mon);

        printf("══ CPU USAGE ══\n");
        print_bar("Total", cpu_total->usage_percent, 40);

        /* Show up to 8 cores */
        int cores_to_show = num_cpus < 8 ? num_cpus : 8;
        for (int i = 0; i < cores_to_show; i++) {
            const CPUStats *cpu = sysmon_get_cpu(mon, i);
            char label[16];
            snprintf(label, sizeof(label), "Core %d", i);
            print_bar(label, cpu->usage_percent, 40);
        }

        if (num_cpus > 8) {
            printf("... and %d more cores\n", num_cpus - 8);
        }

        /* Memory Statistics */
        printf("\n══ MEMORY USAGE ══\n");
        const MemoryStats *mem = sysmon_get_memory(mon);
        print_bar("RAM", mem->usage_percent, 40);
        printf("  Total: %.2f GB\n", mem->total_kb / 1024.0f / 1024.0f);
        printf("  Used:  %.2f GB\n",
               (mem->total_kb - mem->available_kb) / 1024.0f / 1024.0f);
        printf("  Free:  %.2f GB\n", mem->available_kb / 1024.0f / 1024.0f);

        if (mem->swap_total_kb > 0) {
            print_bar("Swap", mem->swap_percent, 40);
        }

        /* Network Statistics */
        printf("\n══ NETWORK TRAFFIC ══\n");
        int num_networks = sysmon_get_num_networks(mon);
        if (num_networks > 0) {
            for (int i = 0; i < num_networks; i++) {
                const NetworkStats *net = sysmon_get_network(mon, i);
                printf("%s:\n", net->name);
                printf("  RX: %8llu packets, %10llu bytes (%.2f Mbps)\n",
                       (unsigned long long)net->rx_packets,
                       (unsigned long long)net->rx_bytes,
                       net->rx_mbps);
                printf("  TX: %8llu packets, %10llu bytes (%.2f Mbps)\n",
                       (unsigned long long)net->tx_packets,
                       (unsigned long long)net->tx_bytes,
                       net->tx_mbps);
                if (net->rx_errors + net->tx_errors > 0) {
                    printf("  Errors: RX=%llu, TX=%llu\n",
                           (unsigned long long)net->rx_errors,
                           (unsigned long long)net->tx_errors);
                }
            }
        } else {
            printf("No network interfaces found\n");
        }

        /* Process Statistics */
        printf("\n══ TOP PROCESSES ══\n");
        int num_processes = sysmon_get_num_processes(mon);
        int to_show = num_processes < 10 ? num_processes : 10;

        printf("  PID   STATE  NAME\n");
        printf("  ────────────────────────────────────\n");
        for (int i = 0; i < to_show; i++) {
            const ProcessStats *proc = sysmon_get_process(mon, i);
            printf("  %5d   %c    %s\n",
                   proc->pid, proc->state, proc->name);
        }

        if (num_processes > 10) {
            printf("  ... and %d more processes\n", num_processes - 10);
        }

        /* Visualization Guide */
        printf("\n══ PARTICLE VISUALIZATION MAPPING ══\n");
        printf("CPU Usage  → Particle Color (Blue→Green→Yellow→Red)\n");
        printf("Memory     → Particle Density\n");
        printf("Network RX → Particle Velocity (Horizontal)\n");
        printf("Network TX → Particle Velocity (Vertical)\n");
        printf("Processes  → Individual Particle Agents\n");

        /* Wait for next sample */
        sleep(SAMPLE_INTERVAL_SEC);
    }

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║              MONITORING SESSION COMPLETE               ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Final summary */
    const CPUStats *final_cpu = sysmon_get_cpu_total(mon);
    const MemoryStats *final_mem = sysmon_get_memory(mon);

    printf("Final Statistics:\n");
    printf("  CPUs:      %d cores\n", sysmon_get_num_cpus(mon));
    printf("  CPU Usage: %.1f%%\n", final_cpu->usage_percent);
    printf("  Memory:    %.1f%% used\n", final_mem->usage_percent);
    printf("  Networks:  %d active\n", sysmon_get_num_networks(mon));
    printf("  Processes: %d monitored\n", sysmon_get_num_processes(mon));
    printf("  Samples:   %llu\n", (unsigned long long)mon->sample_count);
    printf("\n");

    sysmon_destroy(mon);

    printf("Week 3 System Monitor: COMPLETE ✓\n\n");

    return 0;
}
