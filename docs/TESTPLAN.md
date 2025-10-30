# VMM Test Plan

Comprehensive test plan with acceptance criteria for the Virtual Memory Manager simulator.

---

## Test Categories

1. **Unit Tests**: Individual module testing
2. **Integration Tests**: Multi-component interaction
3. **Functional Tests**: End-to-end scenarios
4. **Performance Tests**: Efficiency and scalability
5. **Regression Tests**: Ensure fixes don't break existing functionality

---

## Unit Tests

### UT-1: Frame Allocator

**Test Cases:**

| ID | Test | Expected Result |
|----|------|----------------|
| UT-1.1 | Allocate single frame | Returns frame number 0-N |
| UT-1.2 | Allocate all frames | All succeed until exhausted |
| UT-1.3 | Allocate when full | Returns -1 |
| UT-1.4 | Free allocated frame | Success, frame returns to pool |
| UT-1.5 | Free already-free frame | Warning logged, no crash |
| UT-1.6 | Allocate after free | Previously freed frame can be reused |
| UT-1.7 | Frame metadata | PID, VPN correctly set |
| UT-1.8 | Aging operation | Age counters shift correctly |

**Acceptance Criteria:**
- Zero memory leaks (verified with valgrind)
- All allocations O(1) time
- Bitmap correctly reflects allocation state

---

### UT-2: Page Table

**Test Cases:**

| ID | Test | Expected Result |
|----|------|----------------|
| UT-2.1 | Create single-level PT | Allocates correct number of PTEs |
| UT-2.2 | Create two-level PT | L1 allocated, L2 allocated on-demand |
| UT-2.3 | Lookup unmapped page | Returns invalid PTE |
| UT-2.4 | Map virtual to physical | PTE marked valid, correct frame |
| UT-2.5 | Lookup mapped page | Returns valid PTE with correct frame |
| UT-2.6 | Unmap page | PTE marked invalid |
| UT-2.7 | Out-of-bounds address | Returns NULL safely |
| UT-2.8 | Two-level L2 allocation | L2 table allocated only when accessed |

**Acceptance Criteria:**
- Single-level: O(1) lookup
- Two-level: O(1) lookup with lazy L2 allocation
- No memory leaks

---

### UT-3: TLB

**Test Cases:**

| ID | Test | Expected Result |
|----|------|----------------|
| UT-3.1 | Lookup in empty TLB | Miss |
| UT-3.2 | Insert and lookup | Hit with correct PFN |
| UT-3.3 | Fill TLB and insert | Evicts according to policy |
| UT-3.4 | Multi-process tagging | PID correctly distinguishes entries |
| UT-3.5 | Invalidate single entry | Entry removed |
| UT-3.6 | Invalidate all for PID | All PID entries removed |
| UT-3.7 | FIFO replacement | Evicts oldest entry |
| UT-3.8 | LRU replacement | Evicts least recently used |

**Acceptance Criteria:**
- Hit/miss correctly detected
- Replacement policy correctly implemented
- No thrashing with reasonable TLB size

---

### UT-4: Swap Manager

**Test Cases:**

| ID | Test | Expected Result |
|----|------|----------------|
| UT-4.1 | Allocate swap slot | Returns slot number |
| UT-4.2 | Allocate all slots | All succeed until exhausted |
| UT-4.3 | Allocate when full | Returns -1 |
| UT-4.4 | Free swap slot | Slot returned to pool |
| UT-4.5 | Swap out | Returns simulated latency |
| UT-4.6 | Swap in | Returns simulated latency |
| UT-4.7 | Statistics | Swap-in/out counts incremented |

**Acceptance Criteria:**
- Swap I/O simulates realistic latency (5ms)
- No slot leaks

---

### UT-5: Replacement Algorithms

**Test Cases:**

| ID | Algorithm | Test | Expected Result |
|----|-----------|------|----------------|
| UT-5.1 | FIFO | Select victim | Oldest allocated frame |
| UT-5.2 | FIFO | Insert order | Maintains insertion order |
| UT-5.3 | LRU | Select victim | Frame with minimum access time |
| UT-5.4 | LRU | Update on access | Access time updated |
| UT-5.5 | Clock | Select victim | First frame with ref_bit=0 |
| UT-5.6 | Clock | Second chance | Clears ref_bit on pass |
| UT-5.7 | Approx-LRU | Aging | Age counters shift correctly |
| UT-5.8 | OPT | Select victim | Frame used furthest in future |

**Acceptance Criteria:**
- Each algorithm selects appropriate victim
- FIFO/LRU/Clock: O(N) worst case
- Approx-LRU: Closely approximates LRU

---

### UT-6: Trace Parser

**Test Cases:**

