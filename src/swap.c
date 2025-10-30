/**
 * swap.c - Swap/backing store implementation
 */

#include "swap.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

// Simulated swap I/O latency (microseconds)
#define SWAP_IO_LATENCY_US 5000

SwapManager *swap_create(uint32_t num_slots)
{
    SwapManager *swap = malloc(sizeof(SwapManager));
    if (!swap) {
        LOG_ERROR_MSG("Failed to allocate swap manager");
        return NULL;
    }

    swap->total_slots = num_slots;
    swap->used_slots = 0;
    swap->swap_in_count = 0;
    swap->swap_out_count = 0;

    swap->slots = calloc(num_slots, sizeof(SwapSlot));
    if (!swap->slots) {
        LOG_ERROR_MSG("Failed to allocate swap slots");
        free(swap);
        return NULL;
    }

    swap->free_list = malloc(num_slots * sizeof(uint32_t));
    if (!swap->free_list) {
        LOG_ERROR_MSG("Failed to allocate swap free list");
        free(swap->slots);
        free(swap);
        return NULL;
    }

    // Initialize free list
    for (uint32_t i = 0; i < num_slots; i++) {
        swap->free_list[i] = i;
    }
    swap->free_list_top = num_slots;

    LOG_INFO_MSG("Swap manager created: %u slots (%u MB)", num_slots, num_slots * 4 / 1024);
    return swap;
}

void swap_destroy(SwapManager *swap)
{
    if (!swap)
        return;
    free(swap->free_list);
    free(swap->slots);
    free(swap);
}

int32_t swap_alloc(SwapManager *swap, uint32_t pid, uint64_t vpn)
{
    if (!swap || swap->free_list_top == 0) {
        LOG_ERROR_MSG("Swap space exhausted");
        return -1;
    }

    uint32_t slot = swap->free_list[--swap->free_list_top];
    swap->slots[slot].used = true;
    swap->slots[slot].pid = pid;
    swap->slots[slot].vpn = vpn;
    swap->used_slots++;

    LOG_TRACE_MSG("Swap allocated slot %u for PID=%u VPN=0x%lx", slot, pid, vpn);
    return (int32_t)slot;
}

bool swap_free(SwapManager *swap, uint32_t slot)
{
    if (!swap || slot >= swap->total_slots || !swap->slots[slot].used) {
        return false;
    }

    swap->slots[slot].used = false;
    swap->free_list[swap->free_list_top++] = slot;
    swap->used_slots--;

    LOG_TRACE_MSG("Swap freed slot %u", slot);
    return true;
}

uint64_t swap_out(SwapManager *swap, uint32_t slot, void *data)
{
    if (!swap || slot >= swap->total_slots) {
        return 0;
    }

    // Simulate writing page to disk (data parameter unused in simulation)
    (void)data;
    swap->swap_out_count++;

    LOG_TRACE_MSG("Swap out to slot %u (total swap-outs: %lu)", slot, swap->swap_out_count);
    return SWAP_IO_LATENCY_US;
}

uint64_t swap_in(SwapManager *swap, uint32_t slot, void *data)
{
    if (!swap || slot >= swap->total_slots) {
        return 0;
    }

    // Simulate reading page from disk (data parameter unused in simulation)
    (void)data;
    swap->swap_in_count++;

    LOG_TRACE_MSG("Swap in from slot %u (total swap-ins: %lu)", slot, swap->swap_in_count);
    return SWAP_IO_LATENCY_US;
}

uint32_t swap_get_used_count(SwapManager *swap)
{
    return swap ? swap->used_slots : 0;
}

uint32_t swap_get_free_count(SwapManager *swap)
{
    return swap ? (swap->total_slots - swap->used_slots) : 0;
}

