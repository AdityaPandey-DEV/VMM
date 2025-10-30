/**
 * pagetable.h - Page table management
 * 
 * Supports both single-level and two-level page tables.
 * Provides virtual-to-physical address translation and page table entry management.
 */

#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <stdint.h>
#include <stdbool.h>

// Page table entry flags
#define PTE_VALID (1 << 0)    // Page is in physical memory
#define PTE_DIRTY (1 << 1)    // Page has been modified
#define PTE_ACCESSED (1 << 2) // Page has been accessed
#define PTE_WRITE (1 << 3)    // Page is writable
#define PTE_USER (1 << 4)     // User-mode accessible

// Page table entry (for both single and two-level)
typedef struct {
    uint32_t frame_number; // Physical frame number (or swap offset if not valid)
    uint32_t flags;        // PTE_* flags
    uint64_t swap_offset;  // Offset in swap file if paged out
} PageTableEntry;

// Page table types
typedef enum { PT_SINGLE_LEVEL, PT_TWO_LEVEL } PageTableType;

// Single-level page table
typedef struct {
    uint32_t num_pages;  // Total virtual pages
    PageTableEntry *ptes; // Array of PTEs
} SingleLevelPageTable;

// Two-level page table
typedef struct {
    uint32_t l1_entries;      // Number of L1 entries
    uint32_t l2_entries;      // Number of L2 entries per L1 entry
    PageTableEntry **l1_table; // L1 table (array of pointers to L2 tables)
} TwoLevelPageTable;

// Generic page table structure
typedef struct {
    PageTableType type;
    uint32_t pid;         // Process ID owning this page table
    uint32_t page_size;   // Page size in bytes
    uint64_t address_space_size; // Total virtual address space
    union {
        SingleLevelPageTable single;
        TwoLevelPageTable two_level;
    } table;
} PageTable;

// Page table creation and destruction
PageTable *pagetable_create(uint32_t pid, PageTableType type, uint64_t address_space_size,
                             uint32_t page_size);
void pagetable_destroy(PageTable *pt);

// Address translation
PageTableEntry *pagetable_lookup(PageTable *pt, uint64_t virtual_addr);
bool pagetable_map(PageTable *pt, uint64_t virtual_addr, uint32_t frame_number, uint32_t flags);
bool pagetable_unmap(PageTable *pt, uint64_t virtual_addr);

// PTE manipulation
bool pte_is_valid(PageTableEntry *pte);
bool pte_is_dirty(PageTableEntry *pte);
bool pte_is_accessed(PageTableEntry *pte);
void pte_set_frame(PageTableEntry *pte, uint32_t frame_number);
void pte_set_valid(PageTableEntry *pte, bool valid);
void pte_set_dirty(PageTableEntry *pte, bool dirty);
void pte_set_accessed(PageTableEntry *pte, bool accessed);

// Statistics
uint32_t pagetable_count_valid_pages(PageTable *pt);

#endif // PAGETABLE_H

