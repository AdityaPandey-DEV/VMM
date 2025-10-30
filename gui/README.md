# VMM Simulator - Web GUI

**Authors:** Aditya Pandey, Kartik, Vivek, Gaurang

---

## Overview

Interactive web-based visualization tool for the Virtual Memory Manager simulator. Provides real-time visualization of:
- Translation Lookaside Buffer (TLB) state
- Page table entries
- Physical memory frames
- Address translation pipeline
- Performance metrics and charts

---

## Quick Start

### Option 1: Simple HTTP Server (Python)

```bash
cd gui
python3 -m http.server 8000
```

Then open: http://localhost:8000

### Option 2: Node.js HTTP Server

```bash
cd gui
npx http-server -p 8000
```

Then open: http://localhost:8000

### Option 3: Direct File Opening

Simply open `index.html` in a modern web browser (Chrome, Firefox, Edge).

---

## Features

### 1. Configuration Panel
- Adjust RAM size, page size, TLB size
- Select replacement algorithm (FIFO, LRU, Clock, etc.)
- Choose trace pattern (sequential, random, working set, locality, thrashing)
- Set number of memory accesses

### 2. Real-time Visualization
- **TLB Grid**: Shows all TLB entries with current mappings
- **Page Table**: Displays active page table entries
- **Physical Frames**: Color-coded frame allocation status
- **Pipeline View**: Shows address translation flow in real-time

### 3. Performance Metrics
- Total accesses, reads, writes
- Page faults and fault rate
- TLB hits/misses and hit rate
- Swap operations
- Live charts for fault rate and TLB performance

### 4. Event Log
- Timestamped log of all memory accesses
- Color-coded events (hits in green, faults in red)
- Detailed information about each access

### 5. Simulation Controls
- **Generate Trace**: Create synthetic memory access trace
- **Run**: Execute simulation with animation
- **Step**: Execute one access at a time
- **Reset**: Clear simulation state

---

## How to Use

### Basic Workflow

1. **Configure the simulator**
   - Set RAM size (e.g., 64 MB)
   - Choose algorithm (e.g., LRU)
   - Select trace pattern (e.g., working_set)

2. **Generate trace**
   - Click "Generate Trace" button
   - Trace will be created based on your configuration

3. **Run simulation**
   - Click "Run Simulation" for automatic execution
   - OR click "Step" to execute one access at a time
   - Watch visualizations update in real-time

4. **Analyze results**
   - View metrics in the dashboard
   - Check charts for trends
   - Review event log for details

### Example Scenarios

#### Scenario 1: Compare Algorithms
1. Configure: 32 MB RAM, working_set trace, 5000 accesses
2. Run with FIFO algorithm, note metrics
3. Reset
4. Run with LRU algorithm, compare results

#### Scenario 2: TLB Size Impact
1. Configure: 64 MB RAM, locality trace
2. Run with TLB size = 16, observe hit rate
3. Reset
4. Run with TLB size = 128, compare hit rate improvement

#### Scenario 3: Thrashing Demonstration
1. Configure: 8 MB RAM (small), thrashing trace, 10000 accesses
2. Run and observe high page fault rate
3. Reset with 64 MB RAM
4. Run again and see improvement

---

## Visualization Legend

### TLB Entries
- **Gray**: Empty/invalid entry
- **Blue**: Valid translation
- **Green**: Recently accessed (animated pulse)

### Page Table Entries
- **Gray**: Unmapped
- **Blue**: Valid mapping
- **Yellow**: Dirty (modified)
- **Green**: Recently accessed

### Physical Frames
- **Gray**: Free frame
- **Blue**: Allocated frame
- **Red**: Frame being evicted (animated pulse)

### Pipeline Stages
- **Green ✓**: Success (hit)
- **Red ✗**: Miss or fault

---

## Technical Details

### Files
- `index.html`: Main UI structure
- `style.css`: Modern, responsive styling
- `simulator.js`: Core VMM simulation logic
- `app.js`: UI controller and visualization updates

### Technologies
- Pure JavaScript (no framework dependencies)
- Chart.js for performance graphs
- CSS Grid and Flexbox for responsive layout
- CSS animations for visual feedback

### Browser Compatibility
- Chrome/Edge: ✓ Fully supported
- Firefox: ✓ Fully supported
- Safari: ✓ Fully supported
- Mobile browsers: ✓ Responsive design

---

## Customization

### Modify Trace Patterns

Edit `simulator.js`, function `generateTrace()`:

```javascript
case 'custom_pattern':
    for (let i = 0; i < numAccesses; i++) {
        this.trace.push({
            pid: /* your logic */,
            op: /* 'R' or 'W' */,
            vpn: /* virtual page number */
        });
    }
    break;
```

### Add New Metrics

1. Add counter to `metrics` object in `simulator.js`
2. Update HTML in `index.html` to display new metric
3. Update `updateVisualization()` in `app.js` to populate value

### Customize Colors

Edit CSS variables in `style.css`:

```css
:root {
    --primary-color: #your-color;
    --success-color: #your-color;
    /* ... */
}
```

---

## Integration with C Backend

To connect the GUI with the actual C simulator:

### Option 1: WebAssembly
Compile VMM to WebAssembly and call directly from JavaScript.

### Option 2: REST API
Create a simple HTTP server in C that exposes VMM functionality:
```c
// Pseudo-code
POST /api/configure - Set VMM config
POST /api/trace/generate - Generate trace
POST /api/simulate/step - Execute one step
GET /api/metrics - Get current metrics
```

### Option 3: File-based Communication
- Generate trace in GUI, save to file
- Run C simulator with trace file
- Load JSON results back into GUI

---

## Performance

The GUI can comfortably handle:
- Up to 100,000 memory accesses (slower animation recommended)
- Up to 1024 physical frames
- Up to 256 TLB entries
- Real-time updates at 10 steps/second

For very large simulations, consider:
- Batch mode (run complete simulation, then show results)
- Sampling (update visualization every N steps)

---

## Troubleshooting

### Charts not displaying
- Ensure Chart.js CDN is accessible
- Check browser console for errors

### Slow performance
- Reduce number of accesses
- Increase animation interval in `app.js` (change 100ms to 500ms)

### Visualizations not updating
- Check browser console for JavaScript errors
- Ensure you clicked "Generate Trace" before "Run Simulation"

---

## Future Enhancements

- [ ] Export simulation results as CSV
- [ ] Import custom trace files
- [ ] Compare multiple algorithm runs side-by-side
- [ ] 3D visualization of memory hierarchy
- [ ] Replay and seek functionality
- [ ] Heat map for page access frequency

---

## Contributing

To add features:
1. Fork the repository
2. Create feature branch
3. Test in multiple browsers
4. Submit pull request

---

## License

Educational use only. Part of the VMM Simulator project.

---

## Authors

This GUI was collaboratively designed and developed by:
- **Aditya Pandey** - Core visualization and simulation logic
- **Kartik** - UI design and styling
- **Vivek** - Metrics and charting
- **Gaurang** - Event logging and testing

For questions or issues, please contact the development team.

