/**
 * tlb.h - Translation Lookaside Buffer simulation
 * 
 * Fast cache for virtual-to-physical address translations.
 * Supports FIFO and LRU replacement policies.
 */

#ifndef TLB_H
#define TLB_H

#include <stdint.h>
#include <stdbool.h>

// TLB entry
typedef struct {
    bool valid;
    uint32_t pid;           // Process ID for tagged TLB
    uint64_t vpn;           // Virtual page number
    uint32_t pfn;           // Physical frame number
    uint64_t last_use_time; // For LRU replacement
    uint32_t fifo_index;    // For FIFO replacement
} TLBEntry;

// TLB replacement policies
typedef enum { TLB_FIFO, TLB_LRU } TLBPolicy;

// TLB structure
typedef struct {
    uint32_t size;       // Number of TLB entries
    TLBEntry *entries;   // Array of TLB entries
    TLBPolicy policy;    // Replacement policy
    uint32_t fifo_next;  // Next entry for FIFO
    uint64_t access_counter; // For LRU timestamp
} TLB;

// TLB operations
TLB *tlb_create(uint32_t size, TLBPolicy policy);
void tlb_destroy(TLB *tlb);

// Lookup and update
bool tlb_lookup(TLB *tlb, uint32_t pid, uint64_t vpn, uint32_t *pfn);
void tlb_insert(TLB *tlb, uint32_t pid, uint64_t vpn, uint32_t pfn);
void tlb_invalidate(TLB *tlb, uint32_t pid, uint64_t vpn);
void tlb_invalidate_all(TLB *tlb, uint32_t pid); // For context switch
void tlb_flush(TLB *tlb);                          // Clear entire TLB

#endif // TLB_H

