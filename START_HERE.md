# 🚀 VMM Simulator - START HERE

**Authors:** Aditya Pandey, Kartik, Vivek, Gaurang

---

## Welcome to the Virtual Memory Manager Simulator!

This project provides **TWO** ways to explore virtual memory management:

1. **🌐 Web GUI** (Recommended - No compilation needed!)
2. **💻 Command Line** (For advanced users)

---

## 🌐 Option 1: Web GUI (EASIEST - Try This First!)

### Step 1: Open Terminal
Navigate to the VMM project directory:
```bash
cd /Users/adityapandey/Desktop/VMM
```

### Step 2: Start the Web Server
```bash
cd gui
python3 -m http.server 8000
```

You should see:
```
Serving HTTP on 0.0.0.0 port 8000 (http://0.0.0.0:8000/) ...
```

### Step 3: Open Your Browser
Open your web browser and go to:
```
http://localhost:8000
```

### Step 4: Use the Simulator!

The web interface provides:
- ✅ **Real-time visualization** of TLB, page tables, and memory frames
- ✅ **Interactive controls** to configure and run simulations
- ✅ **Performance charts** showing metrics over time
- ✅ **Event log** with detailed access history
- ✅ **No compilation required!**

#### Quick Tutorial:
1. Configure RAM size (e.g., 64 MB) and algorithm (e.g., LRU)
2. Click **"Generate Trace"** to create memory access pattern
3. Click **"Run Simulation"** to watch it execute
4. Observe the visualizations update in real-time!
5. Check metrics and charts for performance analysis

**That's it! No coding or compilation needed!**

---

## 💻 Option 2: Command Line Interface

### Step 1: Build the Project
```bash
cd /Users/adityapandey/Desktop/VMM
make
```

This compiles all the C code. You should see:
```
Compiling src/main.c...
Compiling src/vmm.c...
...
Linking bin/vmm
```

### Step 2: Generate Sample Traces
```bash
make traces
```

This creates sample memory access traces in the `traces/` directory.

### Step 3: Run a Simulation
```bash
./bin/vmm -r 64 -p 4096 -t traces/working_set.trace -a LRU -T 64
```

**Parameters explained:**
- `-r 64` - 64 MB of RAM
- `-p 4096` - 4KB page size
- `-t traces/working_set.trace` - Input trace file
- `-a LRU` - Use LRU replacement algorithm
- `-T 64` - 64 TLB entries

### Step 4: View Results
The simulator outputs detailed metrics to the console.

---

## 📊 What You'll Learn

### Core Concepts
- How virtual addresses are translated to physical addresses
- How the TLB speeds up address translation
- How page faults occur and are handled
- How different replacement algorithms perform (FIFO, LRU, Clock)
- How memory thrashing happens and how to avoid it

### Hands-on Experiments
1. **Compare algorithms**: Run same trace with different algorithms
2. **TLB impact**: Try small vs large TLB sizes
3. **Memory pressure**: See what happens with limited RAM
4. **Access patterns**: Compare sequential vs random access
5. **Thrashing**: Force pathological behavior and observe

---

## 📚 Documentation Quick Links

- **[QUICKSTART.md](QUICKSTART.md)** - Detailed 5-minute guide
- **[README.md](README.md)** - Complete project documentation
- **[gui/README.md](gui/README.md)** - Web GUI user guide
- **[PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)** - Project overview
- **[AUTHORS.md](AUTHORS.md)** - Team contributions

**For Developers:**
- **[docs/DESIGN.md](docs/DESIGN.md)** - Architecture and design
- **[docs/API.md](docs/API.md)** - Developer API reference
- **[docs/TESTPLAN.md](docs/TESTPLAN.md)** - Testing guide
- **[EXTENSIONS.md](EXTENSIONS.md)** - Future enhancements

---

## 🎯 Recommended Path

