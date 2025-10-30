# VMM Design Document

## Architecture Overview

The Virtual Memory Manager simulator is designed as a modular system with clear separation between policy and mechanism. The architecture follows a layered approach:

```
┌─────────────────────────────────────────────┐
│          Main / CLI Interface               │
│         (main.c, trace_gen.c)               │
└─────────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────────┐
│        VMM Core Orchestration               │
│           (vmm.c / vmm.h)                   │
│  - Address translation pipeline             │
│  - Page fault handling                      │
│  - Process management                       │
└─────────────────────────────────────────────┘
          │          │          │
     ┌────┴───┐  ┌───┴────┐  ┌─┴────────┐
     │        │  │        │  │          │
┌────▼───┐ ┌─▼──▼──┐ ┌───▼──▼─┐ ┌──────▼────┐
│  TLB   │ │ Page  │ │ Frame  │ │   Swap    │
│        │ │ Table │ │ Alloc  │ │           │
└────────┘ └───────┘ └────────┘ └───────────┘
     │                    │
┌────▼────────────────────▼─────────────────┐
│      Replacement Policy Engine            │
│   (FIFO, LRU, Clock, OPT, Approx-LRU)     │
└───────────────────────────────────────────┘
                    │
┌───────────────────▼───────────────────────┐
│         Metrics Collection                │
│    (counters, timers, reporting)          │
└───────────────────────────────────────────┘
                    │
┌───────────────────▼───────────────────────┐
│    Trace Engine (parse/generate)          │
└───────────────────────────────────────────┘
```

---

## Core Components

### 1. VMM Core (`vmm.c`)

**Responsibilities:**
- Orchestrate the address translation pipeline
- Handle TLB lookup → Page table walk → Page fault
- Manage multiple processes with isolated address spaces
- Coordinate between all subsystems

**Key Functions:**
- `vmm_access()`: Main entry point for memory access
- `vmm_handle_page_fault()`: Page fault handler
- `vmm_run_trace()`: Execute trace file

**Design Decisions:**
- **Stateless access function**: Each `vmm_access()` call is independent
- **Lazy process creation**: Processes created on-demand from trace
- **Explicit policy injection**: Replacement policy is pluggable

---

### 2. Page Table (`pagetable.c`)

**Responsibilities:**
- Virtual-to-physical address translation
- Support single-level and two-level page tables
- PTE (Page Table Entry) flag management

**Data Structures:**

```c
// Single-level: Direct array
PageTableEntry ptes[num_pages];

// Two-level: Sparse structure
PageTableEntry **l1_table;  // L1[1024] -> L2[n]
```

**Design Decisions:**
- **Single vs Two-level**: Single-level for simplicity, two-level for large address spaces
- **Lazy L2 allocation**: L2 tables allocated only when accessed
- **Compact PTEs**: Frame number + flags in one structure

**Complexity:**
- Lookup: O(1) single-level, O(1) two-level
- Map/Unmap: O(1)
- Space: O(virtual_pages) single, O(used_pages) two-level

---

### 3. Frame Allocator (`frame.c`)

**Responsibilities:**
- Physical memory (frame) allocation and deallocation
- Track frame metadata (PID, VPN, dirty, reference bits)
- Support efficient victim selection

**Data Structures:**

```c
FrameInfo frames[num_frames];  // Metadata array
uint32_t free_list[num_frames]; // Stack of free frames
uint8_t bitmap[num_frames/8];   // Free/used bitmap
```

**Design Decisions:**
- **Stack-based free list**: O(1) allocation
- **Bitmap for quick checks**: Fast free/used lookup
- **Rich frame metadata**: Support multiple replacement algorithms
- **Aging counters**: For approximate LRU (NFU)

**Complexity:**
- Allocate: O(1)
- Free: O(1)
- Age all frames: O(num_frames)

---

### 4. TLB (`tlb.c`)

**Responsibilities:**
- Fast address translation cache
- Support FIFO and LRU replacement
- Per-process tagging (ASID simulation)

**Data Structures:**

```c
TLBEntry entries[tlb_size];
```

**Design Decisions:**
- **Linear search**: Simple and fast for small TLB sizes (typical: 16-128 entries)
- **Tagged entries**: Include PID to avoid flushing on context switch
- **LRU via timestamps**: Use access counter for ordering

