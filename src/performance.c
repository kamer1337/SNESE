/*
 * performance.c - Performance monitoring implementation
 */

#include <stdio.h>
#include <string.h>
#include "../include/performance.h"

/* Global performance stats */
PerfStats g_perf_stats = {0};

void perf_init(void) {
    memset(&g_perf_stats, 0, sizeof(g_perf_stats));
    g_perf_stats.enabled = true;
    g_perf_stats.counter_count = 0;
}

void perf_enable(bool enable) {
    g_perf_stats.enabled = enable;
}

int perf_register(const char *name) {
    if (!name || g_perf_stats.counter_count >= 32) {
        return -1;
    }
    
    /* Check if already registered */
    for (u8 i = 0; i < g_perf_stats.counter_count; i++) {
        if (strcmp(g_perf_stats.counters[i].name, name) == 0) {
            return i;
        }
    }
    
    /* Register new counter */
    int id = g_perf_stats.counter_count++;
    PerfCounter *counter = &g_perf_stats.counters[id];
    
    counter->name = name;
    counter->call_count = 0;
    counter->total_cycles = 0;
    counter->is_running = false;
    
    return id;
}

void perf_start(int counter_id) {
    if (!g_perf_stats.enabled || counter_id < 0 || counter_id >= g_perf_stats.counter_count) {
        return;
    }
    
    PerfCounter *counter = &g_perf_stats.counters[counter_id];
    if (!counter->is_running) {
        counter->start_time = clock();
        counter->is_running = true;
        counter->call_count++;
    }
}

void perf_stop(int counter_id) {
    clock_t end_time;
    
    if (!g_perf_stats.enabled || counter_id < 0 || counter_id >= g_perf_stats.counter_count) {
        return;
    }
    
    end_time = clock();
    
    PerfCounter *counter = &g_perf_stats.counters[counter_id];
    if (counter->is_running) {
        counter->total_cycles += (u64)(end_time - counter->start_time);
        counter->is_running = false;
    }
}

int perf_get_counter(const char *name) {
    if (!name) {
        return -1;
    }
    
    for (u8 i = 0; i < g_perf_stats.counter_count; i++) {
        if (strcmp(g_perf_stats.counters[i].name, name) == 0) {
            return i;
        }
    }
    
    return -1;
}

void perf_print_stats(void) {
    if (g_perf_stats.counter_count == 0) {
        printf("No performance data collected.\n");
        return;
    }
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║           Performance Statistics                     ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("%-30s %10s %15s %15s\n", "Counter", "Calls", "Total (us)", "Avg (us)");
    printf("%-30s %10s %15s %15s\n", "-------", "-----", "---------", "--------");
    
    for (u8 i = 0; i < g_perf_stats.counter_count; i++) {
        PerfCounter *counter = &g_perf_stats.counters[i];
        
        /* Convert clock ticks to microseconds */
        u64 total_us = (counter->total_cycles * 1000000) / CLOCKS_PER_SEC;
        u64 avg_us = counter->call_count > 0 ? total_us / counter->call_count : 0;
        
        printf("%-30s %10llu %15llu %15llu\n",
               counter->name,
               (unsigned long long)counter->call_count,
               (unsigned long long)total_us,
               (unsigned long long)avg_us);
    }
    
    printf("\n");
}

void perf_reset(void) {
    for (u8 i = 0; i < g_perf_stats.counter_count; i++) {
        PerfCounter *counter = &g_perf_stats.counters[i];
        counter->call_count = 0;
        counter->total_cycles = 0;
        counter->is_running = false;
    }
}
