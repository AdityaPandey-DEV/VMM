/**
 * frame.h - Physical frame allocator
 * 
 * Manages physical memory frames with free-list and bitmap tracking.
 * Supports efficient allocation/deallocation and frame state tracking.
 */

#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdbool.h>

// Frame states
typedef enum {
    FRAME_FREE = 0,
    FRAME_ALLOCATED = 1,
    FRAME_RESERVED = 2
} FrameState;

// Frame metadata
typedef struct {
    uint32_t frame_number;
    uint32_t pid;              // Process ID using this frame
    uint64_t vpn;              // Virtual page number mapped to this frame
    FrameState state;
    uint32_t reference_bit;    // For Clock algorithm
    uint32_t age_counter;      // For aging/approximate LRU
    uint64_t last_access_time; // For exact LRU
    bool dirty;                // Modified bit
    uint32_t pin_count;        // Reference count (for future shared memory)
} FrameInfo;

// Frame allocator
typedef struct {
    uint32_t total_frames;
    uint32_t free_frames;
    FrameInfo *frames;         // Array of frame metadata
    uint32_t *free_list;       // Free frame stack
    uint32_t free_list_top;
    uint8_t *bitmap;           // Bitmap for quick free/used check
} FrameAllocator;

// Initialize frame allocator
FrameAllocator *frame_allocator_create(uint32_t num_frames);
void frame_allocator_destroy(FrameAllocator *allocator);

// Allocate and free frames
int32_t frame_alloc(FrameAllocator *allocator);
bool frame_free(FrameAllocator *allocator, uint32_t frame_num);

// Frame information
FrameInfo *frame_get_info(FrameAllocator *allocator, uint32_t frame_num);
bool frame_is_free(FrameAllocator *allocator, uint32_t frame_num);
uint32_t frame_get_free_count(FrameAllocator *allocator);

// Frame state management
void frame_set_pid(FrameAllocator *allocator, uint32_t frame_num, uint32_t pid);
void frame_set_vpn(FrameAllocator *allocator, uint32_t frame_num, uint64_t vpn);
void frame_set_dirty(FrameAllocator *allocator, uint32_t frame_num, bool dirty);
void frame_set_reference(FrameAllocator *allocator, uint32_t frame_num, bool referenced);
void frame_update_access_time(FrameAllocator *allocator, uint32_t frame_num);

// Aging for approximate LRU
void frame_age_all(FrameAllocator *allocator);

#endif // FRAME_H

