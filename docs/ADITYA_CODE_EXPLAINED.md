# Aditya Pandey - Code Explanation (Hinglish)

**Role:** Lead Developer - Core VMM, Frame Allocator, Swap Manager

---

## 📁 Files jinhe maine banaya:
1. `src/vmm.c` - Main VMM orchestration
2. `src/frame.c` - Physical memory frames ka management
3. `src/swap.c` - Swap space (disk storage) simulation
4. `src/util.c` - Helper functions

---

## 1️⃣ Frame Allocator (`src/frame.c`)

### Yeh kya hai?
Physical memory frames ko manage karta hai - matlab RAM ke chhote pieces ko track karta hai.

### Main Functions:

#### **`frame_allocator_create()`** - Setup karna
```c
FrameAllocator *frame_allocator_create(uint32_t num_frames)
```

**Hinglish Explanation:**
- Jab program start hota hai, toh sabse pehle hume physical memory ko setup karna padta hai
- `num_frames` batata hai ki kitne frames hai (jaise 64MB RAM mein 16,384 frames of 4KB each)
- Yeh function 3 cheezein banata hai:
  1. **Frame metadata array** - har frame ki information store karta hai
  2. **Free list** - jo frames khali hain unki list (stack jaisa)
  3. **Bitmap** - quick check ke liye ki frame free hai ya nahi (0 ya 1)

**Line-by-line:**
```c
// Memory allocate karo allocator ke liye
FrameAllocator *allocator = malloc(sizeof(FrameAllocator));

// Total aur free frames set karo
allocator->total_frames = num_frames;
allocator->free_frames = num_frames;  // Shuru mein sab free hain

// Har frame ki info store karne ke liye array banao
allocator->frames = calloc(num_frames, sizeof(FrameInfo));

// Free frames ki list banao (stack structure)
allocator->free_list = malloc(num_frames * sizeof(uint32_t));

// Sabhi frame numbers free list mein daalo
for (uint32_t i = 0; i < num_frames; i++) {
    allocator->free_list[i] = i;  // 0, 1, 2, 3, ... sab frames
}

// Bitmap banao - har frame ke liye 1 bit (0 = free, 1 = used)
uint32_t bitmap_size = (num_frames + 7) / 8;
allocator->bitmap = calloc(bitmap_size, 1);
```

---

#### **`frame_alloc()`** - Naya frame lena

**Kya karta hai:**
Jab kisi process ko naya page chahiye, toh yeh ek free frame de deta hai.

```c
int32_t frame_alloc(FrameAllocator *allocator)
{
    // Check karo ki free frames hain ya nahi
    if (allocator->free_list_top == 0) {
        return -1;  // Koi free frame nahi hai! ❌
    }
    
    // Free list se top wala frame nikalo (stack pop)
    uint32_t frame_num = allocator->free_list[--allocator->free_list_top];
    allocator->free_frames--;  // Free count kam karo
    
    // Is frame ko "allocated" mark karo
    allocator->frames[frame_num].state = FRAME_ALLOCATED;
    allocator->frames[frame_num].reference_bit = 1;  // Access hua hai
    allocator->frames[frame_num].last_access_time = get_timestamp_us();
    
    // Bitmap mein bhi mark karo (bit set to 1)
    allocator->bitmap[frame_num / 8] |= (1 << (frame_num % 8));
    
    return (int32_t)frame_num;  // Frame number return karo ✅
}
```

**Simple words mein:**
1. Free list check karo
2. Agar free frame hai toh woh de do
3. Usko "used" mark kar do
4. Frame number return kar do

---

#### **`frame_free()`** - Frame wapas karna

**Jab karna hai:**
Jab ek page ab zarurat nahi hai, toh uska frame wapas free kar do.

```c
bool frame_free(FrameAllocator *allocator, uint32_t frame_num)
{
    // Check karo ki frame pehle se free toh nahi
    if (allocator->frames[frame_num].state == FRAME_FREE) {
        return false;  // Pehle se hi free hai!
    }
    
    // Frame ko clean karo - sab info delete karo
    allocator->frames[frame_num].state = FRAME_FREE;
    allocator->frames[frame_num].pid = 0;
    allocator->frames[frame_num].vpn = 0;
    allocator->frames[frame_num].dirty = false;
    
    // Bitmap mein unset karo (bit to 0)
    allocator->bitmap[frame_num / 8] &= ~(1 << (frame_num % 8));
    
    // Free list mein wapas daalo (stack push)
    allocator->free_list[allocator->free_list_top++] = frame_num;
    allocator->free_frames++;
    
    return true;  // Successfully freed! ✅
}
```

---

### **Kyon bitmap + free list dono use karte hain?**

**Bitmap:**
- Bahut fast check - O(1) time
- "Is frame free hai?" - instantly pata chal jata hai
- Kam memory use (1 bit per frame)

**Free List:**
- Fast allocation - O(1) time
- Stack jaisa - top se frame nikalo
- Sabse recently freed frame pehle milega

**Together:** Best of both worlds! 🎯

