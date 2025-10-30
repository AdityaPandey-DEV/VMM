# VMM API Documentation

This document describes the developer API for extending and integrating with the VMM simulator.

---

## Table of Contents

1. [Core VMM Interface](#core-vmm-interface)
2. [Page Table API](#page-table-api)
3. [Frame Allocator API](#frame-allocator-api)
4. [TLB API](#tlb-api)
5. [Swap Manager API](#swap-manager-api)
6. [Replacement Policy API](#replacement-policy-api)
7. [Trace API](#trace-api)
8. [Metrics API](#metrics-api)
9. [Extension Guide](#extension-guide)

---

## Core VMM Interface

### VMM Lifecycle

```c
// Create VMM instance with configuration
VMM *vmm_create(VMMConfig *config);

// Destroy VMM and free all resources
void vmm_destroy(VMM *vmm);
```

### Configuration

```c
typedef struct {
    uint32_t ram_size_mb;
    uint32_t page_size;
    uint32_t num_frames;
    uint64_t virtual_addr_space;
    uint32_t tlb_size;
    TLBPolicy tlb_policy;
    PageTableType pt_type;
    ReplacementAlgorithm replacement_algo;
    uint32_t swap_size_mb;
    uint32_t max_processes;
    uint64_t max_instructions;
    uint32_t random_seed;
    AccessTimeConfig access_times;
    bool verbose;
    bool debug;
} VMMConfig;

// Initialize config with defaults
void vmm_config_init_default(VMMConfig *config);

// Print config to stream
void vmm_config_print(VMMConfig *config, FILE *out);
```

### Process Management

```c
// Add a process to VMM
bool vmm_add_process(VMM *vmm, uint32_t pid);

// Get process descriptor
Process *vmm_get_process(VMM *vmm, uint32_t pid);
```

### Memory Access

```c
// Access virtual memory (main interface)
// Returns: true on success, false on error
bool vmm_access(VMM *vmm, uint32_t pid, uint64_t virtual_addr, bool is_write);

// Run complete trace
bool vmm_run_trace(VMM *vmm, Trace *trace);
```

**Usage Example:**
```c
VMMConfig config;
vmm_config_init_default(&config);
config.ram_size_mb = 128;
config.replacement_algo = REPLACE_LRU;

VMM *vmm = vmm_create(&config);
vmm_add_process(vmm, 1);

// Simulate memory accesses
vmm_access(vmm, 1, 0x1000, false);  // Read from 0x1000
vmm_access(vmm, 1, 0x2000, true);   // Write to 0x2000

vmm_destroy(vmm);
```

---

## Page Table API

### Page Table Operations

```c
// Create page table for a process
PageTable *pagetable_create(uint32_t pid, PageTableType type,
                             uint64_t address_space_size, uint32_t page_size);

// Destroy page table
void pagetable_destroy(PageTable *pt);

// Look up page table entry for virtual address
PageTableEntry *pagetable_lookup(PageTable *pt, uint64_t virtual_addr);

// Map virtual address to physical frame
bool pagetable_map(PageTable *pt, uint64_t virtual_addr, 
                   uint32_t frame_number, uint32_t flags);

// Unmap virtual address
bool pagetable_unmap(PageTable *pt, uint64_t virtual_addr);

// Count valid pages in page table
uint32_t pagetable_count_valid_pages(PageTable *pt);
```

### PTE Manipulation

```c
// Check PTE flags
bool pte_is_valid(PageTableEntry *pte);
bool pte_is_dirty(PageTableEntry *pte);
bool pte_is_accessed(PageTableEntry *pte);

// Modify PTE
void pte_set_frame(PageTableEntry *pte, uint32_t frame_number);
void pte_set_valid(PageTableEntry *pte, bool valid);
void pte_set_dirty(PageTableEntry *pte, bool dirty);
void pte_set_accessed(PageTableEntry *pte, bool accessed);
```

### PTE Flags

```c
#define PTE_VALID    (1 << 0)  // Page in physical memory
#define PTE_DIRTY    (1 << 1)  // Page modified
#define PTE_ACCESSED (1 << 2)  // Page accessed (for algorithms)
#define PTE_WRITE    (1 << 3)  // Writable
#define PTE_USER     (1 << 4)  // User-mode accessible
```

**Extension Example: Add Execute Permission**
```c
// In pagetable.h:
#define PTE_EXEC (1 << 5)

// In page fault handler:
if (is_executable_access && !(pte->flags & PTE_EXEC)) {
    // Handle execution permission fault
}
```

---

## Frame Allocator API

### Frame Operations

```c
// Create frame allocator
FrameAllocator *frame_allocator_create(uint32_t num_frames);

// Destroy frame allocator
void frame_allocator_destroy(FrameAllocator *allocator);

// Allocate frame (returns frame number or -1)
int32_t frame_alloc(FrameAllocator *allocator);

// Free frame
bool frame_free(FrameAllocator *allocator, uint32_t frame_num);

// Get frame info
FrameInfo *frame_get_info(FrameAllocator *allocator, uint32_t frame_num);

// Check if frame is free
bool frame_is_free(FrameAllocator *allocator, uint32_t frame_num);

// Get free frame count
uint32_t frame_get_free_count(FrameAllocator *allocator);
```

### Frame Metadata

```c
typedef struct {
    uint32_t frame_number;
    uint32_t pid;
    uint64_t vpn;
    FrameState state;
    uint32_t reference_bit;
    uint32_t age_counter;
    uint64_t last_access_time;
    bool dirty;
    uint32_t pin_count;  // For future shared memory
} FrameInfo;

// Update frame metadata
void frame_set_pid(FrameAllocator *allocator, uint32_t frame_num, uint32_t pid);
void frame_set_vpn(FrameAllocator *allocator, uint32_t frame_num, uint64_t vpn);
void frame_set_dirty(FrameAllocator *allocator, uint32_t frame_num, bool dirty);
void frame_set_reference(FrameAllocator *allocator, uint32_t frame_num, bool referenced);
void frame_update_access_time(FrameAllocator *allocator, uint32_t frame_num);

// Age all frames (for approximate LRU)
void frame_age_all(FrameAllocator *allocator);
```

**Extension Example: Add Frame Pinning**
```c
bool frame_pin(FrameAllocator *allocator, uint32_t frame_num) {
    FrameInfo *info = frame_get_info(allocator, frame_num);
    if (info) {
        info->pin_count++;
        return true;
    }
    return false;
}

// In victim selection, skip pinned frames:
if (frame->pin_count > 0) continue;
```

---

## TLB API

```c
// Create TLB
TLB *tlb_create(uint32_t size, TLBPolicy policy);

// Destroy TLB
void tlb_destroy(TLB *tlb);

// Lookup translation (returns true if hit, sets *pfn)
bool tlb_lookup(TLB *tlb, uint32_t pid, uint64_t vpn, uint32_t *pfn);

// Insert translation
void tlb_insert(TLB *tlb, uint32_t pid, uint64_t vpn, uint32_t pfn);

// Invalidate single entry
void tlb_invalidate(TLB *tlb, uint32_t pid, uint64_t vpn);

// Invalidate all entries for a process
void tlb_invalidate_all(TLB *tlb, uint32_t pid);

// Flush entire TLB
void tlb_flush(TLB *tlb);
```

---

## Swap Manager API

```c
// Create swap manager
SwapManager *swap_create(uint32_t num_slots);

// Destroy swap manager
void swap_destroy(SwapManager *swap);

// Allocate swap slot (returns slot number or -1)
int32_t swap_alloc(SwapManager *swap, uint32_t pid, uint64_t vpn);

// Free swap slot
bool swap_free(SwapManager *swap, uint32_t slot);

// Swap operations (return simulated latency in microseconds)
uint64_t swap_out(SwapManager *swap, uint32_t slot, void *data);
uint64_t swap_in(SwapManager *swap, uint32_t slot, void *data);

// Statistics
uint32_t swap_get_used_count(SwapManager *swap);
uint32_t swap_get_free_count(SwapManager *swap);
```

---

## Replacement Policy API

### Creating Policies

```c
// Create replacement policy
ReplacementPolicy *replacement_create(ReplacementAlgorithm algo, uint32_t num_frames);

// Destroy policy
void replacement_destroy(ReplacementPolicy *policy);

// For OPT algorithm
void replacement_set_trace(ReplacementPolicy *policy, Trace *trace);
void replacement_set_position(ReplacementPolicy *policy, uint64_t index);
```

### Policy Interface

```c
// Select victim frame for eviction
int32_t replacement_select_victim(ReplacementPolicy *policy, FrameAllocator *allocator);

// Notify policy of events
void replacement_on_access(ReplacementPolicy *policy, uint32_t frame_num, 
                           FrameAllocator *allocator);
void replacement_on_allocate(ReplacementPolicy *policy, uint32_t frame_num);
void replacement_on_free(ReplacementPolicy *policy, uint32_t frame_num);

// Get algorithm name
const char *replacement_get_name(ReplacementAlgorithm algo);
```

### Implementing a New Algorithm

**Example: Random Replacement**

```c
// In replacement.h:
typedef enum {
    // ... existing algorithms
    REPLACE_RANDOM
} ReplacementAlgorithm;

// In replacement.c:
case REPLACE_RANDOM: {
    // Select random allocated frame
    uint32_t start = rand() % allocator->total_frames;
    for (uint32_t i = 0; i < allocator->total_frames; i++) {
        uint32_t idx = (start + i) % allocator->total_frames;
        if (allocator->frames[idx].state == FRAME_ALLOCATED) {
            return idx;
        }
    }
    return -1;
}
```

---

## Trace API

### Trace Management

```c
// Create empty trace
Trace *trace_create(uint64_t initial_capacity);

// Destroy trace
void trace_destroy(Trace *trace);

// Load trace from file
Trace *trace_load(const char *filename);

// Save trace to file
bool trace_save(Trace *trace, const char *filename);

// Add entry to trace
bool trace_add(Trace *trace, uint32_t pid, MemoryOperation op, uint64_t virtual_addr);

// Get trace entry
TraceEntry *trace_get(Trace *trace, uint64_t index);
```

### Trace Generation

```c
// Generate synthetic trace
Trace *trace_generate(TracePattern pattern, uint64_t num_accesses,
                      uint32_t num_processes, uint64_t address_space_size,
                      uint32_t seed);

// Available patterns:
typedef enum {
    PATTERN_SEQUENTIAL,
    PATTERN_RANDOM,
    PATTERN_WORKING_SET,
    PATTERN_LOCALITY,
    PATTERN_THRASHING
} TracePattern;
```

**Custom Trace Generation Example:**
```c
Trace *create_custom_trace(void) {
    Trace *trace = trace_create(1000);
    
    // Simulate stack growth
    for (uint64_t i = 0; i < 100; i++) {
        uint64_t stack_addr = 0x7FFFFFFFFFFF - (i * 4096);
        trace_add(trace, 0, OP_WRITE, stack_addr);
    }
    
    // Simulate heap allocation
    for (uint64_t i = 0; i < 100; i++) {
        uint64_t heap_addr = 0x600000000 + (i * 4096);
        trace_add(trace, 0, OP_WRITE, heap_addr);
    }
    
    return trace;
}
```

---

## Metrics API

### Metrics Collection

```c
// Create metrics collector
Metrics *metrics_create(uint32_t max_processes);

// Destroy metrics
void metrics_destroy(Metrics *metrics);

// Record events
void metrics_record_access(Metrics *m, uint32_t pid, bool is_write);
void metrics_record_tlb_hit(Metrics *m, uint32_t pid);
void metrics_record_tlb_miss(Metrics *m, uint32_t pid);
void metrics_record_page_fault(Metrics *m, uint32_t pid, bool is_major);
void metrics_record_swap_in(Metrics *m);
void metrics_record_swap_out(Metrics *m);
void metrics_record_replacement(Metrics *m);

// Timing
void metrics_start_simulation(Metrics *m);
void metrics_end_simulation(Metrics *m);
```

### Metrics Reporting

```c
// Derived metrics
double metrics_get_page_fault_rate(Metrics *m);
double metrics_get_tlb_hit_rate(Metrics *m);
double metrics_get_avg_memory_access_time(Metrics *m, AccessTimeConfig *config);

// Output
void metrics_print_summary(Metrics *m, FILE *out, AccessTimeConfig *config);
void metrics_print_per_process(Metrics *m, FILE *out);
bool metrics_save_csv(Metrics *m, const char *filename, const char *config_name,
                      AccessTimeConfig *time_config);
bool metrics_save_json(Metrics *m, const char *filename, AccessTimeConfig *time_config);
```

**Custom Metric Example:**
```c
// Add to metrics.h:
typedef struct {
    // ... existing fields
    uint64_t copy_on_write_faults;
} Metrics;

void metrics_record_cow_fault(Metrics *m) {
    if (m) m->copy_on_write_faults++;
}
```

---

## Extension Guide

### Adding Copy-on-Write (COW)

1. **Add PTE flag:**
```c
#define PTE_COW (1 << 6)
```

2. **Add reference counting to frames:**
```c
typedef struct {
    // ... existing fields
    uint32_t ref_count;
} FrameInfo;
```

3. **Modify page fault handler:**
```c
static bool vmm_handle_page_fault(VMM *vmm, Process *proc, 
                                   uint64_t virtual_addr, bool is_write) {
    PageTableEntry *pte = pagetable_lookup(proc->page_table, virtual_addr);
    
    if (pte_is_valid(pte) && is_write && (pte->flags & PTE_COW)) {
        // Copy-on-write fault
        FrameInfo *frame = frame_get_info(vmm->frame_allocator, pte->frame_number);
        
        if (frame->ref_count > 1) {
            // Allocate new frame and copy
            int32_t new_frame = frame_alloc(vmm->frame_allocator);
            memcpy(new_frame_data, old_frame_data, page_size);
            
            // Update PTEs
            pte->frame_number = new_frame;
            pte->flags &= ~PTE_COW;
            frame->ref_count--;
        } else {
            // Last reference, just remove COW flag
            pte->flags &= ~PTE_COW;
        }
        return true;
    }
    
    // ... normal page fault handling
}
```

### Adding Shared Memory

1. **Add shared region tracking:**
```c
typedef struct {
    uint32_t shmid;
    uint32_t *frame_numbers;
    uint32_t num_pages;
} SharedMemoryRegion;
```

2. **Map shared region into multiple page tables:**
```c
bool vmm_map_shared(VMM *vmm, uint32_t *pids, uint32_t num_pids,
                    uint64_t vaddr, SharedMemoryRegion *shm) {
    for (uint32_t i = 0; i < num_pids; i++) {
        Process *proc = vmm_get_process(vmm, pids[i]);
        for (uint32_t page = 0; page < shm->num_pages; page++) {
            pagetable_map(proc->page_table, vaddr + page * page_size,
                         shm->frame_numbers[page], PTE_VALID | PTE_USER | PTE_WRITE);
        }
    }
    return true;
}
```

### Adding Prefetching

```c
void vmm_prefetch_sequential(VMM *vmm, uint32_t pid, uint64_t base_addr, uint32_t num_pages) {
    for (uint32_t i = 0; i < num_pages; i++) {
        uint64_t addr = base_addr + (i * vmm->config.page_size);
        PageTableEntry *pte = pagetable_lookup(proc->page_table, addr);
        
        if (!pte_is_valid(pte)) {
            // Trigger page fault proactively
            vmm_handle_page_fault(vmm, proc, addr, false);
        }
    }
}
```

---

## Thread Safety Notes

**Current Implementation**: Single-threaded

**For Multi-threading**:
- Add mutex to `FrameAllocator` for allocation
- Lock-free TLB with atomic operations
- Per-process page table locks
- Consider RCU for read-heavy data structures

---

## Performance Tips

1. **Use Approx-LRU for >1000 frames**: Exact LRU becomes expensive
2. **Two-level PT for sparse address spaces**: Saves memory
3. **TLB size 64-128**: Best performance/size ratio
4. **Compile with -O3**: 2-3x speedup
5. **Profile with perf**: `perf record -g ./vmm ...`

---

## Integration Examples

### Embedding in Simulator

```c
// Create VMM as part of larger system simulator
typedef struct {
    VMM *vmm;
    CPUState *cpu;
    IOSystem *io;
} SystemSimulator;

void system_execute_instruction(SystemSimulator *sys, Instruction *instr) {
    if (instr->type == INSTR_LOAD || instr->type == INSTR_STORE) {
        bool is_write = (instr->type == INSTR_STORE);
        vmm_access(sys->vmm, sys->cpu->current_pid, instr->address, is_write);
    }
    // ... execute instruction
}
```

---

## Error Handling

All API functions return error indicators:
- Pointers: `NULL` on error
- Boolean: `false` on error
- Integers: `-1` on error

Check return values and inspect logs for details.

---

## Complete Example

See `examples/custom_simulator.c` (if provided) for a complete integration example.

