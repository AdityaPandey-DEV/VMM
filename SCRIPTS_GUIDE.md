# VMM Simulator - Scripts Guide

Quick reference for all automation scripts included in the project.

---

## 🚀 Start Script

### **`./start.sh`** or **`./START`**
The main launcher for the VMM simulator - handles everything automatically!

### Usage

#### Interactive Mode (Recommended for beginners)
```bash
./start.sh
```
Shows a menu with options:
1. 🌐 Start Web GUI
2. 💻 Run CLI with default settings  
3. ⚙️ Run CLI with custom settings (interactive)
4. 🏃 Run performance benchmarks
5. 🧪 Run test suite
6. 📚 Show quick examples
7. 🛠️ Build only (no run)
8. 📖 Open documentation
9. ❌ Exit

#### Command Line Mode
```bash
# Start web GUI
./start.sh gui

# Run CLI simulation
./start.sh cli [ram_mb] [algorithm] [trace_pattern] [tlb_size]
./start.sh cli 128 LRU working_set 64

# Run benchmarks
./start.sh benchmark

# Run tests
./start.sh test

# Build only
./start.sh build

# Show help
./start.sh help
```

### Features
✅ Auto-detects if build is needed  
✅ Generates traces if missing  
✅ Colored output for better readability  
✅ Interactive configuration  
✅ Built-in examples and documentation  

---

## 🛑 Stop Script

### **`./stop.sh`** or **`./STOP`**
Cleanly stops all running VMM processes.

### Usage

#### Stop Everything (Default)
```bash
./stop.sh
```

#### Selective Stopping
```bash
# Stop web server only
./stop.sh --web

# Stop VMM simulations only
./stop.sh --vmm

# Stop trace generators only
./stop.sh --trace

# Stop all and clean output files
./stop.sh --all --clean-output
```

#### Check Status
```bash
./stop.sh --status
```

### What It Stops
- 🌐 Web servers running on port 8000 (GUI)
- 💻 VMM simulation processes (`bin/vmm`)
- 📊 Trace generator processes (`bin/trace_gen`)

### Cleanup Options
```bash
# Also remove output files
./stop.sh --clean-output
```

Removes:
- `output/*.json`, `output/*.csv`, `output/*.log`
- `benchmarks/*.csv`, `benchmarks/*.json`
- Core dumps (`core.*`)
- Swap files (`*.swp`)

---

## 🔄 Typical Workflows

### Workflow 1: Quick Start (Web GUI)
```bash
./start.sh gui
# Opens browser to http://localhost:8000

# When done:
./stop.sh
```

### Workflow 2: CLI Experiments
```bash
# Start with defaults
./start.sh cli

# Try different algorithms
./start.sh cli 64 FIFO random 32
./start.sh cli 64 LRU random 32
./start.sh cli 64 CLOCK random 32

# Compare results
cat output/*.log
```

### Workflow 3: Performance Benchmarking
```bash
# Run all algorithms
./start.sh benchmark

# Results in benchmarks/*.csv
ls -lh benchmarks/

# View results
cat benchmarks/LRU_results.csv
```

### Workflow 4: Development & Testing
```bash
# Build and test
./start.sh build
./start.sh test

# If tests fail, check output
cat tests/output/*.log
```

### Workflow 5: Multiple Sessions
```bash
# Terminal 1: Web GUI
./start.sh gui

# Terminal 2: Run benchmarks while GUI is open
cd /path/to/VMM
./start.sh benchmark

# Terminal 3: Monitor
./stop.sh --status

# When done, stop everything
./stop.sh --all
```

---

## 🎯 Quick Command Reference

| Task | Command |
|------|---------|
| Start GUI | `./start.sh gui` or `./START gui` |
| Run simulation | `./start.sh cli` |
| Run benchmarks | `./start.sh benchmark` |
| Run tests | `./start.sh test` |
| Stop everything | `./stop.sh` or `./STOP` |
| Check status | `./stop.sh --status` |
| Build project | `./start.sh build` or `make` |
| Clean build | `make clean && make` |
| Help | `./start.sh help` |

---

## 📝 Script Features

### Start Script Features
- ✅ Automatic dependency checking
- ✅ Smart build detection
- ✅ Trace auto-generation
- ✅ Interactive configuration wizard
- ✅ Beautiful colored output
- ✅ Built-in examples
- ✅ Documentation integration
- ✅ Error handling

### Stop Script Features
- ✅ Process detection on port 8000
- ✅ Graceful shutdown (SIGTERM first)
- ✅ Force kill if needed (SIGKILL)
- ✅ Selective stopping (web/vmm/trace)
- ✅ Temporary file cleanup
- ✅ Status checking
- ✅ Confirmation of stopped processes

---

## 🐛 Troubleshooting

### "Permission denied" Error
```bash
# Make scripts executable
chmod +x start.sh stop.sh START STOP
```

### "Port 8000 already in use"
```bash
# Stop existing server
./stop.sh --web

# Or use different port (edit gui launcher in start.sh)
```

### "Executables not found"
```bash
# Force rebuild
./start.sh build
# or
make clean && make
```

### "Trace files missing"
```bash
# Generate traces
make traces
# or
./bin/trace_gen -t working_set -n 10000 -o traces/working_set.trace
```

### Can't stop a process
```bash
# Check what's running
./stop.sh --status

# Force stop everything
./stop.sh --all

# Manual kill if needed
lsof -ti:8000 | xargs kill -9
pkill -9 -f bin/vmm
```

---

## 💡 Pro Tips

### Tip 1: Quick Test Different Algorithms
```bash
for algo in FIFO LRU CLOCK OPT; do
    ./start.sh cli 64 $algo working_set 64 > results_$algo.txt
done
```

### Tip 2: Background GUI
```bash
# Run GUI in background
nohup ./start.sh gui > /dev/null 2>&1 &

# Later, stop it
./stop.sh --web
```

### Tip 3: Watch Processes
```bash
# In one terminal
watch -n 1 './stop.sh --status'

# In another, run experiments
./start.sh benchmark
```

### Tip 4: Automated Testing
```bash
#!/bin/bash
./start.sh build
./start.sh test
if [ $? -eq 0 ]; then
    echo "✅ Tests passed!"
    ./start.sh benchmark
else
    echo "❌ Tests failed!"
    exit 1
fi
```

---

## 🎓 Learning Path with Scripts

### Week 1: Getting Started
```bash
# Day 1: GUI exploration
./start.sh gui

# Day 2-3: CLI basics
./start.sh cli 32 FIFO sequential 16
./start.sh cli 32 LRU sequential 16

# Day 4-5: Try different traces
./start.sh cli 64 LRU random 32
./start.sh cli 64 LRU working_set 32
./start.sh cli 64 LRU locality 32
```

### Week 2: Experimentation
```bash
# Compare algorithms
./start.sh benchmark

# Custom traces
./bin/trace_gen -t random -n 5000 -o my_trace.trace
./start.sh cli 64 LRU my_trace 32
```

### Week 3: Advanced
```bash
# Modify code, rebuild, test
vim src/vmm.c
./start.sh build
./start.sh test

# Run specific configurations
./bin/vmm --help
```

---

## 📞 Need Help?

1. **Start script help:** `./start.sh help`
2. **Stop script help:** `./stop.sh --help`
3. **Makefile targets:** `make help`
4. **VMM options:** `./bin/vmm --help`
5. **Documentation:** `./start.sh` → Option 8

---

**Created by:** Aditya Pandey, Kartik, Vivek, Gaurang

**Happy simulating! 🚀**

