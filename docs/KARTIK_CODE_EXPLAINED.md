# Kartik - Code Explanation (Hinglish)

**Role:** Memory Subsystems Developer - Page Tables & Replacement Algorithms

---

## 📁 Files jinhe maine banaya:
1. `src/pagetable.c` - Page table implementation (single & two-level)
2. `src/replacement.c` - Page replacement algorithms (FIFO, LRU, Clock, OPT)

---

## 1️⃣ Page Table (`src/pagetable.c`)

### Yeh kya hai?
Virtual addresses ko physical addresses mein convert karne ke liye map. Jaise phone book - naam se number dhundho, waise virtual address se physical frame number dhundho.

### Main Concepts:

**Single-Level Page Table:**
- Simple array - ek hi layer
- Fast but memory zyada use karta hai
- Chhote address spaces ke liye best

**Two-Level Page Table:**
- Do layers - L1 aur L2
- Memory save hoti hai sparse addresses ke liye
- Modern systems use karte hain

---

### **`pagetable_create()`** - Page table setup karna

```c
PageTable *pagetable_create(uint32_t pid, PageTableType type, 
                            uint64_t address_space_size, uint32_t page_size)
```

**Kya karta hai:**
Har process ke liye ek naya page table banata hai.

```c
PageTable *pt = calloc(1, sizeof(PageTable));

pt->type = type;                          // SINGLE ya TWO_LEVEL
pt->pid = pid;                            // Kis process ka hai
pt->page_size = page_size;                // Page kitne bade hain (4KB, 8KB)
pt->address_space_size = address_space_size;

// Total kitne pages possible hain calculate karo
uint32_t total_pages = address_space_size / page_size;

if (type == PT_SINGLE_LEVEL) {
    // Single-level: Bas ek array banao
    pt->table.single.num_pages = total_pages;
    pt->table.single.ptes = calloc(total_pages, sizeof(PageTableEntry));
    
    LOG_INFO_MSG("Single-level page table: %u pages", total_pages);
}
else {
    // Two-level: L1 aur L2 tables banao
    pt->table.two_level.l1_entries = 1024;  // Top 10 bits
    pt->table.two_level.l2_entries = (total_pages + 1023) / 1024;
    
    // L1 table banao (array of pointers)
    pt->table.two_level.l1_table = calloc(pt->table.two_level.l1_entries, 
                                          sizeof(PageTableEntry *));
    
    // L2 tables on-demand banenge (jab zarurat hogi)
    LOG_INFO_MSG("Two-level page table: L1=%u, L2=%u", 
                 pt->table.two_level.l1_entries, pt->table.two_level.l2_entries);
}

return pt;
```

**Single vs Two-level Example:**

Maan lo 4GB virtual space aur 4KB pages:
- Total pages = 4GB / 4KB = 1,048,576 pages
- Har PTE = 16 bytes

**Single-level:**
```
Space = 1,048,576 × 16 bytes = 16 MB per process! 😰
Agar 100 processes hain = 1.6 GB sirf page tables ke liye!
```

**Two-level:**
```
L1 = 1024 entries × 8 bytes = 8 KB (always allocated)
L2 = Only jahan actually pages use ho rahe hain
Example: Agar sirf 100 pages use ho rahe hain:
  L2 = 1 table × 1024 entries × 16 bytes = 16 KB
  Total = 8 KB + 16 KB = 24 KB only! 🎉
```

---

### **`pagetable_lookup()`** - Address translation

**Sabse important function!** Virtual address se PTE dhundta hai.

```c
PageTableEntry *pagetable_lookup(PageTable *pt, uint64_t virtual_addr)
{
    // Virtual address se VPN (Virtual Page Number) nikalo
    uint64_t vpn = virtual_addr / pt->page_size;
    
    if (pt->type == PT_SINGLE_LEVEL) {
        // Single-level: Direct array index
        if (vpn >= pt->table.single.num_pages) {
            return NULL;  // Out of bounds!
        }
        return &pt->table.single.ptes[vpn];  // O(1) lookup! ⚡
    }
    else {
        // Two-level: Do steps
        
        // Step 1: Top 10 bits se L1 index nikalo
        uint32_t l1_index = (vpn >> 10) & 0x3FF;  // 0x3FF = 1023 in binary
        
        // Step 2: Bottom 10 bits se L2 index nikalo
        uint32_t l2_index = vpn & 0x3FF;
        
        // Check: L1 index valid hai?
        if (l1_index >= pt->table.two_level.l1_entries) {
            return NULL;
        }
        
        // Check: L2 table exist karta hai?
        if (!pt->table.two_level.l1_table[l1_index]) {
            return NULL;  // L2 table nahi banaya gaya abhi
        }
        
        // L2 table se PTE return karo
        return &pt->table.two_level.l1_table[l1_index][l2_index];
    }
}
```

