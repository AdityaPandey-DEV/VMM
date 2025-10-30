/**
 * VMM Simulator - UI Controller
 * Authors: Aditya Pandey, Kartik, Vivek, Gaurang
 */

let simulator = null;
let isRunning = false;
let animationInterval = null;

// Charts
let faultChart = null;
let tlbChart = null;

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    initializeCharts();
    setupEventListeners();
    createInitialVisualization();
});

function setupEventListeners() {
    document.getElementById('generate-trace-btn').addEventListener('click', generateTrace);
    document.getElementById('run-simulation-btn').addEventListener('click', toggleSimulation);
    document.getElementById('step-btn').addEventListener('click', stepSimulation);
    document.getElementById('reset-btn').addEventListener('click', resetSimulation);
}

function getConfig() {
    return {
        ramSize: parseInt(document.getElementById('ram-size').value),
        pageSize: parseInt(document.getElementById('page-size').value),
        tlbSize: parseInt(document.getElementById('tlb-size').value),
        algorithm: document.getElementById('algorithm').value,
        pattern: document.getElementById('trace-pattern').value,
        numAccesses: parseInt(document.getElementById('num-accesses').value)
    };
}

function generateTrace() {
    const config = getConfig();
    simulator = new VMMSimulator(config);
    simulator.generateTrace(config.pattern, config.numAccesses);
    
    logEvent(`Generated ${config.pattern} trace with ${config.numAccesses} accesses`);
    updateVisualization();
    addToast('Trace generated successfully!', 'success');
}

function toggleSimulation() {
    if (!simulator || simulator.trace.length === 0) {
        addToast('Please generate a trace first!', 'warning');
        return;
    }
    
    const btn = document.getElementById('run-simulation-btn');
    
    if (isRunning) {
        isRunning = false;
        clearInterval(animationInterval);
        btn.textContent = 'Run Simulation';
        btn.classList.remove('btn-danger');
        btn.classList.add('btn-success');
    } else {
        isRunning = true;
        btn.textContent = 'Pause';
        btn.classList.remove('btn-success');
        btn.classList.add('btn-danger');
        
        animationInterval = setInterval(() => {
            const result = simulator.step();
            if (result.done) {
                toggleSimulation();
                addToast('Simulation completed!', 'success');
            } else {
                updateVisualization(result);
            }
        }, 100); // 100ms per step
    }
}

function stepSimulation() {
    if (!simulator || simulator.trace.length === 0) {
        addToast('Please generate a trace first!', 'warning');
        return;
    }
    
    const result = simulator.step();
    if (result.done) {
        addToast('Trace completed!', 'info');
    } else {
        updateVisualization(result);
    }
}

function resetSimulation() {
    if (simulator) {
        simulator.reset();
    }
    isRunning = false;
    clearInterval(animationInterval);
    
    document.getElementById('run-simulation-btn').textContent = 'Run Simulation';
    document.getElementById('run-simulation-btn').classList.remove('btn-danger');
    document.getElementById('run-simulation-btn').classList.add('btn-success');
    
    updateVisualization();
    clearLog();
    resetCharts();
    
    addToast('Simulation reset', 'info');
}

function updateVisualization(result = null) {
    if (!simulator) return;
    
    // Update metrics
    const metrics = simulator.getMetrics();
    document.getElementById('stat-accesses').textContent = metrics.totalAccesses;
    document.getElementById('stat-faults').textContent = metrics.pageFaults;
    document.getElementById('stat-fault-rate').textContent = metrics.faultRate + '%';
    document.getElementById('stat-tlb-hits').textContent = metrics.tlbHits;
    document.getElementById('stat-tlb-misses').textContent = metrics.tlbMisses;
    document.getElementById('stat-tlb-rate').textContent = metrics.tlbHitRate + '%';
    document.getElementById('stat-swap-ins').textContent = metrics.swapIns;
    document.getElementById('stat-swap-outs').textContent = metrics.swapOuts;
    
    // Update TLB visualization
    updateTLB();
    
    // Update page table visualization
    updatePageTable();
    
    // Update frames visualization
    updateFrames(result);
    
    // Update pipeline
    if (result && result.entry) {
        updatePipeline(result);
    }
    
    // Update charts
    updateCharts(metrics);
    
    // Log event
    if (result && result.entry) {
        const entry = result.entry;
        const pfnStr = result.result && result.result.pfn >= 0 ? result.result.pfn : 'N/A';
        let msg = `PID${entry.pid} ${entry.op} VPN:${entry.vpn} → `;
        if (result.result && result.result.tlbHit) {
            msg += `TLB HIT (PFN:${pfnStr})`;
            logEvent(msg, 'hit');
        } else if (result.result && result.result.pageFault) {
            msg += `PAGE FAULT (PFN:${pfnStr})`;
            if (result.result.evicted) {
                msg += ` [Evicted PID${result.result.evicted.pid} VPN:${result.result.evicted.vpn}]`;
            }
            logEvent(msg, 'fault');
        } else {
            msg += `PT HIT (PFN:${pfnStr})`;
            logEvent(msg);
        }
    }
}

