/**
 * pagetable.c - Page table implementation
 */

#include "pagetable.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

PageTable *pagetable_create(uint32_t pid, PageTableType type, uint64_t address_space_size,
                             uint32_t page_size)
{
    if (!is_power_of_two(page_size)) {
        LOG_ERROR_MSG("Page size must be power of 2");
        return NULL;
    }

    PageTable *pt = calloc(1, sizeof(PageTable));
    if (!pt) {
        LOG_ERROR_MSG("Failed to allocate page table");
        return NULL;
    }

    pt->type = type;
    pt->pid = pid;
    pt->page_size = page_size;
    pt->address_space_size = address_space_size;

    uint32_t total_pages = address_space_size / page_size;

    if (type == PT_SINGLE_LEVEL) {
        pt->table.single.num_pages = total_pages;
        pt->table.single.ptes = calloc(total_pages, sizeof(PageTableEntry));
        if (!pt->table.single.ptes) {
            LOG_ERROR_MSG("Failed to allocate single-level page table entries");
            free(pt);
            return NULL;
        }
        LOG_INFO_MSG("Created single-level page table for PID %u: %u pages", pid, total_pages);
    } else {
        // Two-level page table
        // L1: 10 bits (1024 entries), L2: remaining bits
        pt->table.two_level.l1_entries = 1024;
        pt->table.two_level.l2_entries = (total_pages + 1023) / 1024;

        pt->table.two_level.l1_table = calloc(pt->table.two_level.l1_entries, sizeof(PageTableEntry *));
        if (!pt->table.two_level.l1_table) {
            LOG_ERROR_MSG("Failed to allocate two-level L1 table");
            free(pt);
            return NULL;
        }
        // L2 tables allocated on demand
        LOG_INFO_MSG("Created two-level page table for PID %u: L1=%u, L2=%u", pid,
                     pt->table.two_level.l1_entries, pt->table.two_level.l2_entries);
    }

    return pt;
}

void pagetable_destroy(PageTable *pt)
{
    if (!pt)
        return;

    if (pt->type == PT_SINGLE_LEVEL) {
        free(pt->table.single.ptes);
    } else {
        // Free all allocated L2 tables
        for (uint32_t i = 0; i < pt->table.two_level.l1_entries; i++) {
            if (pt->table.two_level.l1_table[i]) {
                free(pt->table.two_level.l1_table[i]);
            }
        }
        free(pt->table.two_level.l1_table);
    }
    free(pt);
}

PageTableEntry *pagetable_lookup(PageTable *pt, uint64_t virtual_addr)
{
    if (!pt)
        return NULL;

    uint64_t vpn = virtual_addr / pt->page_size;

    if (pt->type == PT_SINGLE_LEVEL) {
        if (vpn >= pt->table.single.num_pages) {
            return NULL; // Out of bounds
        }
        return &pt->table.single.ptes[vpn];
    } else {
        // Two-level lookup
        uint32_t l1_index = (vpn >> 10) & 0x3FF; // Top 10 bits
        uint32_t l2_index = vpn & 0x3FF;          // Bottom 10 bits

        if (l1_index >= pt->table.two_level.l1_entries) {
            return NULL;
        }

        // Check if L2 table exists
        if (!pt->table.two_level.l1_table[l1_index]) {
            return NULL; // L2 table not allocated
        }

        return &pt->table.two_level.l1_table[l1_index][l2_index];
    }
}

bool pagetable_map(PageTable *pt, uint64_t virtual_addr, uint32_t frame_number, uint32_t flags)
{
    if (!pt)
        return false;

    uint64_t vpn = virtual_addr / pt->page_size;

    if (pt->type == PT_SINGLE_LEVEL) {
        if (vpn >= pt->table.single.num_pages) {
            return false;
        }
        pt->table.single.ptes[vpn].frame_number = frame_number;
        pt->table.single.ptes[vpn].flags = flags | PTE_VALID;
        return true;
    } else {
        // Two-level mapping
        uint32_t l1_index = (vpn >> 10) & 0x3FF;
        uint32_t l2_index = vpn & 0x3FF;

        if (l1_index >= pt->table.two_level.l1_entries) {
            return false;
        }

        // Allocate L2 table if needed
        if (!pt->table.two_level.l1_table[l1_index]) {
            pt->table.two_level.l1_table[l1_index] =
                calloc(pt->table.two_level.l2_entries, sizeof(PageTableEntry));
            if (!pt->table.two_level.l1_table[l1_index]) {
                LOG_ERROR_MSG("Failed to allocate L2 table");
                return false;
            }
        }

        pt->table.two_level.l1_table[l1_index][l2_index].frame_number = frame_number;
        pt->table.two_level.l1_table[l1_index][l2_index].flags = flags | PTE_VALID;
        return true;
    }
}

bool pagetable_unmap(PageTable *pt, uint64_t virtual_addr)
{
    PageTableEntry *pte = pagetable_lookup(pt, virtual_addr);
    if (!pte)
        return false;

    pte->flags &= ~PTE_VALID;
    return true;
}

bool pte_is_valid(PageTableEntry *pte)
{
    return pte && (pte->flags & PTE_VALID);
}

bool pte_is_dirty(PageTableEntry *pte)
{
    return pte && (pte->flags & PTE_DIRTY);
}

bool pte_is_accessed(PageTableEntry *pte)
{
    return pte && (pte->flags & PTE_ACCESSED);
}

void pte_set_frame(PageTableEntry *pte, uint32_t frame_number)
{
    if (pte) {
        pte->frame_number = frame_number;
    }
}

void pte_set_valid(PageTableEntry *pte, bool valid)
{
    if (pte) {
        if (valid)
            pte->flags |= PTE_VALID;
        else
            pte->flags &= ~PTE_VALID;
    }
}

void pte_set_dirty(PageTableEntry *pte, bool dirty)
{
    if (pte) {
        if (dirty)
            pte->flags |= PTE_DIRTY;
        else
            pte->flags &= ~PTE_DIRTY;
    }
}

void pte_set_accessed(PageTableEntry *pte, bool accessed)
{
    if (pte) {
        if (accessed)
            pte->flags |= PTE_ACCESSED;
        else
            pte->flags &= ~PTE_ACCESSED;
    }
}

uint32_t pagetable_count_valid_pages(PageTable *pt)
{
    if (!pt)
        return 0;

    uint32_t count = 0;

    if (pt->type == PT_SINGLE_LEVEL) {
        for (uint32_t i = 0; i < pt->table.single.num_pages; i++) {
            if (pte_is_valid(&pt->table.single.ptes[i])) {
                count++;
            }
        }
    } else {
        for (uint32_t i = 0; i < pt->table.two_level.l1_entries; i++) {
            if (pt->table.two_level.l1_table[i]) {
                for (uint32_t j = 0; j < pt->table.two_level.l2_entries; j++) {
                    if (pte_is_valid(&pt->table.two_level.l1_table[i][j])) {
                        count++;
                    }
                }
            }
        }
    }

    return count;
}