**Complexity:**
- Lookup: O(tlb_size) ≈ O(1) for small TLB
- Insert: O(tlb_size) for LRU, O(1) for FIFO

**Optimization Opportunity**: For very large TLBs, use hash table

---

### 5. Swap Manager (`swap.c`)

**Responsibilities:**
- Simulate disk-based backing store
- Allocate/free swap slots
- Track swap I/O statistics

**Design Decisions:**
- **Simulated I/O**: No actual disk access, just latency simulation
- **Stack-based allocation**: O(1) swap slot allocation
- **Configurable latency**: Realistic 5ms swap I/O time

**Complexity:**
- All operations: O(1)

---

### 6. Replacement Algorithms (`replacement.c`)

**Responsibilities:**
- Select victim frame for eviction
- Implement multiple algorithms with unified interface

#### FIFO (First-In-First-Out)
- **Data**: Circular queue of frame numbers
- **Victim selection**: Head of queue
- **Complexity**: O(1)
- **Pros**: Simple, predictable
- **Cons**: No consideration of access patterns (Belady's anomaly)

#### LRU (Least Recently Used)
- **Data**: Timestamp per frame (`last_access_time`)
- **Victim selection**: Frame with minimum timestamp
- **Complexity**: O(num_frames) scan
- **Pros**: Optimal for many workloads
- **Cons**: O(n) victim selection, requires exact tracking

#### Approximate LRU (Aging/NFU)
- **Data**: 32-bit age counter per frame
- **Algorithm**: Periodic right-shift of counters, add reference bit to MSB
- **Victim selection**: Frame with minimum age counter
- **Complexity**: O(num_frames) scan, but infrequent aging
- **Pros**: O(1) amortized, close to LRU performance
- **Cons**: Periodic global operation

#### Clock (Second-Chance)
- **Data**: Reference bit per frame, clock hand pointer
- **Algorithm**: Scan circularly, clear reference bits, evict first with bit=0
- **Victim selection**: O(num_frames) worst case, but typically O(1)
- **Pros**: Good balance of performance and simplicity
- **Cons**: Can scan entire frame table in worst case

#### OPT (Optimal/Belady)
- **Data**: Pointer to trace for future knowledge
- **Victim selection**: Evict page used furthest in future
- **Complexity**: O(num_frames × remaining_trace)
- **Pros**: Provably optimal, useful as baseline
- **Cons**: Requires future knowledge, expensive

**Design Pattern**: Strategy pattern for pluggable algorithms

---

### 7. Trace Engine (`trace.c`)

**Responsibilities:**
- Parse memory access trace files
- Generate synthetic traces with patterns

**Trace Patterns:**

1. **Sequential**: Linear page access (best case for prefetching)
2. **Random**: Uniform random addresses (worst case for caching)
3. **Working Set**: 90% within localized set, 10% random (realistic)
4. **Locality**: Temporal/spatial locality with occasional jumps
5. **Thrashing**: Access more pages than RAM capacity (pathological)

**Deterministic Generation**: Uses seeded LCG for reproducibility

---

### 8. Metrics (`metrics.c`)

**Responsibilities:**
- Collect performance counters
- Calculate derived metrics
- Generate reports in multiple formats

**Key Metrics:**

- **Page Faults**: Total, major (swap I/O), minor
- **Fault Rate**: page_faults / total_accesses
- **TLB Hits/Misses**: TLB effectiveness
- **Swap I/O**: swap-ins, swap-outs
- **AMT (Average Memory Access Time)**:
  ```
  AMT = TLB_hit_time + 
        (TLB_miss_rate × PT_access_time) +
        (page_fault_rate × page_fault_penalty)
  ```

**Per-Process Metrics**: Breakdown by PID for multi-process workloads

---

## Address Translation Pipeline

```
Virtual Address (VA)
        │
        ├──> [TLB Lookup] ──(HIT)──> Physical Address (PA)
        │            │
        │          (MISS)
        │            │
        └────> [Page Table Walk]
                     │
               [PTE Valid?]
                 │       │
               (YES)   (NO = Page Fault)
                 │       │
                PA   [Page Fault Handler]
                         │
                    [Allocate Frame]
                         │
                    [Victim needed?]
                      │         │
                    (NO)      (YES)
                      │         │
                      │    [Select Victim]
                      │         │
                      │    [Evict if dirty]
                      │         │
                      └─────────┤
                                │
                        [Map VA -> Frame]
                                │
                        [Update TLB]
                                │
                               PA
```

---

## Memory Layout & Data Structure Design

### Cache Efficiency Considerations

1. **Contiguous Arrays**: Frame metadata, PTEs stored in arrays for cache locality
2. **Hot/Cold Separation**: Frequently accessed fields (frame_number, flags) grouped together
3. **Bitmap Alignment**: Free-frame bitmap aligned to cache lines

### Space Complexity

| Component | Space Complexity |
|-----------|------------------|
| Single-level PT | O(virtual_pages) |
| Two-level PT | O(L1_entries + used_L2_tables × L2_entries) |
| Frame metadata | O(num_frames) |
| TLB | O(tlb_size) |
| Swap | O(swap_slots) |

### Typical Memory Usage

For 64MB RAM, 4KB pages, 32-bit addresses:
- Frames: 16,384 frames
- Frame metadata: 16,384 × 64 bytes ≈ 1 MB
- Single-level PT (per process): 1M entries × 16 bytes ≈ 16 MB
- Two-level PT (per process): ~16 KB (sparse)

**Recommendation**: Use two-level PT for >1GB virtual space

---

## Tradeoffs & Design Choices

### 1. Exact LRU vs Approximate LRU

| Aspect | Exact LRU | Approximate LRU |
|--------|-----------|-----------------|
| Victim selection | O(num_frames) | O(num_frames) |
| Access overhead | Update timestamp | Set reference bit |
| Accuracy | Perfect | ~95% of LRU |
| Implementation | Scan for min timestamp | Aging with periodic shifts |

**Choice**: Provide both; use Approx-LRU for large memory

### 2. TLB Size vs Hit Rate

Empirical data (from working_set trace):
- 16 entries: ~75% hit rate
- 64 entries: ~92% hit rate
- 256 entries: ~98% hit rate

**Diminishing returns** above 128 entries for typical workloads

### 3. Page Table Structure

| Virtual Space | Recommended PT Type | Reason |
|---------------|---------------------|--------|
| <1 GB | Single-level | Simplicity, speed |
| 1-64 GB | Two-level | Space efficiency |
| >64 GB | Multi-level (>2) or hashed | Scalability |

### 4. Swap Size

**Rule of thumb**: swap = 2× RAM for safety

Thrashing begins when: `working_set_size > (RAM + swap_throughput × refill_time)`

---

## Simulation Fidelity

### What We Simulate

- Address translation latency (TLB, PT walks)
- Page fault handling time
- Swap I/O latency
- Memory access patterns

### What We Don't Simulate (Future Extensions)

- Actual data storage (content of pages)
- Hardware page table walkers
- TLB shootdown for multi-core
- NUMA effects
- Cache hierarchy below TLB

---

## Extension Points

1. **Copy-on-Write**:
   - Add `COW` flag to PTEs
   - Track reference counts in `FrameInfo`
   - Implement copy in page fault handler

2. **Shared Memory**:
   - Multi-map frames in page tables
   - Reference counting in frame allocator

3. **Memory-Mapped Files**:
   - Add file descriptor to PTEs
   - Load from "file" on page fault

4. **Large Pages (Huge Pages)**:
   - Support multiple page sizes
   - Extend page table to indicate size

5. **Prefetching**:
   - Detect sequential patterns in trace
   - Speculatively load adjacent pages

---

## Performance Considerations

### Bottlenecks

1. **Exact LRU**: O(num_frames) scan on every eviction
2. **OPT Algorithm**: O(num_frames × trace_length)
3. **Page Table Walks**: For large two-level tables

### Optimizations Implemented

1. ✅ Bitmap for O(1) frame allocation
2. ✅ Approximate LRU with aging
3. ✅ TLB to reduce PT walks
4. ✅ Lazy L2 table allocation

### Future Optimizations

- Hash table for page tables in sparse address spaces
- Lock-free data structures for multi-threading
- SIMD for aging all frames

---

## Testing Strategy

See [TESTPLAN.md](TESTPLAN.md) for detailed test plan.

---

## References

- [Intel 64 and IA-32 Architectures Software Developer's Manual](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
- Linux kernel: `mm/` subsystem
- *Computer Architecture: A Quantitative Approach* by Hennessy and Patterson

