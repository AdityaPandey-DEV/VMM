/**
 * replacement.c - Page replacement algorithms implementation
 */

#include "replacement.h"
#include "trace.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

ReplacementPolicy *replacement_create(ReplacementAlgorithm algo, uint32_t num_frames)
{
    ReplacementPolicy *policy = calloc(1, sizeof(ReplacementPolicy));
    if (!policy) {
        LOG_ERROR_MSG("Failed to allocate replacement policy");
        return NULL;
    }

    policy->algorithm = algo;

    // Initialize algorithm-specific state
    if (algo == REPLACE_FIFO) {
        policy->fifo_queue = calloc(num_frames, sizeof(uint32_t));
        if (!policy->fifo_queue) {
            LOG_ERROR_MSG("Failed to allocate FIFO queue");
            free(policy);
            return NULL;
        }
        policy->fifo_size = 0;
        policy->fifo_head = 0;
        policy->fifo_tail = 0;
    } else if (algo == REPLACE_CLOCK) {
        policy->clock_hand = 0;
    }

    LOG_INFO_MSG("Replacement policy created: %s", replacement_get_name(algo));
    return policy;
}

void replacement_destroy(ReplacementPolicy *policy)
{
    if (!policy)
        return;
    if (policy->fifo_queue) {
        free(policy->fifo_queue);
    }
    free(policy);
}

void replacement_set_trace(ReplacementPolicy *policy, Trace *trace)
{
    if (policy && policy->algorithm == REPLACE_OPT) {
        policy->trace = trace;
    }
}

void replacement_set_position(ReplacementPolicy *policy, uint64_t index)
{
    if (policy && policy->algorithm == REPLACE_OPT) {
        policy->current_index = index;
    }
}

// Helper for OPT: find next use of a frame in trace
static uint64_t find_next_use(ReplacementPolicy *policy, uint32_t frame_num,
                               FrameAllocator *allocator)
{
    if (!policy->trace || policy->current_index >= policy->trace->count) {
        return UINT64_MAX; // Never used again
    }

    FrameInfo *frame = frame_get_info(allocator, frame_num);
    if (!frame) {
        return UINT64_MAX;
    }

    // Search forward in trace for next access to this page
    for (uint64_t i = policy->current_index + 1; i < policy->trace->count; i++) {
        TraceEntry *entry = trace_get(policy->trace, i);
        if (entry && entry->pid == frame->pid) {
            uint64_t vpn = entry->virtual_addr / 4096; // Assuming 4KB pages
            if (vpn == frame->vpn) {
                return i;
            }
        }
    }

    return UINT64_MAX; // Not found in remaining trace
}