| ID | Test | Expected Result |
|----|------|----------------|
| UT-6.1 | Load valid trace | All entries parsed |
| UT-6.2 | Load malformed trace | Graceful error, skip bad lines |
| UT-6.3 | Generate sequential | Addresses sequential |
| UT-6.4 | Generate random | Addresses random, deterministic with seed |
| UT-6.5 | Generate working_set | 90% within working set |
| UT-6.6 | Save and reload | Identical trace |

**Acceptance Criteria:**
- Supports hex and decimal addresses
- Deterministic generation with seed

---

## Integration Tests

### IT-1: TLB + Page Table

**Scenario:** TLB miss should trigger page table walk

**Steps:**
1. Access unmapped page (TLB miss)
2. Page fault occurs
3. Map page in page table
4. Insert into TLB
5. Re-access (should be TLB hit)

**Expected:**
- First access: TLB miss, page fault
- Second access: TLB hit
- TLB hit rate: 50%

---

### IT-2: Page Fault + Frame Allocation

**Scenario:** Page fault allocates frame

**Steps:**
1. Access unmapped page
2. Page fault handler invoked
3. Frame allocated
4. Page mapped to frame

**Expected:**
- Page fault count: 1
- Frame allocated
- Valid PTE created

---

### IT-3: Frame Eviction + Swap

**Scenario:** No free frames triggers eviction and swap

**Setup:** RAM size < working set

**Steps:**
1. Fill all frames
2. Access new page
3. Victim selected
4. Dirty victim swapped out
5. New page loaded

**Expected:**
- Replacement count: >0
- Swap-out count: >0 (if victims dirty)
- No frame leaks

---

### IT-4: Multi-Process Isolation

**Scenario:** Processes have isolated address spaces

**Steps:**
1. Process 0 maps page at 0x1000
2. Process 1 maps page at 0x1000
3. Both access 0x1000

**Expected:**
- Different physical frames
- No cross-contamination
- TLB correctly tagged by PID

---

## Functional Tests

### FT-1: Sequential Access

**Trace:** `sequential.trace` (1000 accesses, sequential pages)

**Configuration:**
- RAM: 16 MB (4096 frames)
- Page size: 4KB
- Algorithm: Any

**Expected Metrics:**
- Page faults: ~100-200 (working set fits in memory)
- Page fault rate: <20%
- TLB hit rate: >85% (high locality)

---

### FT-2: Random Access

**Trace:** `random.trace` (5000 accesses, random pages)

**Configuration:**
- RAM: 16 MB
- Page size: 4KB
- Algorithm: LRU

**Expected Metrics:**
- Page faults: >500
- Page fault rate: >10%
- TLB hit rate: <70% (low locality)

---

### FT-3: Working Set

**Trace:** `working_set.trace` (10000 accesses, 90% within working set)

**Configuration:**
- RAM: 32 MB
- Page size: 4KB
- Algorithm: LRU

**Expected Metrics:**
- Page faults: 200-500
- Page fault rate: 2-5%
- TLB hit rate: >90%

---

### FT-4: Thrashing

**Trace:** `thrashing.trace` (15000 accesses, working set > RAM)

**Configuration:**
- RAM: 8 MB (small)
- Page size: 4KB
- Algorithm: FIFO

**Expected Metrics:**
- Page faults: >5000
- Page fault rate: >30%
- Swap I/O: Very high
- Performance: Poor AMT

---

### FT-5: Algorithm Comparison

**Trace:** Same trace for all algorithms

**Algorithms:** FIFO, LRU, Clock, OPT

