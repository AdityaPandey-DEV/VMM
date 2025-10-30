# VMM Simulator - Quick Start Guide

**Authors:** Aditya Pandey, Kartik, Vivek, Gaurang

This guide will get you up and running with the VMM Simulator in 5 minutes!

---

## Option 1: Web GUI (Recommended for Beginners)

### Step 1: Navigate to GUI directory
```bash
cd gui
```

### Step 2: Start web server
```bash
python3 -m http.server 8000
```

### Step 3: Open browser
Navigate to: **http://localhost:8000**

### Step 4: Use the GUI
1. Configure RAM size and algorithm in the configuration panel
2. Click "Generate Trace" to create a memory access pattern
3. Click "Run Simulation" to watch the visualization in real-time
4. Observe TLB, page table, and frame visualizations update live!

**That's it!** No compilation needed.

---

## Option 2: Command Line (For Advanced Users)

### Step 1: Build the project
```bash
make
```

### Step 2: Generate sample traces
```bash
make traces
```

### Step 3: Run your first simulation
```bash
./bin/vmm -r 64 -p 4096 -t traces/working_set.trace -a LRU -T 64
```

### Step 4: View results
The simulation will output performance metrics to the console:
- Page faults and fault rate
- TLB hits/misses
- Swap I/O statistics
- Average memory access time

---

## Sample Commands

### Small RAM with FIFO
```bash
./bin/vmm -r 16 -p 4096 -t traces/sequential.trace -a FIFO -T 16
```
**Expected:** High page fault rate with limited memory

### Large RAM with LRU
```bash
./bin/vmm -r 128 -p 4096 -t traces/locality.trace -a LRU -T 128
```
**Expected:** Low fault rate, excellent TLB hit rate

### Compare algorithms
```bash
# Run with each algorithm
for algo in FIFO LRU CLOCK OPT; do
    ./bin/vmm -r 32 -p 4096 -t traces/random.trace -a $algo -T 32 \
        --csv results_$algo.csv --config-name $algo
done
```
**Result:** CSV files for each algorithm that you can compare

---

## Understanding the Output

### Console Output
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
  Fault Rate:           3.24%   ← Lower is better

TLB Performance:
  Hits:                 9456
  Misses:                544
  Hit Rate:            94.56%   ← Higher is better

Swap I/O:
  Swap-ins:               42
  Swap-outs:              38
  Replacements:          324

Average Memory Access Time:
  AMT:               145.32 ns  ← Lower is better
```

### What These Numbers Mean

- **Fault Rate < 5%**: Excellent - your RAM is sufficient
- **Fault Rate 5-20%**: Good - reasonable performance
- **Fault Rate > 20%**: Poor - thrashing likely occurring

- **TLB Hit Rate > 90%**: Excellent locality
- **TLB Hit Rate 70-90%**: Good
- **TLB Hit Rate < 70%**: Poor locality or TLB too small

---

## Running Tests

### Run all automated tests
```bash
make test
```

This runs 10 comprehensive tests including:
- Algorithm correctness
- TLB effectiveness
- Multi-process handling
- Thrashing scenarios

---

## Next Steps

### Learn More
- Read [README.md](README.md) for complete feature list
- Check [DESIGN.md](docs/DESIGN.md) for architecture details
- See [API.md](docs/API.md) to extend the simulator

### Experiment
1. Try different trace patterns (sequential, random, locality)
2. Compare algorithms with the same trace
3. Vary RAM size and see impact on performance
4. Test different page sizes (4KB vs 8KB vs 16KB)

### Customize
1. Generate your own traces with `./bin/trace_gen`
2. Create custom trace files (format: `pid op addr`)
3. Modify replacement algorithms in `src/replacement.c`
4. Add new metrics in `src/metrics.c`

---

## Troubleshooting

### Build fails
```bash
# Ensure you have GCC installed
gcc --version

# Clean and rebuild
make clean
make
```

### No traces found
```bash
# Generate them
make traces
```

### GUI doesn't work
- Try a different browser (Chrome recommended)
- Check if port 8000 is available
- Use a different port: `python3 -m http.server 8080`

---

## Getting Help

1. Check [TESTPLAN.md](docs/TESTPLAN.md) for expected behavior
2. Read error messages carefully
3. Enable debug mode: `./bin/vmm ... -D`
4. Run with valgrind: `make valgrind`

---

## Credits

Developed by: **Aditya Pandey, Kartik, Vivek, Gaurang**

For educational and research purposes.

---

**Enjoy exploring virtual memory management!** 🎉