**Example walkthrough:**

Virtual address = `0x00801234`, Page size = 4KB (4096 bytes)

```
Step 1: VPN nikalo
VPN = 0x00801234 / 4096 = 0x00801 = 2049 (decimal)

Step 2: Two-level breakdown
Binary VPN = 00000 00000 10000 00001
             ↑        ↑        ↑
             unused   L1(10)   L2(10)

L1 index = 00000 00010 = 2
L2 index = 00000 00001 = 1

Step 3: Lookup
- L1 table[2] pe jao
- Wahan se L2 table ka pointer lo
- L2 table[1] pe jao
- Woh PTE return karo! ✅
```

---

### **`pagetable_map()`** - Virtual to physical mapping create karna

```c
bool pagetable_map(PageTable *pt, uint64_t virtual_addr, 
                   uint32_t frame_number, uint32_t flags)
{
    uint64_t vpn = virtual_addr / pt->page_size;
    
    if (pt->type == PT_SINGLE_LEVEL) {
        // Single-level: Seedha set kar do
        if (vpn >= pt->table.single.num_pages) {
            return false;
        }
        
        pt->table.single.ptes[vpn].frame_number = frame_number;
        pt->table.single.ptes[vpn].flags = flags | PTE_VALID;
        return true;
    }
    else {
        // Two-level: Pehle ensure karo L2 table exist karta hai
        uint32_t l1_index = (vpn >> 10) & 0x3FF;
        uint32_t l2_index = vpn & 0x3FF;
        
        if (l1_index >= pt->table.two_level.l1_entries) {
            return false;
        }
        
        // Agar L2 table nahi hai, toh banao (lazy allocation)
        if (!pt->table.two_level.l1_table[l1_index]) {
            pt->table.two_level.l1_table[l1_index] = 
                calloc(pt->table.two_level.l2_entries, sizeof(PageTableEntry));
            
            if (!pt->table.two_level.l1_table[l1_index]) {
                LOG_ERROR_MSG("Failed to allocate L2 table");
                return false;
            }
        }
        
        // Ab L2 entry set karo
        pt->table.two_level.l1_table[l1_index][l2_index].frame_number = frame_number;
        pt->table.two_level.l1_table[l1_index][l2_index].flags = flags | PTE_VALID;
        return true;
    }
}
```

**Lazy allocation ka matlab:**
- L2 tables tabhi banate hain jab zarurat ho
- Memory bachti hai! 💰
- Agar koi process sirf kuch hi pages use kare, toh baaki L2 tables nahi banenge

---

### **Page Table Entry (PTE) Flags**

Har PTE mein frame number ke saath kuch flags bhi hote hain:

```c
#define PTE_VALID    (1 << 0)  // Page RAM mein hai
#define PTE_DIRTY    (1 << 1)  // Page modify hua hai
#define PTE_ACCESSED (1 << 2)  // Page access hua hai
#define PTE_WRITE    (1 << 3)  // Write allowed hai
#define PTE_USER     (1 << 4)  // User mode access hai
```

**Flags check karne ke helper functions:**

```c
bool pte_is_valid(PageTableEntry *pte)
{
    return pte && (pte->flags & PTE_VALID);  // Bit 0 check
}

bool pte_is_dirty(PageTableEntry *pte)
{
    return pte && (pte->flags & PTE_DIRTY);  // Bit 1 check
}

void pte_set_dirty(PageTableEntry *pte, bool dirty)
{
    if (pte) {
        if (dirty)
            pte->flags |= PTE_DIRTY;   // Bit set (OR operation)
        else
            pte->flags &= ~PTE_DIRTY;  // Bit clear (AND with NOT)
    }
}
```

---

## 2️⃣ Replacement Algorithms (`src/replacement.c`)

### Yeh kya hai?
Jab RAM full ho jaye, toh kis page ko nikalna hai? Yeh decide karne ke liye algorithms!