**Expected Order (page faults, best to worst):**
1. OPT (best)
2. LRU
3. Clock
4. FIFO (worst, subject to Belady's anomaly)

**Acceptance:**
- OPT has fewest page faults
- LRU ≈ Clock (within 10%)
- FIFO may be worse than LRU

---

### FT-6: TLB Size Impact

**Trace:** `locality.trace`

**Configurations:**
- TLB size: 8, 16, 32, 64, 128, 256

**Expected:**
- TLB hit rate increases with size
- Diminishing returns after 128 entries
- AMT decreases with larger TLB

---

### FT-7: Page Size Impact

**Trace:** Same trace

**Configurations:**
- Page size: 4KB, 8KB, 16KB

**Expected:**
- Larger pages: Fewer page faults (more data per fault)
- Larger pages: Potential internal fragmentation
- Trade-off visible in metrics

---

## Performance Tests

### PT-1: Scalability - Large RAM

**Configuration:**
- RAM: 1 GB (262,144 frames)
- Trace: 100,000 accesses

**Acceptance:**
- Completes within 10 seconds
- Memory usage: <2 GB
- No performance degradation

---

### PT-2: Scalability - Long Trace

**Configuration:**
- RAM: 64 MB
- Trace: 1,000,000 accesses

**Acceptance:**
- Completes within 30 seconds
- Linear time complexity O(n)

---

### PT-3: Algorithm Efficiency

**Measure:** Victim selection time

**Expected:**
- FIFO: O(1)
- Clock: O(1) amortized
- Approx-LRU: O(1) amortized
- LRU: O(num_frames) (acceptable for <10K frames)
- OPT: O(num_frames × trace_remaining) (slow, for comparison only)

---

### PT-4: Memory Footprint

**Configuration:**
- RAM: 64 MB
- Processes: 16

**Acceptance:**
- Single-level PT: ~16 MB per process (expected)
- Two-level PT: <1 MB per process (sparse)
- Choose PT type based on address space size

---

## Regression Tests

### RT-1: Memory Leaks

**Tool:** Valgrind

**Command:**
```bash
valgrind --leak-check=full --show-leak-kinds=all ./bin/vmm -r 64 -t traces/random.trace -a LRU
```

**Acceptance:**
- 0 bytes definitely lost
- 0 bytes indirectly lost
- All heap blocks freed

---

### RT-2: Address Sanitizer

**Build:**
```bash
make debug
```

**Run:**
```bash
./bin/vmm_debug -r 32 -t traces/thrashing.trace -a CLOCK
```

**Acceptance:**
- No heap-buffer-overflow
- No use-after-free
- No memory leaks

---

### RT-3: Undefined Behavior

**Tool:** UBSan (built into `make debug`)

**Acceptance:**
- No signed integer overflow
- No null pointer dereference
- No unaligned memory access

---

## Acceptance Test Suite

### Automated Test Execution

```bash
make test
```

**Tests Executed:**
1. Generate sample traces ✓
2. Run FIFO algorithm ✓
3. Run LRU algorithm ✓
4. Run Clock algorithm ✓
5. Run OPT algorithm ✓
6. TLB size comparison ✓
7. Multi-process workload ✓
8. Thrashing scenario ✓
9. Two-level page table ✓
10. Different page sizes ✓

**Pass Criteria:**
- All 10 tests pass
- Exit code: 0

---

## Expected Output Samples

### Sample Output: Working Set Trace, LRU, 64MB RAM

```
==================== SIMULATION SUMMARY ====================

Memory Accesses:
  Total:              10000
  Reads:               8000 (80.0%)
  Writes:              2000 (20.0%)

Page Faults:
  Total:                 324
  Major:                  42 (required swap-in)
  Minor:                 282 (no I/O)
  Fault Rate:           3.24%

TLB Performance:
  Hits:                 9456
  Misses:                544
  Hit Rate:            94.56%

Swap I/O:
  Swap-ins:               42
  Swap-outs:              38
  Replacements:          324

Average Memory Access Time:
  AMT:               145.32 ns
  Slowdown:            145.3x (vs TLB hit)

Simulation Time:
  Wall time:            2.345 ms
  Throughput:         4264.4 accesses/ms

============================================================
```

---

## Test Data Files

All test traces available in `traces/` directory:

1. `simple.trace` - 15 accesses, 2 processes (manual verification)
2. `sequential.trace` - 1000 accesses, sequential pattern
3. `random.trace` - 5000 accesses, uniform random
4. `working_set.trace` - 10000 accesses, 90% locality
5. `locality.trace` - 8000 accesses, temporal/spatial locality
6. `thrashing.trace` - 15000 accesses, exceeds RAM capacity

---

## Quality Checklist

- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] All functional tests produce expected metrics (±10%)
- [ ] No memory leaks (valgrind clean)
- [ ] No undefined behavior (UBSan clean)
- [ ] No address sanitizer errors
- [ ] Code compiles without warnings (`-Wall -Wextra`)
- [ ] Deterministic behavior with same seed
- [ ] Documentation matches implementation
- [ ] Examples in README.md work as described

---

## Known Limitations

1. **Single-threaded**: No concurrent memory access simulation
2. **No actual data**: Pages don't store data, only metadata
3. **Simplified page tables**: Real CPUs use more complex structures
4. **No TLB shootdown**: Multi-core TLB coherency not simulated

These are intentional simplifications for clarity and focus on core concepts.

---

## Future Test Enhancements

1. Property-based testing (e.g., QuickCheck-style)
2. Fuzzing with AFL or libFuzzer
3. Comparative testing against other VM simulators
4. Stress testing with massive traces (>100M accesses)
5. Multi-threaded trace execution

---

## Bug Reporting

If tests fail, provide:
1. Command line used
2. Trace file (if custom)
3. Expected vs actual metrics
4. Log output (use `-D` flag)
5. Environment (OS, GCC version)

