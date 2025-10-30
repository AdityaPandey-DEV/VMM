# Virtual Memory Manager (VMM) Simulator

**Authors:** Aditya Pandey, Kartik, Vivek, Gaurang

A complete, professional virtual memory simulation framework for learning, teaching, and research. Implements core OS memory management concepts including multi-level page tables, TLB, demand paging, swap, and multiple page replacement algorithms.

---

## Features

### Core Functionality
- **Multi-level Page Tables**: Single-level and two-level page table support
- **Translation Lookaside Buffer (TLB)**: Configurable size with FIFO and LRU policies
- **Demand Paging**: Lazy allocation with page fault handling
- **Swap/Backing Store**: Simulated disk storage for paged-out memory
- **Page Replacement Algorithms**:
  - FIFO (First-In-First-Out)
  - LRU (Least Recently Used - exact implementation)
  - Approx-LRU (Aging/NFU algorithm for O(1) replacement)
  - Clock (Second-Chance algorithm)
  - OPT (Optimal/Belady's algorithm for comparison)

### Advanced Features
- **Multi-process Support**: Isolated virtual address spaces per process
- **Comprehensive Metrics**: Page faults, TLB hit/miss ratios, swap I/O, average memory access time
- **Flexible I/O**: JSON, CSV, and console output formats
- **Trace Generation**: Synthetic trace patterns (sequential, random, locality, working set, thrashing)
- **Performance Instrumentation**: Simulated access times and throughput measurement

---

## Web GUI

A modern, interactive web-based visualization interface is available in the `gui/` directory!

**Quick Start GUI:**
```bash
cd gui
python3 -m http.server 8000
# Open http://localhost:8000 in your browser
```

**Features:**
- Real-time visualization of TLB, page tables, and physical frames
- Interactive configuration and simulation controls
- Performance charts and metrics dashboard
- Step-by-step execution with detailed event logging

See [gui/README.md](gui/README.md) for complete GUI documentation.

---

## Quick Start (Command Line)

### Build
```bash
make
```

### Generate Sample Traces
```bash
make traces
```

### Run a Simple Simulation
```bash
./bin/vmm -r 64 -p 4096 -t traces/working_set.trace -a LRU -T 32
```

### Run All Tests
```bash
make test
```

---

## Usage Examples

### Example 1: Small RAM, FIFO Algorithm
```bash
./bin/vmm -r 16 -p 4096 -t traces/sequential.trace -a FIFO -T 16
```
**Expected**: High page fault rate with small RAM

### Example 2: Medium RAM, LRU Algorithm with JSON Output
```bash
./bin/vmm -r 64 -p 4096 -t traces/working_set.trace -a LRU -T 64 \
    -o results/lru_output.json --csv results/lru_metrics.csv
```
**Expected**: Moderate page fault rate, good TLB hit rate with locality

### Example 3: Large RAM, Clock Algorithm, Verbose Mode
```bash
./bin/vmm -r 256 -p 4096 -t traces/locality.trace -a CLOCK -T 128 -V
```
**Expected**: Low page fault rate, detailed per-process statistics

### Example 4: Compare Algorithms
```bash
# Run with different algorithms and compare CSV output
for algo in FIFO LRU CLOCK OPT; do
    ./bin/vmm -r 32 -p 4096 -t traces/random.trace -a $algo -T 32 \
        --csv results/${algo}_comparison.csv --config-name $algo
done
```

---

## Command-Line Options

### Required
- `-t, --trace FILE` - Input trace file (format: `pid op addr`)

### Memory Configuration
- `-r, --ram SIZE` - Physical RAM size in MB (default: 64)
- `-p, --page-size SIZE` - Page size in bytes (default: 4096)
- `-s, --swap SIZE` - Swap size in MB (default: 256)
- `-v, --vspace SIZE` - Virtual address space in MB (default: 4096)

### Algorithms
- `-a, --algorithm ALGO` - Replacement algorithm: FIFO, LRU, APPROX_LRU, CLOCK, OPT (default: CLOCK)
- `-T, --tlb-size SIZE` - TLB entries (default: 64)
- `--tlb-policy POLICY` - TLB policy: FIFO, LRU (default: LRU)
- `--pt-type TYPE` - Page table type: SINGLE, TWO_LEVEL (default: SINGLE)

### Simulation
- `-n, --max-accesses N` - Stop after N memory accesses
- `--seed SEED` - Random seed (default: 42)

### Output
- `-o, --output FILE` - JSON output file
- `--csv FILE` - CSV output file
- `--config-name NAME` - Configuration name for CSV output

### Verbosity
- `-V, --verbose` - Verbose output with per-process metrics
- `-D, --debug` - Debug output with trace-level logging
- `-q, --quiet` - Quiet mode (errors only)

---

## Trace Format

Input trace files are text files with one memory access per line:
```
<pid> <op> <virtual_address>
```

Where:
- `pid` - Process ID (integer)
- `op` - Operation: `R` (read) or `W` (write)
- `virtual_address` - Virtual address in hex (0x...) or decimal

Example:
```
0 R 0x1000
0 W 0x2000
1 R 0x1000
```

---

## Trace Generation

Generate synthetic traces with various patterns:

```bash
# Sequential access pattern
./bin/trace_gen -t sequential -n 10000 -o traces/sequential.trace

# Random access pattern
./bin/trace_gen -t random -n 20000 -p 8 -o traces/random.trace

# Working set with locality
./bin/trace_gen -t working_set -n 15000 -p 4 -o traces/working_set.trace

# Temporal/spatial locality
./bin/trace_gen -t locality -n 12000 -o traces/locality.trace

# Thrashing pattern (pathological)
./bin/trace_gen -t thrashing -n 25000 -o traces/thrashing.trace
```

---

## Output Formats

### Console Summary
```
==================== SIMULATION SUMMARY ====================

Memory Accesses:
  Total:              10000
  Reads:               8000 (80.0%)
  Writes:              2000 (20.0%)

Page Faults:
  Total:                 845
  Fault Rate:           8.45%

TLB Performance:
  Hits:                 9234
  Misses:                766
  Hit Rate:            92.34%

...
```

### JSON Output
Complete metrics including per-process breakdown (use `-o filename.json`)

### CSV Output
Single-line format for spreadsheet analysis (use `--csv filename.csv`)

---

## Build Targets

- `make` or `make all` - Build release version
- `make debug` - Build with debug symbols and sanitizers
- `make release` - Build optimized version
- `make test` - Run test suite
- `make traces` - Generate sample trace files
- `make valgrind` - Run with memory leak checker
- `make format` - Format code with clang-format
- `make clean` - Remove build artifacts
- `make distclean` - Remove all generated files

---

## Architecture

See [DESIGN.md](docs/DESIGN.md) for detailed architecture and design decisions.

See [API.md](docs/API.md) for developer API documentation.

See [TESTPLAN.md](docs/TESTPLAN.md) for test cases and acceptance criteria.

---

## Extending the VMM

The simulator is designed for easy extension:

1. **Add New Replacement Algorithm**: Implement in `src/replacement.c`
2. **Custom Trace Patterns**: Extend `trace_generate()` in `src/trace.c`
3. **Additional Metrics**: Add counters in `src/metrics.h/c`
4. **Memory-Mapped Files**: Extend page fault handler in `src/vmm.c`
5. **Copy-on-Write**: Add COW logic to frame allocator and page fault handler
6. **Shared Memory**: Implement reference counting in `FrameInfo`

See docs/API.md for extension points.

---

## Performance Optimization

The implementation includes several optimizations:

1. **Bitmap for Free Frames**: O(1) frame allocation
2. **Hash-based Page Tables**: For sparse address spaces (in two-level PT)
3. **Approximate LRU**: O(1) replacement using aging counters
4. **Efficient TLB**: Direct search with LRU tracking
5. **Cache-friendly Data Structures**: Contiguous arrays where possible

Compile with optimizations:
```bash
make release
```

---

## Dependencies

- **GCC** or compatible C compiler (C11 standard)
- **GNU Make**
- **bc** (for test arithmetic)
- **clang-format** (optional, for code formatting)
- **valgrind** (optional, for memory leak checking)

---

## Troubleshooting

### Compilation Errors
- Ensure GCC supports C11: `gcc --version`
- Install missing dependencies: `sudo apt-get install build-essential`

### Trace File Errors
- Verify trace format: each line must be `pid op addr`
- Check file permissions: `ls -l traces/`

### Segmentation Faults
- Run with debug build: `make debug && ./bin/vmm_debug ...`
- Use valgrind: `make valgrind`
- Enable debug logging: `./bin/vmm ... -D`

---

## License

This project is released for educational and research purposes.

---

## Authors

**Development Team:**
- **Aditya Pandey** - Core VMM implementation, memory management subsystems
- **Kartik** - Page table implementation, replacement algorithms
- **Vivek** - TLB simulation, metrics collection and reporting
- **Gaurang** - Trace generation, testing framework, documentation

This project was collaboratively developed as a comprehensive teaching/learning tool for operating systems courses and virtual memory research.

---

## References

- *Operating System Concepts* by Silberschatz, Galvin, and Gagne
- *Modern Operating Systems* by Andrew S. Tanenbaum
- Linux kernel memory management documentation

