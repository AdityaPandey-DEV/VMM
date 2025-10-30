/**
 * frame.c - Physical frame allocator implementation
 */

#include "frame.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

FrameAllocator *frame_allocator_create(uint32_t num_frames)
{
    FrameAllocator *allocator = malloc(sizeof(FrameAllocator));
    if (!allocator) {
        LOG_ERROR_MSG("Failed to allocate frame allocator");
        return NULL;
    }

    allocator->total_frames = num_frames;
    allocator->free_frames = num_frames;

    // Allocate frame info array
    allocator->frames = calloc(num_frames, sizeof(FrameInfo));
    if (!allocator->frames) {
        LOG_ERROR_MSG("Failed to allocate frame info array");
        free(allocator);
        return NULL;
    }

    // Initialize frame metadata
    for (uint32_t i = 0; i < num_frames; i++) {
        allocator->frames[i].frame_number = i;
        allocator->frames[i].state = FRAME_FREE;
    }

    // Allocate free list (stack of free frame numbers)
    allocator->free_list = malloc(num_frames * sizeof(uint32_t));
    if (!allocator->free_list) {
        LOG_ERROR_MSG("Failed to allocate free list");
        free(allocator->frames);
        free(allocator);
        return NULL;
    }

    // Initialize free list with all frames
    for (uint32_t i = 0; i < num_frames; i++) {
        allocator->free_list[i] = i;
    }
    allocator->free_list_top = num_frames;

    // Allocate bitmap (1 bit per frame)
    uint32_t bitmap_size = (num_frames + 7) / 8;
    allocator->bitmap = calloc(bitmap_size, 1);
    if (!allocator->bitmap) {
        LOG_ERROR_MSG("Failed to allocate bitmap");
        free(allocator->free_list);
        free(allocator->frames);
        free(allocator);
        return NULL;
    }

    LOG_INFO_MSG("Frame allocator created: %u frames (%u KB)", num_frames,
                 num_frames * 4); // Assuming 4KB pages
    return allocator;
}

void frame_allocator_destroy(FrameAllocator *allocator)
{
    if (!allocator)
        return;
    free(allocator->bitmap);
    free(allocator->free_list);
    free(allocator->frames);
    free(allocator);
}

int32_t frame_alloc(FrameAllocator *allocator)
{
    if (!allocator || allocator->free_list_top == 0) {
        return -1; // No free frames
    }

    // Pop from free list
    uint32_t frame_num = allocator->free_list[--allocator->free_list_top];
    allocator->free_frames--;

    // Update frame state
    allocator->frames[frame_num].state = FRAME_ALLOCATED;
    allocator->frames[frame_num].reference_bit = 1;
    allocator->frames[frame_num].last_access_time = get_timestamp_us();
    allocator->frames[frame_num].age_counter = 0;

    // Set bitmap bit
    allocator->bitmap[frame_num / 8] |= (1 << (frame_num % 8));

    LOG_TRACE_MSG("Allocated frame %u (free: %u)", frame_num, allocator->free_frames);
    return (int32_t)frame_num;
}

bool frame_free(FrameAllocator *allocator, uint32_t frame_num)
{
    if (!allocator || frame_num >= allocator->total_frames) {
        return false;
    }

    if (allocator->frames[frame_num].state == FRAME_FREE) {
        LOG_WARN_MSG("Attempting to free already-free frame %u", frame_num);
        return false;
    }

    // Clear frame metadata
    allocator->frames[frame_num].state = FRAME_FREE;
    allocator->frames[frame_num].pid = 0;
    allocator->frames[frame_num].vpn = 0;
    allocator->frames[frame_num].reference_bit = 0;
    allocator->frames[frame_num].dirty = false;
    allocator->frames[frame_num].pin_count = 0;

    // Clear bitmap bit
    allocator->bitmap[frame_num / 8] &= ~(1 << (frame_num % 8));

    // Push to free list
    allocator->free_list[allocator->free_list_top++] = frame_num;
    allocator->free_frames++;

    LOG_TRACE_MSG("Freed frame %u (free: %u)", frame_num, allocator->free_frames);
    return true;
}

FrameInfo *frame_get_info(FrameAllocator *allocator, uint32_t frame_num)
{
    if (!allocator || frame_num >= allocator->total_frames) {
        return NULL;
    }
    return &allocator->frames[frame_num];
}

bool frame_is_free(FrameAllocator *allocator, uint32_t frame_num)
{
    if (!allocator || frame_num >= allocator->total_frames) {
        return false;
    }
    return allocator->frames[frame_num].state == FRAME_FREE;
}

uint32_t frame_get_free_count(FrameAllocator *allocator)
{
    return allocator ? allocator->free_frames : 0;
}

void frame_set_pid(FrameAllocator *allocator, uint32_t frame_num, uint32_t pid)
{
    if (allocator && frame_num < allocator->total_frames) {
        allocator->frames[frame_num].pid = pid;
    }
}

void frame_set_vpn(FrameAllocator *allocator, uint32_t frame_num, uint64_t vpn)
{
    if (allocator && frame_num < allocator->total_frames) {
        allocator->frames[frame_num].vpn = vpn;
    }
}

void frame_set_dirty(FrameAllocator *allocator, uint32_t frame_num, bool dirty)
{
    if (allocator && frame_num < allocator->total_frames) {
        allocator->frames[frame_num].dirty = dirty;
    }
}

void frame_set_reference(FrameAllocator *allocator, uint32_t frame_num, bool referenced)
{
    if (allocator && frame_num < allocator->total_frames) {
        allocator->frames[frame_num].reference_bit = referenced ? 1 : 0;
    }
}

void frame_update_access_time(FrameAllocator *allocator, uint32_t frame_num)
{
    if (allocator && frame_num < allocator->total_frames) {
        allocator->frames[frame_num].last_access_time = get_timestamp_us();
        allocator->frames[frame_num].reference_bit = 1;
    }
}

void frame_age_all(FrameAllocator *allocator)
{
    if (!allocator)
        return;

    for (uint32_t i = 0; i < allocator->total_frames; i++) {
        if (allocator->frames[i].state == FRAME_ALLOCATED) {
            // Shift age counter right and add reference bit to MSB
            allocator->frames[i].age_counter >>= 1;
            if (allocator->frames[i].reference_bit) {
                allocator->frames[i].age_counter |= 0x80000000;
                allocator->frames[i].reference_bit = 0;
            }
        }
    }
}