int32_t replacement_select_victim(ReplacementPolicy *policy, FrameAllocator *allocator)
{
    if (!policy || !allocator) {
        return -1;
    }

    switch (policy->algorithm) {
    case REPLACE_FIFO: {
        if (policy->fifo_size == 0) {
            LOG_ERROR_MSG("FIFO queue is empty");
            return -1;
        }
        // Victim is at head of queue
        uint32_t victim = policy->fifo_queue[policy->fifo_head];
        policy->fifo_head = (policy->fifo_head + 1) % allocator->total_frames;
        policy->fifo_size--;
        LOG_TRACE_MSG("FIFO victim: frame %u", victim);
        return victim;
    }

    case REPLACE_LRU: {
        // Find frame with minimum last_access_time
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

    case REPLACE_APPROX_LRU: {
        // Find frame with minimum age counter (NFU/Aging)
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

    case REPLACE_CLOCK: {
        // Second-chance algorithm
        uint32_t start = policy->clock_hand;
        while (1) {
            if (allocator->frames[policy->clock_hand].state == FRAME_ALLOCATED) {
                if (allocator->frames[policy->clock_hand].reference_bit == 0) {
                    // Found victim
                    uint32_t victim = policy->clock_hand;
                    policy->clock_hand = (policy->clock_hand + 1) % allocator->total_frames;
                    LOG_TRACE_MSG("Clock victim: frame %u", victim);
                    return victim;
                } else {
                    // Give second chance
                    allocator->frames[policy->clock_hand].reference_bit = 0;
                }
            }
            policy->clock_hand = (policy->clock_hand + 1) % allocator->total_frames;

            // Prevent infinite loop
            if (policy->clock_hand == start) {
                // All frames have reference bit set, take current one
                uint32_t victim = policy->clock_hand;
                policy->clock_hand = (policy->clock_hand + 1) % allocator->total_frames;
                return victim;
            }
        }
    }

    case REPLACE_OPT: {
        // Optimal - replace page that will be used furthest in future
        uint64_t max_next_use = 0;
        int32_t victim = -1;
        for (uint32_t i = 0; i < allocator->total_frames; i++) {
            if (allocator->frames[i].state == FRAME_ALLOCATED) {
                uint64_t next_use = find_next_use(policy, i, allocator);
                if (next_use > max_next_use) {
                    max_next_use = next_use;
                    victim = i;
                }
            }
        }
        LOG_TRACE_MSG("OPT victim: frame %d (next use at %lu)", victim, max_next_use);
        return victim;
    }

    default:
        LOG_ERROR_MSG("Unknown replacement algorithm");
        return -1;
    }
}

void replacement_on_access(ReplacementPolicy *policy, uint32_t frame_num,
                            FrameAllocator *allocator)
{
    if (!policy || !allocator)
        return;

    // Update frame access time for LRU
    if (policy->algorithm == REPLACE_LRU) {
        frame_update_access_time(allocator, frame_num);
    }
    // Set reference bit for Clock and Approx-LRU
    else if (policy->algorithm == REPLACE_CLOCK || policy->algorithm == REPLACE_APPROX_LRU) {
        frame_set_reference(allocator, frame_num, true);
    }
}

void replacement_on_allocate(ReplacementPolicy *policy, uint32_t frame_num)
{
    if (!policy)
        return;

    // Add to FIFO queue
    if (policy->algorithm == REPLACE_FIFO) {
        if (policy->fifo_queue) {
            policy->fifo_queue[policy->fifo_tail] = frame_num;
            policy->fifo_tail = (policy->fifo_tail + 1) % 1024; // Assuming max 1024 frames
            policy->fifo_size++;
        }
    }
}

void replacement_on_free(ReplacementPolicy *policy, uint32_t frame_num)
{
    if (!policy)
        return;

    // Remove from FIFO queue if present
    if (policy->algorithm == REPLACE_FIFO && policy->fifo_queue) {
        // Linear search and remove (simple implementation)
        for (uint32_t i = 0; i < policy->fifo_size; i++) {
            uint32_t idx = (policy->fifo_head + i) % 1024;
            if (policy->fifo_queue[idx] == frame_num) {
                // Shift remaining entries
                for (uint32_t j = i; j < policy->fifo_size - 1; j++) {
                    uint32_t cur_idx = (policy->fifo_head + j) % 1024;
                    uint32_t next_idx = (policy->fifo_head + j + 1) % 1024;
                    policy->fifo_queue[cur_idx] = policy->fifo_queue[next_idx];
                }
                policy->fifo_tail = (policy->fifo_tail - 1 + 1024) % 1024;
                policy->fifo_size--;
                break;
            }
        }
    }
}

const char *replacement_get_name(ReplacementAlgorithm algo)
{
    switch (algo) {
    case REPLACE_FIFO:
        return "FIFO";
    case REPLACE_LRU:
        return "LRU";
    case REPLACE_APPROX_LRU:
        return "Approx-LRU";
    case REPLACE_CLOCK:
        return "Clock";
    case REPLACE_OPT:
        return "OPT";
    default:
        return "Unknown";
    }
}