### For First-Time Users:
1. ✅ Start with **Web GUI** (easiest)
2. ✅ Read **QUICKSTART.md** for examples
3. ✅ Experiment with different configurations
4. ✅ Try all trace patterns and algorithms
5. ✅ Then explore command-line interface

### For Developers:
1. ✅ Read **DESIGN.md** for architecture
2. ✅ Review **API.md** for extension points
3. ✅ Study source code in `src/` directory
4. ✅ Run tests with `make test`
5. ✅ Check **EXTENSIONS.md** for ideas

---

## 🐛 Troubleshooting

### Web GUI doesn't load
- **Problem:** Port 8000 already in use
- **Solution:** Try a different port: `python3 -m http.server 8080`

### Build fails
- **Problem:** GCC not installed
- **Solution:** Install build tools: `xcode-select --install` (macOS)

### Traces not found
- **Problem:** Trace files not generated
- **Solution:** Run `make traces` first

### Permission denied
- **Problem:** Script not executable
- **Solution:** `chmod +x tests/run_tests.sh`

---

## 💡 Example Sessions

### Session 1: First Simulation (Web GUI)
1. Start web server: `cd gui && python3 -m http.server 8000`
2. Open http://localhost:8000
3. Select "Working Set" trace pattern
4. Click "Generate Trace"
5. Click "Run Simulation"
6. Watch TLB and frames update in real-time!

**Time:** 2 minutes

### Session 2: Algorithm Comparison (CLI)
```bash
# Build and generate traces
make && make traces

# Run with FIFO
./bin/vmm -r 32 -t traces/random.trace -a FIFO --csv fifo.csv

# Run with LRU
./bin/vmm -r 32 -t traces/random.trace -a LRU --csv lru.csv

# Compare results
# LRU should have fewer page faults!
```

**Time:** 5 minutes

### Session 3: Explore Thrashing (Web GUI)
1. Configure: 8 MB RAM (very small)
2. Select "Thrashing" pattern
3. Generate and run
4. Observe: Very high page fault rate (>30%)
5. Reset and try with 64 MB RAM
6. Observe: Dramatically lower fault rate

**Time:** 3 minutes

---

## ✨ Features Highlight

### What Makes This Special:

1. **🎨 Beautiful Web Interface**
   - Modern, responsive design
   - Real-time visualization
   - Interactive controls
   - No installation needed!

2. **🔬 Comprehensive Simulation**
   - 5 replacement algorithms
   - Multiple trace patterns
   - Realistic timing simulation
   - Detailed metrics

3. **📖 Excellent Documentation**
   - 2000+ lines of docs
   - Clear examples
   - Architecture diagrams
   - API reference

4. **🧪 Thorough Testing**
   - 10 automated tests
   - Sample traces included
   - Validation scripts
   - Expected outputs documented

5. **🚀 Production Quality**
   - Clean, modular code
   - Zero memory leaks
   - Comprehensive error handling
   - Professional build system

---

## 🎓 Perfect For:

- ✅ **Students** learning operating systems
- ✅ **Educators** teaching VM concepts
- ✅ **Researchers** comparing algorithms
- ✅ **Self-learners** exploring OS internals
- ✅ **Developers** understanding system performance

---

## 👥 Credits

**Developed by:**
- **Aditya Pandey** - Core VMM, frame allocator, swap manager
- **Kartik** - Page tables, replacement algorithms
- **Vivek** - TLB simulation, metrics system
- **Gaurang** - Trace engine, testing, documentation

**All team members** contributed to the web GUI and visualization.

---

## 🎉 Ready to Start?

### Quick Start Web GUI:
```bash
cd gui
python3 -m http.server 8000
# Open http://localhost:8000
```

### Quick Start CLI:
```bash
make
make traces
./bin/vmm -t traces/working_set.trace -a LRU
```

---

**Questions? Check the documentation or experiment with the GUI!**

**Enjoy exploring virtual memory! 🎊**

