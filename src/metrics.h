/**
 * metrics.h - Performance metrics collection and reporting
 * 
 * Tracks page faults, TLB hits/misses, swap I/O, and calculates derived metrics.
 * Supports multi-format output (console, CSV, JSON).
 */

#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Per-process metrics
typedef struct {
    uint32_t pid;
    uint64_t total_accesses;
    uint64_t reads;
    uint64_t writes;
    uint64_t page_faults;
    uint64_t tlb_hits;
    uint64_t tlb_misses;
} ProcessMetrics;

// Global metrics
typedef struct {
    // Access counts
    uint64_t total_accesses;
    uint64_t total_reads;
    uint64_t total_writes;
    
    // Page faults
    uint64_t page_faults;
    uint64_t major_faults; // Required disk I/O
    uint64_t minor_faults; // No disk I/O needed
    
    // TLB
    uint64_t tlb_hits;
    uint64_t tlb_misses;
    
    // Swap I/O
    uint64_t swap_ins;
    uint64_t swap_outs;
    
    // Page replacements
    uint64_t replacements;
    
    // Timing (simulated)
    uint64_t total_memory_access_time_us; // Microseconds
    uint64_t simulation_start_time_us;
    uint64_t simulation_end_time_us;
    
    // Per-process metrics
    ProcessMetrics *process_metrics;
    uint32_t num_processes;
    uint32_t max_processes;
    
} Metrics;

// Configuration for AMT calculation
typedef struct {
    uint64_t tlb_hit_time_ns;      // TLB hit latency (nanoseconds)
    uint64_t memory_access_time_ns; // Memory access time
    uint64_t page_fault_time_us;    // Page fault handling time (microseconds)
    uint64_t swap_io_time_us;       // Swap I/O time
} AccessTimeConfig;

// Metrics operations
Metrics *metrics_create(uint32_t max_processes);
void metrics_destroy(Metrics *metrics);

// Record events
void metrics_record_access(Metrics *m, uint32_t pid, bool is_write);
void metrics_record_tlb_hit(Metrics *m, uint32_t pid);
void metrics_record_tlb_miss(Metrics *m, uint32_t pid);
void metrics_record_page_fault(Metrics *m, uint32_t pid, bool is_major);
void metrics_record_swap_in(Metrics *m);
void metrics_record_swap_out(Metrics *m);
void metrics_record_replacement(Metrics *m);

// Simulation timing
void metrics_start_simulation(Metrics *m);
void metrics_end_simulation(Metrics *m);

// Derived metrics
double metrics_get_page_fault_rate(Metrics *m);
double metrics_get_tlb_hit_rate(Metrics *m);
double metrics_get_avg_memory_access_time(Metrics *m, AccessTimeConfig *config);

// Reporting
void metrics_print_summary(Metrics *m, FILE *out, AccessTimeConfig *config);
void metrics_print_per_process(Metrics *m, FILE *out);
bool metrics_save_csv(Metrics *m, const char *filename, const char *config_name,
                      AccessTimeConfig *time_config);
bool metrics_save_json(Metrics *m, const char *filename, AccessTimeConfig *time_config);

#endif // METRICS_H

