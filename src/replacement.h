/**
 * replacement.h - Page replacement algorithms
 * 
 * Implements FIFO, LRU (exact and approximate), Clock (Second-Chance), and OPT.
 * Provides a unified interface for victim selection.
 */

#ifndef REPLACEMENT_H
#define REPLACEMENT_H

#include <stdint.h>
#include <stdbool.h>
#include "frame.h"

// Replacement algorithms
typedef enum {
    REPLACE_FIFO,  // First-In-First-Out
    REPLACE_LRU,   // Least Recently Used (exact)
    REPLACE_APPROX_LRU, // Approximate LRU with aging
    REPLACE_CLOCK, // Clock (Second-Chance)
    REPLACE_OPT    // Optimal (requires future knowledge from trace)
} ReplacementAlgorithm;

// Forward declaration for trace access (OPT needs future references)
typedef struct Trace Trace;

// Replacement policy state
typedef struct {
    ReplacementAlgorithm algorithm;
    
    // FIFO state
    uint32_t *fifo_queue;
    uint32_t fifo_head;
    uint32_t fifo_tail;
    uint32_t fifo_size;
    
    // Clock state
    uint32_t clock_hand;
    
    // OPT state
    Trace *trace;              // Pointer to trace for future reference
    uint64_t current_index;    // Current position in trace
    
} ReplacementPolicy;

// Policy creation and destruction
ReplacementPolicy *replacement_create(ReplacementAlgorithm algo, uint32_t num_frames);
void replacement_destroy(ReplacementPolicy *policy);

// Set trace for OPT algorithm
void replacement_set_trace(ReplacementPolicy *policy, Trace *trace);
void replacement_set_position(ReplacementPolicy *policy, uint64_t index);

// Victim selection - returns frame number to evict
int32_t replacement_select_victim(ReplacementPolicy *policy, FrameAllocator *allocator);

// Update policy state on memory access
void replacement_on_access(ReplacementPolicy *policy, uint32_t frame_num, FrameAllocator *allocator);
void replacement_on_allocate(ReplacementPolicy *policy, uint32_t frame_num);
void replacement_on_free(ReplacementPolicy *policy, uint32_t frame_num);

// Algorithm name for reporting
const char *replacement_get_name(ReplacementAlgorithm algo);

#endif // REPLACEMENT_H

