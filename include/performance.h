/*
 * performance.h - Performance monitoring and optimization helpers
 * 
 * Provides tools for profiling and optimizing SNESE code
 */

#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include "types.h"
#include <time.h>

/* Performance counter */
typedef struct {
    const char *name;
    u64 call_count;
    u64 total_cycles;
    clock_t start_time;
    bool is_running;
} PerfCounter;

/* Performance statistics */
typedef struct {
    PerfCounter counters[32];
    u8 counter_count;
    bool enabled;
} PerfStats;

/* Global performance stats */
extern PerfStats g_perf_stats;

/* Function declarations */

/*
 * Initialize performance monitoring
 */
void perf_init(void);

/*
 * Enable/disable performance monitoring
 */
void perf_enable(bool enable);

/*
 * Register a new performance counter
 */
int perf_register(const char *name);

/*
 * Start timing for a counter
 */
void perf_start(int counter_id);

/*
 * Stop timing for a counter
 */
void perf_stop(int counter_id);

/*
 * Get counter by name
 */
int perf_get_counter(const char *name);

/*
 * Print performance statistics
 */
void perf_print_stats(void);

/*
 * Reset all counters
 */
void perf_reset(void);

/* Convenience macros */
#define PERF_SCOPE_START(name) \
    static int __perf_id_##name = -1; \
    if (__perf_id_##name == -1) { \
        __perf_id_##name = perf_register(#name); \
    } \
    perf_start(__perf_id_##name)

#define PERF_SCOPE_END(name) \
    perf_stop(__perf_id_##name)

#endif /* PERFORMANCE_H */