function updateTLB() {
    const grid = document.getElementById('tlb-grid');
    grid.innerHTML = '';
    
    simulator.tlb.forEach((entry, i) => {
        const div = document.createElement('div');
        div.className = 'tlb-entry' + (entry.valid ? ' valid' : '');
        div.innerHTML = `
            <div>${i}</div>
            ${entry.valid ? `<div>P${entry.pid}:${entry.vpn}→${entry.pfn}</div>` : '<div>-</div>'}
        `;
        grid.appendChild(div);
    });
}

function updatePageTable() {
    const grid = document.getElementById('page-table-grid');
    grid.innerHTML = '';
    
    // Show first 64 PTEs across all processes
    let count = 0;
    for (let pid in simulator.pageTable) {
        for (let vpn in simulator.pageTable[pid]) {
            if (count >= 64) break;
            const pte = simulator.pageTable[pid][vpn];
            const div = document.createElement('div');
            div.className = 'pte' + 
                (pte.valid ? ' valid' : '') +
                (pte.dirty ? ' dirty' : '') +
                (pte.accessed ? ' accessed' : '');
            div.innerHTML = `<div>P${pid}:${vpn}</div><div>${pte.pfn}</div>`;
            div.title = `PID:${pid} VPN:${vpn} → PFN:${pte.pfn}`;
            grid.appendChild(div);
            count++;
        }
        if (count >= 64) break;
    }
}

function updateFrames(result) {
    if (!simulator || !simulator.frames) {
        console.log('No simulator or frames available');
        return;
    }
    
    const grid = document.getElementById('frame-grid');
    grid.innerHTML = '';
    
    // Collect allocated frames to show them preferentially
    const allocatedFrames = [];
    const freeFrames = [];
    
    for (let i = 0; i < simulator.frames.length; i++) {
        if (!simulator.frames[i].free) {
            allocatedFrames.push(i);
        } else if (freeFrames.length < 128) {
            freeFrames.push(i);
        }
    }
    
    // Show allocated frames first, then some free frames (max 256 total)
    const framesToShow = [...allocatedFrames, ...freeFrames].slice(0, 256);
    
    console.log(`Showing ${framesToShow.length} frames (${allocatedFrames.length} allocated, ${framesToShow.length - allocatedFrames.length} free) out of ${simulator.frames.length} total`);
    
    for (let i of framesToShow) {
        const frame = simulator.frames[i];
        const div = document.createElement('div');
        div.className = 'frame' + 
            (frame.free ? ' free' : ' allocated');
        
        if (result && result.result && result.result.evicted && result.result.evicted.pfn === i) {
            div.classList.add('victim');
        }
        
        div.textContent = frame.free ? '-' : `P${frame.pid}`;
        div.title = frame.free ? `Frame ${i}: Free` : 
            `Frame ${i}: PID ${frame.pid}, VPN ${frame.vpn}${frame.dirty ? ' (D)' : ''}`;
        grid.appendChild(div);
    }
}

