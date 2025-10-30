/**
 * swap.h - Backing store (swap space) simulation
 * 
 * Simulates disk-based storage for paged-out memory.
 * Tracks swap slots and simulates I/O latency.
 */

#ifndef SWAP_H
#define SWAP_H

#include <stdint.h>
#include <stdbool.h>

// Swap slot metadata
typedef struct {
    bool used;
    uint32_t pid;
    uint64_t vpn;
} SwapSlot;

// Swap manager
typedef struct {
    uint32_t total_slots;     // Total swap slots available
    uint32_t used_slots;      // Currently used slots
    SwapSlot *slots;          // Slot metadata
    uint32_t *free_list;      // Free slot stack
    uint32_t free_list_top;
    uint64_t swap_in_count;   // Statistics
    uint64_t swap_out_count;
} SwapManager;

// Swap operations
SwapManager *swap_create(uint32_t num_slots);
void swap_destroy(SwapManager *swap);

// Allocate and free swap slots
int32_t swap_alloc(SwapManager *swap, uint32_t pid, uint64_t vpn);
bool swap_free(SwapManager *swap, uint32_t slot);

// Swap I/O simulation (returns simulated latency in microseconds)
uint64_t swap_out(SwapManager *swap, uint32_t slot, void *data);
uint64_t swap_in(SwapManager *swap, uint32_t slot, void *data);

// Statistics
uint32_t swap_get_used_count(SwapManager *swap);
uint32_t swap_get_free_count(SwapManager *swap);

#endif // SWAP_H

