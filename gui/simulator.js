/**
 * VMM Simulator - Core Simulation Logic
 * Authors: Aditya Pandey, Kartik, Vivek, Gaurang
 */

class VMMSimulator {
    constructor(config) {
        this.config = config;
        this.reset();
    }

    reset() {
        // Calculate frames
        this.numFrames = Math.floor((this.config.ramSize * 1024 * 1024) / this.config.pageSize);
        
        console.log(`VMM Reset: ${this.config.ramSize}MB RAM, ${this.config.pageSize}B pages = ${this.numFrames} frames`);
        
        // Initialize data structures
        this.tlb = new Array(this.config.tlbSize).fill(null).map(() => ({
            valid: false,
            pid: 0,
            vpn: 0,
            pfn: 0,
            lastUse: 0
        }));
        
        this.pageTable = {};  // pid -> vpn -> pte mapping
        
        this.frames = new Array(this.numFrames).fill(null).map((_, i) => ({
            number: i,
            free: true,
            pid: 0,
            vpn: 0,
            dirty: false,
            referenced: false,
            lastAccess: 0
        }));
        
        this.freeFrames = Array.from({length: this.numFrames}, (_, i) => i);
        
        console.log(`Initialized ${this.frames.length} frames, free list has ${this.freeFrames.length} entries`);
        
        // Metrics
        this.metrics = {
            totalAccesses: 0,
            reads: 0,
            writes: 0,
            pageFaults: 0,
            tlbHits: 0,
            tlbMisses: 0,
            swapIns: 0,
            swapOuts: 0,
            replacements: 0
        };
        
        // Replacement policy state
        this.fifoQueue = [];
        this.clockHand = 0;
        this.accessCounter = 0;
        
        // Trace
        this.trace = [];
        this.currentStep = 0;
    }

    generateTrace(pattern, numAccesses) {
        this.trace = [];
        const maxVPN = Math.floor((4 * 1024 * 1024 * 1024) / this.config.pageSize); // 4GB address space
        
        switch (pattern) {
            case 'sequential':
                for (let i = 0; i < numAccesses; i++) {
                    const vpn = i % 256; // Cycle through 256 pages
                    this.trace.push({
                        pid: Math.floor(i / 100) % 4,
                        op: Math.random() > 0.2 ? 'R' : 'W',
                        vpn: vpn
                    });
                }
                break;
                
            case 'random':
                for (let i = 0; i < numAccesses; i++) {
                    this.trace.push({
                        pid: Math.floor(Math.random() * 4),
                        op: Math.random() > 0.25 ? 'R' : 'W',
                        vpn: Math.floor(Math.random() * 512)
                    });
                }
                break;
                
            case 'working_set':
                let workingSetBase = [0, 256, 512, 768];
                for (let i = 0; i < numAccesses; i++) {
                    const pid = i % 4;
                    const base = workingSetBase[pid];
                    let vpn;
                    if (Math.random() < 0.9) {
                        // 90% within working set
                        vpn = base + Math.floor(Math.random() * 64);
                    } else {
                        // 10% random
                        vpn = Math.floor(Math.random() * 512);
                    }
                    this.trace.push({
                        pid: pid,
                        op: Math.random() > 0.3 ? 'R' : 'W',
                        vpn: vpn
                    });
                    // Shift working set periodically
                    if (i % 500 === 0) {
                        workingSetBase[pid] = (workingSetBase[pid] + 16) % 512;
                    }
                }
                break;
                
            case 'locality':
                let currentVPN = 0;
                for (let i = 0; i < numAccesses; i++) {
                    if (Math.random() < 0.7) {
                        // 70% nearby
                        currentVPN = (currentVPN + Math.floor(Math.random() * 8) - 4 + 512) % 512;
                    } else {
                        // 30% jump
                        currentVPN = Math.floor(Math.random() * 512);
                    }
                    this.trace.push({
                        pid: Math.floor(i / 50) % 4,
                        op: Math.random() > 0.25 ? 'R' : 'W',
                        vpn: currentVPN
                    });
                }
                break;
                
            case 'thrashing':
                const numPages = Math.floor(this.numFrames * 1.5); // More pages than RAM
                for (let i = 0; i < numAccesses; i++) {
                    const vpn = (i % numPages);
                    this.trace.push({
                        pid: i % 4,
                        op: 'R',
                        vpn: vpn
                    });
                }
                break;
        }
    }