function updatePipeline(result) {
    if (!result || !result.entry || !result.result) return;
    
    const entry = result.entry;
    const accessResult = result.result;
    const vaddr = entry.vpn * simulator.config.pageSize;
    const paddr = accessResult.pfn >= 0 ? accessResult.pfn * simulator.config.pageSize : 0;
    
    document.getElementById('stage-virtual').querySelector('.stage-content').textContent = 
        `0x${vaddr.toString(16).padStart(8, '0')}`;
    
    const tlbStage = document.getElementById('stage-tlb').querySelector('.stage-content');
    tlbStage.textContent = accessResult.tlbHit ? 'HIT ✓' : 'MISS ✗';
    tlbStage.style.color = accessResult.tlbHit ? '#10b981' : '#ef4444';
    
    const ptStage = document.getElementById('stage-pagetable').querySelector('.stage-content');
    ptStage.textContent = accessResult.pageFault ? 'FAULT ✗' : 'HIT ✓';
    ptStage.style.color = accessResult.pageFault ? '#ef4444' : '#10b981';
    
    document.getElementById('stage-physical').querySelector('.stage-content').textContent = 
        `0x${paddr.toString(16).padStart(8, '0')}`;
}

function createInitialVisualization() {
    // Create empty grids
    const tlbGrid = document.getElementById('tlb-grid');
    for (let i = 0; i < 64; i++) {
        const div = document.createElement('div');
        div.className = 'tlb-entry';
        div.innerHTML = `<div>${i}</div><div>-</div>`;
        tlbGrid.appendChild(div);
    }
}

function initializeCharts() {
    const faultCtx = document.getElementById('fault-chart').getContext('2d');
    faultChart = new Chart(faultCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Page Fault Rate (%)',
                data: [],
                borderColor: '#ef4444',
                backgroundColor: 'rgba(239, 68, 68, 0.1)',
                tension: 0.4
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: { beginAtZero: true, max: 100 }
            }
        }
    });
    
    const tlbCtx = document.getElementById('tlb-chart').getContext('2d');
    tlbChart = new Chart(tlbCtx, {
        type: 'doughnut',
        data: {
            labels: ['Hits', 'Misses'],
            datasets: [{
                data: [0, 0],
                backgroundColor: ['#10b981', '#ef4444']
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false
        }
    });
}

function updateCharts(metrics) {
    // Update fault rate chart
    if (faultChart.data.labels.length > 50) {
        faultChart.data.labels.shift();
        faultChart.data.datasets[0].data.shift();
    }
    faultChart.data.labels.push(metrics.totalAccesses);
    faultChart.data.datasets[0].data.push(parseFloat(metrics.faultRate));
    faultChart.update('none');
    
    // Update TLB chart
    tlbChart.data.datasets[0].data = [metrics.tlbHits, metrics.tlbMisses];
    tlbChart.update('none');
}

function resetCharts() {
    faultChart.data.labels = [];
    faultChart.data.datasets[0].data = [];
    faultChart.update();
    
    tlbChart.data.datasets[0].data = [0, 0];
    tlbChart.update();
}

function logEvent(message, type = 'info') {
    const log = document.getElementById('event-log');
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    const timestamp = new Date().toLocaleTimeString();
    entry.textContent = `[${timestamp}] ${message}`;
    log.insertBefore(entry, log.firstChild);
    
    // Keep only last 100 entries
    while (log.children.length > 100) {
        log.removeChild(log.lastChild);
    }
}

function clearLog() {
    document.getElementById('event-log').innerHTML = '';
}

function addToast(message, type = 'info') {
    // Simple toast notification
    const toast = document.createElement('div');
    toast.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        background: ${type === 'success' ? '#10b981' : type === 'warning' ? '#f59e0b' : '#2563eb'};
        color: white;
        padding: 1rem 2rem;
        border-radius: 8px;
        box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        z-index: 1000;
        animation: slideIn 0.3s ease-out;
    `;
    toast.textContent = message;
    document.body.appendChild(toast);
    
    setTimeout(() => {
        toast.style.animation = 'slideOut 0.3s ease-in';
        setTimeout(() => document.body.removeChild(toast), 300);
    }, 3000);
}

// Add CSS animations
const style = document.createElement('style');
style.textContent = `
    @keyframes slideIn {
        from { transform: translateX(400px); opacity: 0; }
        to { transform: translateX(0); opacity: 1; }
    }
    @keyframes slideOut {
        from { transform: translateX(0); opacity: 1; }
        to { transform: translateX(400px); opacity: 0; }
    }
`;
document.head.appendChild(style);