Humne 5 algorithms implement kiye:
1. **FIFO** - First In First Out
2. **LRU** - Least Recently Used
3. **Approx-LRU** - Approximate LRU (aging)
4. **Clock** - Second Chance
5. **OPT** - Optimal (Belady's algorithm)

---

### **FIFO Algorithm** - Sabse simple!

**Concept:**
Jo page pehle aaya, woh pehle jayega. Queue jaisa.

```c
case REPLACE_FIFO: {
    // FIFO queue se head nikalo
    if (policy->fifo_size == 0) {
        return -1;  // Kuch nahi hai queue mein
    }
    
    // Head wala frame victim hai
    uint32_t victim = policy->fifo_queue[policy->fifo_head];
    
    // Head aage badhao (circular queue)
    policy->fifo_head = (policy->fifo_head + 1) % allocator->total_frames;
    policy->fifo_size--;
    
    LOG_TRACE_MSG("FIFO victim: frame %u", victim);
    return victim;
}
```

**Example:**
```
Frames allocate hue order: 5 → 3 → 7 → 2
FIFO queue: [5, 3, 7, 2]
             ↑ head

Victim = 5 (oldest)
After eviction: [3, 7, 2]
                 ↑ head
```

**Problem with FIFO:**
- Access pattern ignore karta hai
- Purana page frequently used ho sakta hai, phir bhi nikal dega! 😞
- **Belady's Anomaly**: Kabhi-kabhi zyada frames se zyada page faults!

---

### **LRU Algorithm** - Best performance!

**Concept:**
Jo page sabse lamba time se access nahi hua, woh nikalo.

```c
case REPLACE_LRU: {
    // Sabhi frames mein se minimum access time wala dhundho
    uint64_t min_time = UINT64_MAX;
    int32_t victim = -1;
    
    for (uint32_t i = 0; i < allocator->total_frames; i++) {
        if (allocator->frames[i].state == FRAME_ALLOCATED) {
            if (allocator->frames[i].last_access_time < min_time) {
                min_time = allocator->frames[i].last_access_time;
                victim = i;
            }
        }
    }
    
    LOG_TRACE_MSG("LRU victim: frame %d (time %lu)", victim, min_time);
    return victim;
}
```

**Kaise kaam karta hai:**

```
Current time = 1000

Frame 0: last_access = 950  (50 units pehle)
Frame 1: last_access = 990  (10 units pehle) ← recently used
Frame 2: last_access = 800  (200 units pehle) ← victim! oldest
Frame 3: last_access = 920  (80 units pehle)

Victim = Frame 2 (LRU)
```

**Pros:**
- Best approximation of optimal
- Good hit rates
- Considers actual usage

**Cons:**
- O(n) time to find victim (slow for large RAM)
- Har access pe timestamp update karna padta hai

---

### **Approximate LRU (Aging)** - Smart optimization!

**Concept:**
LRU jaisa performance, but O(1) victim selection!

**Kaise:**
Har frame mein ek "age counter" (32-bit) hai.

```c
case REPLACE_APPROX_LRU: {
    // Minimum age counter wala frame dhundho
    uint32_t min_age = UINT32_MAX;
    int32_t victim = -1;
    
    for (uint32_t i = 0; i < allocator->total_frames; i++) {
        if (allocator->frames[i].state == FRAME_ALLOCATED) {
            if (allocator->frames[i].age_counter < min_age) {
                min_age = allocator->frames[i].age_counter;
                victim = i;
            }
        }
    }
    
    LOG_TRACE_MSG("Approx-LRU victim: frame %d (age %u)", victim, min_age);
    return victim;
}
```

**Aging process** (periodic update):

```c
void frame_age_all(FrameAllocator *allocator)
{
    for (uint32_t i = 0; i < allocator->total_frames; i++) {
        if (allocator->frames[i].state == FRAME_ALLOCATED) {
            // Right shift karo (divide by 2)
            allocator->frames[i].age_counter >>= 1;
            
            // Agar recently access hua, toh MSB set karo
            if (allocator->frames[i].reference_bit) {
                allocator->frames[i].age_counter |= 0x80000000;  // Set MSB
                allocator->frames[i].reference_bit = 0;  // Clear reference bit
            }
        }
    }
}
```

**Example:**

```
Initial state:
Frame 0: age = 00000000, ref_bit = 0
Frame 1: age = 00000000, ref_bit = 1
Frame 2: age = 00000000, ref_bit = 0

After aging:
Frame 0: age = 00000000 >> 1 = 00000000 (not accessed)
Frame 1: age = (00000000 >> 1) | 10000000 = 10000000 (accessed!)
Frame 2: age = 00000000 >> 1 = 00000000 (not accessed)

Next access to Frame 0, set ref_bit = 1

After next aging:
Frame 0: age = (00000000 >> 1) | 10000000 = 10000000
Frame 1: age = 10000000 >> 1 = 01000000
Frame 2: age = 00000000 >> 1 = 00000000 ← LRU! (smallest age)
```

**Magic:**
- MSB = most recent access
- LSB = oldest access
- 32 bits = 32 levels of history! 🎯

---

### **Clock Algorithm** - Second Chance

**Concept:**
Circular list with a "clock hand". Reference bit = second chance.

```c
case REPLACE_CLOCK: {
    uint32_t start = policy->clock_hand;
    
    while (1) {
        if (allocator->frames[policy->clock_hand].state == FRAME_ALLOCATED) {
            
            if (allocator->frames[policy->clock_hand].reference_bit == 0) {
                // Found victim! Reference bit = 0
                uint32_t victim = policy->clock_hand;
                policy->clock_hand = (policy->clock_hand + 1) % allocator->total_frames;
                LOG_TRACE_MSG("Clock victim: frame %u", victim);
                return victim;
            }
            else {
                // Give second chance - clear reference bit
                allocator->frames[policy->clock_hand].reference_bit = 0;
            }
        }
        
        // Move clock hand forward
        policy->clock_hand = (policy->clock_hand + 1) % allocator->total_frames;
        
        // Prevent infinite loop
        if (policy->clock_hand == start) break;
    }
    
    // Agar sab ke reference bit set hain, toh current hand pe wala victim
    return policy->clock_hand;
}
```

**Visualization:**

```
Circular frame list:
   [F0,r=1] → [F1,r=0] → [F2,r=1] → [F3,r=0]
      ↑                                 ↓
   [F7,r=1] ← [F6,r=0] ← [F5,r=1] ← [F4,r=1]
                              ↑
                          clock_hand

Scan:
- F5: r=1 → Clear to 0, move hand
- F6: r=0 → VICTIM! ✅
```

**Kyun achha hai:**
- LRU se zyada simple
- FIFO se zyada smart
- Reference bit = "recently used?" check
- Real OSes use karte hain! (Linux, Windows)

---

### **OPT Algorithm** - Theoretically best!

**Concept:**
Jo page sabse door future mein use hoga, woh nikalo.

**Problem:**
Future mein kya hoga kaise pata? 🔮

**Solution:**
Simulation ke liye - trace pehle se available hai, toh future dekh sakte hain!

```c
case REPLACE_OPT: {
    // Har frame ke liye check karo - next use kab hai?
    uint64_t max_next_use = 0;
    int32_t victim = -1;
    
    for (uint32_t i = 0; i < allocator->total_frames; i++) {
        if (allocator->frames[i].state == FRAME_ALLOCATED) {
            
            // Future mein next use dhundho
            uint64_t next_use = find_next_use(policy, i, allocator);
            
            // Jo sabse door use hoga, woh victim
            if (next_use > max_next_use) {
                max_next_use = next_use;
                victim = i;
            }
        }
    }
    
    LOG_TRACE_MSG("OPT victim: frame %d (next use at %lu)", victim, max_next_use);
    return victim;
}

// Helper function
static uint64_t find_next_use(ReplacementPolicy *policy, uint32_t frame_num,
                               FrameAllocator *allocator)
{
    // Future trace mein scan karo
    for (uint64_t i = policy->current_index + 1; i < policy->trace->count; i++) {
        TraceEntry *entry = trace_get(policy->trace, i);
        
        if (entry->pid == allocator->frames[frame_num].pid) {
            uint64_t vpn = entry->virtual_addr / 4096;
            if (vpn == allocator->frames[frame_num].vpn) {
                return i;  // Found! Is position pe use hoga
            }
        }
    }
    
    return UINT64_MAX;  // Kabhi use nahi hoga
}
```

**Example:**

```
Current position: 100
Remaining trace:
  105: Access page A
  110: Access page C
  115: Access page B
  200: Access page A
  
Current frames: A, B, C, D

Next use positions:
- Page A: 105 (soon!)
- Page B: 115
- Page C: 110
- Page D: never (UINT64_MAX) ← VICTIM! ✅

OPT selects D (used furthest in future)
```

**Kyon important:**
- Theoretical best performance
- Comparison baseline for other algorithms
- Real OS mein use nahi ho sakta (future nahi pata)
- Research ke liye useful! 📊

---

## 📊 Algorithm Comparison

| Algorithm | Time Complexity | Space | Pros | Cons |
|-----------|----------------|-------|------|------|
| FIFO | O(1) | O(n) queue | Simple | Belady's anomaly |
| LRU | O(n) scan | O(1) | Best practical | Slow victim selection |
| Approx-LRU | O(n) scan | O(1) | Good + fast | Periodic aging needed |
| Clock | O(n) worst | O(1) | Balanced | Can scan full circle |
| OPT | O(n×trace) | O(trace) | Theoretical best | Needs future! |

---

## 💡 Key Learnings:

### Page Tables:
1. **Two-level saves memory** for sparse address spaces
2. **Lazy allocation** = only create L2 when needed
3. **Flags important** - valid, dirty, accessed bits track karte hain

### Replacement:
1. **No single best** - trade-offs everywhere
2. **LRU closest to OPT** practically
3. **Clock = good balance** (that's why Linux uses it!)
4. **FIFO simplest** but can be worst

---

**Questions? Code padho aur experiment karo! 🚀**