---

## 2️⃣ Swap Manager (`src/swap.c`)

### Yeh kya hai?
Jab RAM full ho jaye, toh kuch pages ko disk pe save kar dete hain. Yeh "swap space" ko manage karta hai.

### Main Functions:

#### **`swap_create()`** - Swap space setup

```c
SwapManager *swap_create(uint32_t num_slots)
```

**Kya karta hai:**
Disk pe ek jagah banata hai jahan pages save ho sakein.

```c
SwapManager *swap = malloc(sizeof(SwapManager));

swap->total_slots = num_slots;      // Kitne pages save kar sakte hain
swap->used_slots = 0;                // Abhi koi use nahi hai
swap->swap_in_count = 0;             // Kitni baar disk se load kiya
swap->swap_out_count = 0;            // Kitni baar disk pe save kiya

// Har slot ki info ke liye array
swap->slots = calloc(num_slots, sizeof(SwapSlot));

// Free slots ki list (frame allocator jaisa)
swap->free_list = malloc(num_slots * sizeof(uint32_t));
for (uint32_t i = 0; i < num_slots; i++) {
    swap->free_list[i] = i;  // Sab slots initially free
}
```

---

#### **`swap_alloc()`** - Disk pe jagah reserve karna

**Kab chahiye:**
Jab ek page ko RAM se nikalna hai aur disk pe save karna hai.

```c
int32_t swap_alloc(SwapManager *swap, uint32_t pid, uint64_t vpn)
{
    // Check karo free slot hai ya nahi
    if (swap->free_list_top == 0) {
        return -1;  // Swap space bhi full! 😱
    }
    
    // Free slot nikalo
    uint32_t slot = swap->free_list[--swap->free_list_top];
    
    // Is slot ko mark karo as used
    swap->slots[slot].used = true;
    swap->slots[slot].pid = pid;      // Kis process ka page hai
    swap->slots[slot].vpn = vpn;      // Konsa page hai
    swap->used_slots++;
    
    return (int32_t)slot;  // Slot number return karo
}
```

---

#### **`swap_out()`** - Page ko disk pe save karna

```c
uint64_t swap_out(SwapManager *swap, uint32_t slot, void *data)
{
    // Statistics update karo
    swap->swap_out_count++;
    
    // Simulate disk write latency (5 milliseconds)
    // Real mein yahan disk pe write hota, but simulation hai toh sirf time return karte hain
    return 5000;  // 5000 microseconds = 5ms
}
```

**Samjho:**
- Real OS mein yeh disk pe actually write karega
- Hamara simulator sirf time simulate karta hai
- Disk slow hota hai - isliye 5ms latency

---

#### **`swap_in()`** - Page ko disk se wapas load karna

```c
uint64_t swap_in(SwapManager *swap, uint32_t slot, void *data)
{
    // Statistics update
    swap->swap_in_count++;
    
    // Simulate disk read latency
    return 5000;  // 5ms lagta hai read karne mein
}
```

**Kyon slow hai:**
- Disk RAM se bahut slow hota hai
- RAM: 100 nanoseconds (0.0001 ms)
- Disk: 5000000 nanoseconds (5 ms)
- **50,000 times slower!** 🐌

---

## 3️⃣ VMM Core (`src/vmm.c`)

### Yeh sabse important hai! 🌟

Yeh sab components ko connect karta hai aur memory access handle karta hai.

### **`vmm_create()`** - Pure system ko setup karna

```c
VMM *vmm_create(VMMConfig *config)
{
    VMM *vmm = calloc(1, sizeof(VMM));
    
    // Configuration copy karo
    memcpy(&vmm->config, config, sizeof(VMMConfig));
    
    // 1. Frame allocator banao
    vmm->frame_allocator = frame_allocator_create(config->num_frames);
    
    // 2. TLB banao (fast cache)
    vmm->tlb = tlb_create(config->tlb_size, config->tlb_policy);
    
    // 3. Swap manager banao
    uint32_t swap_slots = (config->swap_size_mb * 1024 * 1024) / config->page_size;
    vmm->swap = swap_create(swap_slots);
    
    // 4. Replacement policy setup karo
    vmm->replacement_policy = replacement_create(config->replacement_algo, config->num_frames);
    
    // 5. Metrics collector banao
    vmm->metrics = metrics_create(config->max_processes);
    
    // 6. Process array banao
    vmm->processes = calloc(config->max_processes, sizeof(Process));
    
    return vmm;
}
```

**Samjho:**
Yeh ek factory jaisa hai jo pure virtual memory system ko assemble karta hai!

---

### **`vmm_access()`** - Memory access ka main logic

**Yeh sabse complex function hai!** Chalo step-by-step samajhte hain:

```c
bool vmm_access(VMM *vmm, uint32_t pid, uint64_t virtual_addr, bool is_write)
{
    // Step 1: Metrics update karo
    metrics_record_access(vmm->metrics, pid, is_write);
    
    // Virtual address se page number nikalo
    uint64_t vpn = virtual_addr / vmm->config.page_size;
    uint32_t pfn;  // Physical frame number
    
    // Step 2: Pehle TLB mein dhundho (fastest!)
    if (tlb_lookup(vmm->tlb, pid, vpn, &pfn)) {
        // TLB HIT! 🎯 Mil gaya!
        metrics_record_tlb_hit(vmm->metrics, pid);
        
        // Frame ko access mark karo (for LRU)
        replacement_on_access(vmm->replacement_policy, pfn, vmm->frame_allocator);
        
        // Agar write hai toh dirty bit set karo
        if (is_write) {
            frame_set_dirty(vmm->frame_allocator, pfn, true);
        }
        
        return true;  // Ho gaya! ✅
    }
    
    // TLB miss ho gaya 😞
    metrics_record_tlb_miss(vmm->metrics, pid);
    
    // Step 3: Page table mein dhundho
    Process *proc = vmm_get_process(vmm, pid);
    PageTableEntry *pte = pagetable_lookup(proc->page_table, virtual_addr);
    
    if (pte && pte_is_valid(pte)) {
        // Page table mein mil gaya! RAM mein hai page
        pfn = pte->frame_number;
        
        // TLB mein add kar do for next time
        tlb_insert(vmm->tlb, pid, vpn, pfn);
        
        // Frame access update karo
        replacement_on_access(vmm->replacement_policy, pfn, vmm->frame_allocator);
        
        if (is_write) {
            frame_set_dirty(vmm->frame_allocator, pfn, true);
        }
        
        return true;  // Done! ✅
    }
    
    // Step 4: PAGE FAULT! 😱 Page RAM mein nahi hai
    return vmm_handle_page_fault(vmm, proc, virtual_addr, is_write);
}
```

---

### **`vmm_handle_page_fault()`** - Sabse challenging part!

**Jab page RAM mein nahi hai:**

```c
static bool vmm_handle_page_fault(VMM *vmm, Process *proc, uint64_t virtual_addr, bool is_write)
{
    LOG_DEBUG_MSG("Page fault: PID=%u, addr=0x%lx", proc->pid, virtual_addr);
    
    metrics_record_page_fault(vmm->metrics, proc->pid, is_major);
    
    // Pehle free frame dhundho
    int32_t frame_num = frame_alloc(vmm->frame_allocator);
    
    if (frame_num < 0) {
        // Koi free frame nahi! 😰
        // Victim select karo - kisi ko nikalna padega
        
        frame_num = replacement_select_victim(vmm->replacement_policy, vmm->frame_allocator);
        
        FrameInfo *victim = frame_get_info(vmm->frame_allocator, frame_num);
        
        // Agar victim dirty hai, toh pehle disk pe save karo
        if (victim->dirty) {
            int32_t swap_slot = swap_alloc(vmm->swap, victim->pid, victim->vpn);
            swap_out(vmm->swap, swap_slot, NULL);  // Disk pe save
            metrics_record_swap_out(vmm->metrics);
        }
        
        // Victim ke page table entry ko invalid karo
        // Victim ke TLB entry ko remove karo
        
        metrics_record_replacement(vmm->metrics);
    }
    
    // Agar page swap space mein tha, toh load karo
    if (pte->swap_offset > 0) {
        swap_in(vmm->swap, pte->swap_offset, NULL);  // Disk se load
        metrics_record_swap_in(vmm->metrics);
    }
    
    // Naya mapping banao
    pagetable_map(proc->page_table, virtual_addr, frame_num, flags);
    
    // Frame info update karo
    frame_set_pid(vmm->frame_allocator, frame_num, proc->pid);
    frame_set_vpn(vmm->frame_allocator, frame_num, vpn);
    
    // TLB mein add karo
    tlb_insert(vmm->tlb, pid, vpn, frame_num);
    
    return true;  // Page fault handle ho gaya! ✅
}
```

---

## 📊 Summary - Aditya ka contribution:

### Frame Allocator:
- ✅ O(1) allocation aur deallocation
- ✅ Bitmap for fast checking
- ✅ Free list for efficient reuse
- ✅ Frame metadata tracking

### Swap Manager:
- ✅ Disk simulation
- ✅ Swap slot allocation
- ✅ I/O latency modeling
- ✅ Statistics tracking

### VMM Core:
- ✅ Address translation pipeline
- ✅ TLB → Page Table → Page Fault
- ✅ Page fault handling
- ✅ Victim eviction
- ✅ Swap in/out coordination

---

## 💡 Key Learnings:

1. **Memory hierarchy fast se slow:**
   - TLB: 1 ns ⚡
   - RAM: 100 ns 🏃
   - Disk: 5,000,000 ns 🐌

2. **Data structures matter:**
   - Bitmap: Fast checking
   - Free list: Fast allocation
   - Dono saath mein = Best performance! 🎯

3. **Page fault expensive hai:**
   - TLB miss: 100x slower
   - Page fault: 50,000x slower! 😱

---

**Bas itna hi! Questions? Code mein comments bhi padho! 📖**

