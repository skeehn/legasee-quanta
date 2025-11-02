#ifndef SYSMON_H
#define SYSMON_H

#include <stdint.h>
#include <stdbool.h>
#include "error.h"

/**
 * System Monitor (Week 3)
 *
 * Real-time system metrics visualization:
 * - CPU usage per core
 * - Memory usage (total, used, cached)
 * - Network traffic (RX/TX bytes)
 * - Process monitoring
 *
 * Data Sources:
 * - /proc/stat - CPU statistics
 * - /proc/meminfo - Memory information
 * - /proc/net/dev - Network device statistics
 * - /proc/[pid]/stat - Process information
 */

#define SYSMON_MAX_CPUS 128
#define SYSMON_MAX_PROCESSES 256
#define SYSMON_MAX_NET_DEVICES 16
#define SYSMON_NAME_LEN 64

/* CPU metrics */
typedef struct {
    int cpu_id;              /* CPU core ID (-1 for aggregate) */
    uint64_t user;           /* User mode time */
    uint64_t nice;           /* User mode with low priority */
    uint64_t system;         /* System mode time */
    uint64_t idle;           /* Idle time */
    uint64_t iowait;         /* IO wait time */
    uint64_t irq;            /* IRQ servicing time */
    uint64_t softirq;        /* Soft IRQ time */
    uint64_t total;          /* Total time */
    float usage_percent;     /* CPU usage percentage (0-100) */
} CPUStats;

/* Memory metrics */
typedef struct {
    uint64_t total_kb;       /* Total RAM in KB */
    uint64_t free_kb;        /* Free RAM in KB */
    uint64_t available_kb;   /* Available RAM in KB */
    uint64_t buffers_kb;     /* Buffer cache in KB */
    uint64_t cached_kb;      /* Page cache in KB */
    uint64_t swap_total_kb;  /* Total swap in KB */
    uint64_t swap_free_kb;   /* Free swap in KB */
    float usage_percent;     /* Memory usage percentage */
    float swap_percent;      /* Swap usage percentage */
} MemoryStats;

/* Network device metrics */
typedef struct {
    char name[SYSMON_NAME_LEN];  /* Device name (eth0, wlan0, etc) */
    uint64_t rx_bytes;           /* Received bytes */
    uint64_t rx_packets;         /* Received packets */
    uint64_t rx_errors;          /* Receive errors */
    uint64_t tx_bytes;           /* Transmitted bytes */
    uint64_t tx_packets;         /* Transmitted packets */
    uint64_t tx_errors;          /* Transmit errors */
    float rx_mbps;               /* RX speed in Mbps */
    float tx_mbps;               /* TX speed in Mbps */
} NetworkStats;

/* Process metrics */
typedef struct {
    int pid;                 /* Process ID */
    char name[SYSMON_NAME_LEN]; /* Process name */
    char state;              /* Process state (R, S, D, Z, T) */
    uint64_t utime;          /* User CPU time */
    uint64_t stime;          /* System CPU time */
    uint64_t vsize_kb;       /* Virtual memory size in KB */
    uint64_t rss_kb;         /* Resident set size in KB */
    float cpu_percent;       /* CPU usage percentage */
    float mem_percent;       /* Memory usage percentage */
} ProcessStats;

/* System monitor state */
typedef struct {
    /* CPU tracking */
    CPUStats cpu_total;
    CPUStats cpus[SYSMON_MAX_CPUS];
    int num_cpus;
    CPUStats prev_cpu_total;  /* Previous sample for delta calculation */
    CPUStats prev_cpus[SYSMON_MAX_CPUS];

    /* Memory tracking */
    MemoryStats memory;

    /* Network tracking */
    NetworkStats networks[SYSMON_MAX_NET_DEVICES];
    int num_networks;
    NetworkStats prev_networks[SYSMON_MAX_NET_DEVICES];
    uint64_t last_sample_time_ms;  /* For rate calculation */

    /* Process tracking */
    ProcessStats processes[SYSMON_MAX_PROCESSES];
    int num_processes;

    /* Update tracking */
    uint64_t sample_count;
    bool initialized;
} SystemMonitor;

/**
 * Create system monitor
 */
SystemMonitor* sysmon_create(void);

/**
 * Destroy system monitor
 */
void sysmon_destroy(SystemMonitor *mon);

/**
 * Update all system metrics (call periodically)
 *
 * @param mon System monitor
 * @return Error status
 */
Error sysmon_update(SystemMonitor *mon);

/**
 * Get CPU statistics
 */
const CPUStats* sysmon_get_cpu_total(const SystemMonitor *mon);
const CPUStats* sysmon_get_cpu(const SystemMonitor *mon, int cpu_id);
int sysmon_get_num_cpus(const SystemMonitor *mon);

/**
 * Get memory statistics
 */
const MemoryStats* sysmon_get_memory(const SystemMonitor *mon);

/**
 * Get network statistics
 */
const NetworkStats* sysmon_get_network(const SystemMonitor *mon, int index);
int sysmon_get_num_networks(const SystemMonitor *mon);

/**
 * Get process statistics
 */
const ProcessStats* sysmon_get_process(const SystemMonitor *mon, int index);
int sysmon_get_num_processes(const SystemMonitor *mon);

/**
 * Update specific metrics (for fine-grained control)
 */
Error sysmon_update_cpu(SystemMonitor *mon);
Error sysmon_update_memory(SystemMonitor *mon);
Error sysmon_update_network(SystemMonitor *mon);
Error sysmon_update_processes(SystemMonitor *mon, int max_processes);

/**
 * Utility functions
 */
const char* sysmon_process_state_name(char state);
void sysmon_print_summary(const SystemMonitor *mon);

#endif /* SYSMON_H */
