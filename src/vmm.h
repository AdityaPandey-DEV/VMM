/**
 * vmm.h - Virtual Memory Manager core
 * 
 * Main VMM orchestration: address translation, page fault handling, TLB management,
 * and multi-process support.
 */

#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stdbool.h>
#include "pagetable.h"
#include "frame.h"
#include "tlb.h"
#include "swap.h"
#include "replacement.h"
#include "metrics.h"
#include "trace.h"

// VMM configuration
typedef struct {
    // Memory configuration
    uint32_t ram_size_mb;           // Physical RAM size in MB
    uint32_t page_size;             // Page size in bytes
    uint32_t num_frames;            // Total physical frames
    uint64_t virtual_addr_space;    // Virtual address space per process
    
    // TLB configuration
    uint32_t tlb_size;
    TLBPolicy tlb_policy;
    
    // Page table configuration
    PageTableType pt_type;
    
    // Replacement algorithm
    ReplacementAlgorithm replacement_algo;
    
    // Swap configuration
    uint32_t swap_size_mb;
    
    // Simulation parameters
    uint32_t max_processes;
    uint64_t max_instructions;      // Stop after N memory accesses
    uint32_t random_seed;
    
    // Access time simulation
    AccessTimeConfig access_times;
    
    // Verbosity
    bool verbose;
    bool debug;
    
} VMMConfig;

// Process descriptor
typedef struct {
    uint32_t pid;
    PageTable *page_table;
    bool active;
} Process;

// VMM instance
typedef struct {
    VMMConfig config;
    
    // Core components
    FrameAllocator *frame_allocator;
    TLB *tlb;
    SwapManager *swap;
    ReplacementPolicy *replacement_policy;
    Metrics *metrics;
    
    // Process management
    Process *processes;
    uint32_t num_processes;
    
} VMM;

// VMM lifecycle
VMM *vmm_create(VMMConfig *config);
void vmm_destroy(VMM *vmm);

// Process management
bool vmm_add_process(VMM *vmm, uint32_t pid);
Process *vmm_get_process(VMM *vmm, uint32_t pid);

// Main memory access interface
bool vmm_access(VMM *vmm, uint32_t pid, uint64_t virtual_addr, bool is_write);

// Run trace
bool vmm_run_trace(VMM *vmm, Trace *trace);

// Configuration helpers
void vmm_config_init_default(VMMConfig *config);
void vmm_config_print(VMMConfig *config, FILE *out);

#endif // VMM_H