    step() {
        if (this.currentStep >= this.trace.length) {
            return { done: true };
        }
        
        const entry = this.trace[this.currentStep];
        const result = this.access(entry.pid, entry.vpn, entry.op === 'W');
        
        this.currentStep++;
        return {
            done: false,
            entry: entry,
            result: result,
            step: this.currentStep
        };
    }

    access(pid, vpn, isWrite) {
        this.metrics.totalAccesses++;
        if (isWrite) this.metrics.writes++;
        else this.metrics.reads++;
        
        this.accessCounter++;
        
        const result = {
            tlbHit: false,
            pageFault: false,
            pfn: -1,
            evicted: null
        };
        
        // Step 1: TLB lookup
        const tlbEntry = this.tlbLookup(pid, vpn);
        if (tlbEntry) {
            this.metrics.tlbHits++;
            result.tlbHit = true;
            result.pfn = tlbEntry.pfn;
            
            // Update frame access time
            this.frames[tlbEntry.pfn].lastAccess = this.accessCounter;
            this.frames[tlbEntry.pfn].referenced = true;
            if (isWrite) {
                this.frames[tlbEntry.pfn].dirty = true;
            }
            
            return result;
        }
        
        this.metrics.tlbMisses++;
        
        // Step 2: Page table lookup
        const pte = this.pageTableLookup(pid, vpn);
        if (pte && pte.valid) {
            // Page in memory, update TLB
            result.pfn = pte.pfn;
            this.tlbInsert(pid, vpn, pte.pfn);
            
            this.frames[pte.pfn].lastAccess = this.accessCounter;
            this.frames[pte.pfn].referenced = true;
            if (isWrite) {
                this.frames[pte.pfn].dirty = true;
            }
            
            return result;
        }
        
        // Step 3: Page fault
        this.metrics.pageFaults++;
        result.pageFault = true;
        
        // Allocate frame
        let pfn = this.allocateFrame();
        if (pfn === -1) {
            // Need to evict
            const victim = this.selectVictim();
            if (victim !== -1) {
                result.evicted = {
                    pfn: victim,
                    pid: this.frames[victim].pid,
                    vpn: this.frames[victim].vpn,
                    dirty: this.frames[victim].dirty
                };
                
                // Invalidate victim's PTE
                this.pageTableInvalidate(this.frames[victim].pid, this.frames[victim].vpn);
                this.tlbInvalidate(this.frames[victim].pid, this.frames[victim].vpn);
                
                if (this.frames[victim].dirty) {
                    this.metrics.swapOuts++;
                }
                
                pfn = victim;
                this.metrics.replacements++;
            }
        }
        
        if (pfn !== -1) {
            // Map page to frame
            this.frames[pfn].free = false;
            this.frames[pfn].pid = pid;
            this.frames[pfn].vpn = vpn;
            this.frames[pfn].dirty = isWrite;
            this.frames[pfn].referenced = true;
            this.frames[pfn].lastAccess = this.accessCounter;
            
            console.log(`Allocated frame ${pfn} to PID${pid} VPN:${vpn}, free=${this.frames[pfn].free}`);
            
            this.pageTableMap(pid, vpn, pfn);
            this.tlbInsert(pid, vpn, pfn);
            
            // Update replacement policy
            if (this.config.algorithm === 'FIFO') {
                this.fifoQueue.push(pfn);
            }
            
            result.pfn = pfn;
            
            // Check if we need to trigger swap-in (simulate loading from disk)
            if (result.pageFault && !result.evicted) {
                this.metrics.swapIns++;  // Count initial page loads as swap-ins
            }
        }
        
        return result;
    }

