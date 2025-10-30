/**
 * tlb.c - TLB implementation
 */

#include "tlb.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

TLB *tlb_create(uint32_t size, TLBPolicy policy)
{
    if (size == 0) {
        LOG_ERROR_MSG("TLB size must be > 0");
        return NULL;
    }

    TLB *tlb = malloc(sizeof(TLB));
    if (!tlb) {
        LOG_ERROR_MSG("Failed to allocate TLB");
        return NULL;
    }

    tlb->size = size;
    tlb->policy = policy;
    tlb->fifo_next = 0;
    tlb->access_counter = 0;

    tlb->entries = calloc(size, sizeof(TLBEntry));
    if (!tlb->entries) {
        LOG_ERROR_MSG("Failed to allocate TLB entries");
        free(tlb);
        return NULL;
    }

    LOG_INFO_MSG("TLB created: %u entries, policy=%s", size,
                 policy == TLB_FIFO ? "FIFO" : "LRU");
    return tlb;
}

void tlb_destroy(TLB *tlb)
{
    if (!tlb)
        return;
    free(tlb->entries);
    free(tlb);
}

bool tlb_lookup(TLB *tlb, uint32_t pid, uint64_t vpn, uint32_t *pfn)
{
    if (!tlb || !pfn)
        return false;

    // Search for matching entry
    for (uint32_t i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].pid == pid && tlb->entries[i].vpn == vpn) {
            *pfn = tlb->entries[i].pfn;

            // Update LRU timestamp
            if (tlb->policy == TLB_LRU) {
                tlb->entries[i].last_use_time = tlb->access_counter++;
            }

            LOG_TRACE_MSG("TLB hit: PID=%u VPN=0x%lx -> PFN=%u", pid, vpn, *pfn);
            return true;
        }
    }

    LOG_TRACE_MSG("TLB miss: PID=%u VPN=0x%lx", pid, vpn);
    return false;
}

void tlb_insert(TLB *tlb, uint32_t pid, uint64_t vpn, uint32_t pfn)
{
    if (!tlb)
        return;

    uint32_t victim_index;

    // Check if entry already exists (update instead of insert)
    for (uint32_t i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].pid == pid && tlb->entries[i].vpn == vpn) {
            tlb->entries[i].pfn = pfn;
            if (tlb->policy == TLB_LRU) {
                tlb->entries[i].last_use_time = tlb->access_counter++;
            }
            return;
        }
    }

    // Find victim based on policy
    if (tlb->policy == TLB_FIFO) {
        victim_index = tlb->fifo_next;
        tlb->fifo_next = (tlb->fifo_next + 1) % tlb->size;
    } else { // TLB_LRU
        // Find entry with minimum last_use_time
        uint64_t min_time = UINT64_MAX;
        victim_index = 0;
        for (uint32_t i = 0; i < tlb->size; i++) {
            if (!tlb->entries[i].valid) {
                victim_index = i;
                break;
            }
            if (tlb->entries[i].last_use_time < min_time) {
                min_time = tlb->entries[i].last_use_time;
                victim_index = i;
            }
        }
    }

    // Insert new entry
    tlb->entries[victim_index].valid = true;
    tlb->entries[victim_index].pid = pid;
    tlb->entries[victim_index].vpn = vpn;
    tlb->entries[victim_index].pfn = pfn;
    tlb->entries[victim_index].last_use_time = tlb->access_counter++;

    LOG_TRACE_MSG("TLB insert: PID=%u VPN=0x%lx -> PFN=%u (index %u)", pid, vpn, pfn,
                  victim_index);
}

void tlb_invalidate(TLB *tlb, uint32_t pid, uint64_t vpn)
{
    if (!tlb)
        return;

    for (uint32_t i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].pid == pid && tlb->entries[i].vpn == vpn) {
            tlb->entries[i].valid = false;
            LOG_TRACE_MSG("TLB invalidate: PID=%u VPN=0x%lx", pid, vpn);
            return;
        }
    }
}

void tlb_invalidate_all(TLB *tlb, uint32_t pid)
{
    if (!tlb)
        return;

    uint32_t count = 0;
    for (uint32_t i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].pid == pid) {
            tlb->entries[i].valid = false;
            count++;
        }
    }
    LOG_DEBUG_MSG("TLB invalidated %u entries for PID %u", count, pid);
}

void tlb_flush(TLB *tlb)
{
    if (!tlb)
        return;

    memset(tlb->entries, 0, tlb->size * sizeof(TLBEntry));
    tlb->fifo_next = 0;
    LOG_DEBUG_MSG("TLB flushed");
}