    tlbLookup(pid, vpn) {
        for (let i = 0; i < this.tlb.length; i++) {
            if (this.tlb[i].valid && this.tlb[i].pid === pid && this.tlb[i].vpn === vpn) {
                this.tlb[i].lastUse = this.accessCounter;
                return this.tlb[i];
            }
        }
        return null;
    }

    tlbInsert(pid, vpn, pfn) {
        // Find invalid entry or LRU entry
        let victimIdx = 0;
        let minUse = this.accessCounter;
        
        for (let i = 0; i < this.tlb.length; i++) {
            if (!this.tlb[i].valid) {
                victimIdx = i;
                break;
            }
            if (this.tlb[i].lastUse < minUse) {
                minUse = this.tlb[i].lastUse;
                victimIdx = i;
            }
        }
        
        this.tlb[victimIdx] = {
            valid: true,
            pid: pid,
            vpn: vpn,
            pfn: pfn,
            lastUse: this.accessCounter
        };
    }

    tlbInvalidate(pid, vpn) {
        for (let i = 0; i < this.tlb.length; i++) {
            if (this.tlb[i].valid && this.tlb[i].pid === pid && this.tlb[i].vpn === vpn) {
                this.tlb[i].valid = false;
            }
        }
    }

    pageTableLookup(pid, vpn) {
        if (!this.pageTable[pid]) {
            this.pageTable[pid] = {};
        }
        return this.pageTable[pid][vpn] || null;
    }

    pageTableMap(pid, vpn, pfn) {
        if (!this.pageTable[pid]) {
            this.pageTable[pid] = {};
        }
        this.pageTable[pid][vpn] = {
            valid: true,
            pfn: pfn,
            dirty: false,
            accessed: true
        };
    }

    pageTableInvalidate(pid, vpn) {
        if (this.pageTable[pid] && this.pageTable[pid][vpn]) {
            this.pageTable[pid][vpn].valid = false;
        }
    }

    allocateFrame() {
        if (this.freeFrames.length > 0) {
            return this.freeFrames.pop();
        }
        return -1;
    }

    selectVictim() {
        switch (this.config.algorithm) {
            case 'FIFO':
                return this.fifoQueue.shift();
                
            case 'LRU':
                let minTime = this.accessCounter;
                let victim = -1;
                for (let i = 0; i < this.frames.length; i++) {
                    if (!this.frames[i].free && this.frames[i].lastAccess < minTime) {
                        minTime = this.frames[i].lastAccess;
                        victim = i;
                    }
                }
                return victim;
                
            case 'CLOCK':
                let start = this.clockHand;
                while (true) {
                    if (!this.frames[this.clockHand].free) {
                        if (!this.frames[this.clockHand].referenced) {
                            const victim = this.clockHand;
                            this.clockHand = (this.clockHand + 1) % this.frames.length;
                            return victim;
                        }
                        this.frames[this.clockHand].referenced = false;
                    }
                    this.clockHand = (this.clockHand + 1) % this.frames.length;
                    if (this.clockHand === start) break;
                }
                return this.clockHand;
                
            default:
                return 0;
        }
    }

    getMetrics() {
        return {
            ...this.metrics,
            faultRate: this.metrics.totalAccesses > 0 
                ? (this.metrics.pageFaults / this.metrics.totalAccesses * 100).toFixed(2)
                : 0,
            tlbHitRate: (this.metrics.tlbHits + this.metrics.tlbMisses) > 0
                ? (this.metrics.tlbHits / (this.metrics.tlbHits + this.metrics.tlbMisses) * 100).toFixed(2)
                : 0
        };
    }
}

