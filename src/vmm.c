/**
 * vmm.c - Virtual Memory Manager core implementation
 * 
 * Authors: Aditya Pandey, Kartik, Vivek, Gaurang
 * Copyright (c) 2025
 */

#include "vmm.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

void vmm_config_init_default(VMMConfig *config)
{
    if (!config)
        return;

    memset(config, 0, sizeof(VMMConfig));

    // Memory configuration
    config->ram_size_mb = 64;
    config->page_size = 4096;
    config->num_frames = (config->ram_size_mb * 1024 * 1024) / config->page_size;
    config->virtual_addr_space = 1ULL << 32; // 4GB virtual space

    // TLB configuration
    config->tlb_size = 64;
    config->tlb_policy = TLB_LRU;

    // Page table configuration
    config->pt_type = PT_SINGLE_LEVEL;

    // Replacement algorithm
    config->replacement_algo = REPLACE_CLOCK;

    // Swap configuration
    config->swap_size_mb = 256;

    // Simulation parameters
    config->max_processes = 16;
    config->max_instructions = UINT64_MAX;
    config->random_seed = 42;

    // Access times (typical values)
    config->access_times.tlb_hit_time_ns = 1;
    config->access_times.memory_access_time_ns = 100;
    config->access_times.page_fault_time_us = 1000;
    config->access_times.swap_io_time_us = 5000;

    // Verbosity
    config->verbose = false;
    config->debug = false;
}

void vmm_config_print(VMMConfig *config, FILE *out)
{
    if (!config || !out)
        return;

    fprintf(out, "VMM Configuration:\n");
    fprintf(out, "  RAM:              %u MB (%u frames)\n", config->ram_size_mb,
            config->num_frames);
    fprintf(out, "  Page size:        %u bytes\n", config->page_size);
    fprintf(out, "  Virtual space:    %lu bytes (%.1f GB)\n", config->virtual_addr_space,
            config->virtual_addr_space / (1024.0 * 1024 * 1024));
    fprintf(out, "  TLB:              %u entries (%s)\n", config->tlb_size,
            config->tlb_policy == TLB_FIFO ? "FIFO" : "LRU");
    fprintf(out, "  Page table:       %s\n",
            config->pt_type == PT_SINGLE_LEVEL ? "Single-level" : "Two-level");
    fprintf(out, "  Replacement:      %s\n",
            replacement_get_name(config->replacement_algo));
    fprintf(out, "  Swap:             %u MB\n", config->swap_size_mb);
    fprintf(out, "  Max processes:    %u\n", config->max_processes);
}

VMM *vmm_create(VMMConfig *config)
{
    if (!config) {
        LOG_ERROR_MSG("Invalid VMM configuration");
        return NULL;
    }

    VMM *vmm = calloc(1, sizeof(VMM));
    if (!vmm) {
        LOG_ERROR_MSG("Failed to allocate VMM");
        return NULL;
    }

    memcpy(&vmm->config, config, sizeof(VMMConfig));

    // Create frame allocator
    vmm->frame_allocator = frame_allocator_create(config->num_frames);
    if (!vmm->frame_allocator) {
        LOG_ERROR_MSG("Failed to create frame allocator");
        vmm_destroy(vmm);
        return NULL;
    }

    // Create TLB
    vmm->tlb = tlb_create(config->tlb_size, config->tlb_policy);
    if (!vmm->tlb) {
        LOG_ERROR_MSG("Failed to create TLB");
        vmm_destroy(vmm);
        return NULL;
    }

    // Create swap manager
    uint32_t swap_slots = (config->swap_size_mb * 1024 * 1024) / config->page_size;
    vmm->swap = swap_create(swap_slots);
    if (!vmm->swap) {
        LOG_ERROR_MSG("Failed to create swap manager");
        vmm_destroy(vmm);
        return NULL;
    }

    // Create replacement policy
    vmm->replacement_policy = replacement_create(config->replacement_algo, config->num_frames);
    if (!vmm->replacement_policy) {
        LOG_ERROR_MSG("Failed to create replacement policy");
        vmm_destroy(vmm);
        return NULL;
    }

    // Create metrics
    vmm->metrics = metrics_create(config->max_processes);
    if (!vmm->metrics) {
        LOG_ERROR_MSG("Failed to create metrics");
        vmm_destroy(vmm);
        return NULL;
    }

    // Allocate process array
    vmm->processes = calloc(config->max_processes, sizeof(Process));
    if (!vmm->processes) {
        LOG_ERROR_MSG("Failed to allocate process array");
        vmm_destroy(vmm);
        return NULL;
    }

    LOG_INFO_MSG("VMM created successfully");
    return vmm;
}

void vmm_destroy(VMM *vmm)
{
    if (!vmm)
        return;

    // Destroy all process page tables
    for (uint32_t i = 0; i < vmm->num_processes; i++) {
        if (vmm->processes[i].page_table) {
            pagetable_destroy(vmm->processes[i].page_table);
        }
    }

    free(vmm->processes);
    metrics_destroy(vmm->metrics);
    replacement_destroy(vmm->replacement_policy);
    swap_destroy(vmm->swap);
    tlb_destroy(vmm->tlb);
    frame_allocator_destroy(vmm->frame_allocator);
    free(vmm);

    LOG_INFO_MSG("VMM destroyed");
}

bool vmm_add_process(VMM *vmm, uint32_t pid)
{
    if (!vmm || vmm->num_processes >= vmm->config.max_processes) {
        LOG_ERROR_MSG("Cannot add process: limit reached");
        return false;
    }

    // Check if process already exists
    for (uint32_t i = 0; i < vmm->num_processes; i++) {
        if (vmm->processes[i].pid == pid) {
            LOG_WARN_MSG("Process %u already exists", pid);
            return true;
        }
    }

    // Create page table for process
    PageTable *pt = pagetable_create(pid, vmm->config.pt_type, vmm->config.virtual_addr_space,
                                      vmm->config.page_size);
    if (!pt) {
        LOG_ERROR_MSG("Failed to create page table for PID %u", pid);
        return false;
    }

    vmm->processes[vmm->num_processes].pid = pid;
    vmm->processes[vmm->num_processes].page_table = pt;
    vmm->processes[vmm->num_processes].active = true;
    vmm->num_processes++;

    LOG_INFO_MSG("Added process %u", pid);
    return true;
}

Process *vmm_get_process(VMM *vmm, uint32_t pid)
{
    if (!vmm)
        return NULL;

    for (uint32_t i = 0; i < vmm->num_processes; i++) {
        if (vmm->processes[i].pid == pid) {
            return &vmm->processes[i];
        }
    }

    return NULL;
}

// Handle page fault
static bool vmm_handle_page_fault(VMM *vmm, Process *proc, uint64_t virtual_addr, bool is_write)
{
    LOG_DEBUG_MSG("Page fault: PID=%u, addr=0x%lx, %s", proc->pid, virtual_addr,
                  is_write ? "WRITE" : "READ");

    uint64_t vpn = virtual_addr / vmm->config.page_size;
    PageTableEntry *pte = pagetable_lookup(proc->page_table, virtual_addr);

    if (!pte) {
        LOG_ERROR_MSG("Invalid address: 0x%lx", virtual_addr);
        return false;
    }

    bool is_major_fault = false;

    // Try to allocate a frame
    int32_t frame_num = frame_alloc(vmm->frame_allocator);

    // If no free frames, select victim and evict
    if (frame_num < 0) {
        LOG_DEBUG_MSG("No free frames, selecting victim");
        frame_num = replacement_select_victim(vmm->replacement_policy, vmm->frame_allocator);

        if (frame_num < 0) {
            LOG_ERROR_MSG("Failed to select victim frame");
            return false;
        }

        // Evict victim frame
        FrameInfo *victim_frame = frame_get_info(vmm->frame_allocator, frame_num);
        if (victim_frame) {
            // Find victim's page table entry and invalidate
            Process *victim_proc = vmm_get_process(vmm, victim_frame->pid);
            if (victim_proc) {
                uint64_t victim_addr = victim_frame->vpn * vmm->config.page_size;
                PageTableEntry *victim_pte =
                    pagetable_lookup(victim_proc->page_table, victim_addr);
                if (victim_pte) {
                    // If dirty, swap out
                    if (victim_frame->dirty) {
                        int32_t swap_slot =
                            swap_alloc(vmm->swap, victim_frame->pid, victim_frame->vpn);
                        if (swap_slot >= 0) {
                            swap_out(vmm->swap, swap_slot, NULL);
                            metrics_record_swap_out(vmm->metrics);
                            victim_pte->swap_offset = swap_slot;
                        }
                    }
                    pte_set_valid(victim_pte, false);

                    // Invalidate TLB entry
                    tlb_invalidate(vmm->tlb, victim_frame->pid, victim_frame->vpn);
                }
            }
        }

        metrics_record_replacement(vmm->metrics);
    }

    // Check if page is in swap
    if (pte->swap_offset > 0) {
        // Swap in
        swap_in(vmm->swap, pte->swap_offset, NULL);
        metrics_record_swap_in(vmm->metrics);
        swap_free(vmm->swap, pte->swap_offset);
        pte->swap_offset = 0;
        is_major_fault = true;
    }

    // Map page to frame
    uint32_t flags = PTE_VALID | PTE_USER;
    if (is_write) {
        flags |= PTE_WRITE;
    }

    pagetable_map(proc->page_table, virtual_addr, frame_num, flags);

    // Update frame metadata
    frame_set_pid(vmm->frame_allocator, frame_num, proc->pid);
    frame_set_vpn(vmm->frame_allocator, frame_num, vpn);
    frame_set_dirty(vmm->frame_allocator, frame_num, is_write);

    // Notify replacement policy
    replacement_on_allocate(vmm->replacement_policy, frame_num);

    // Update metrics
    metrics_record_page_fault(vmm->metrics, proc->pid, is_major_fault);

    LOG_DEBUG_MSG("Page fault handled: allocated frame %d", frame_num);
    return true;
}

bool vmm_access(VMM *vmm, uint32_t pid, uint64_t virtual_addr, bool is_write)
{
    if (!vmm) {
        return false;
    }

    // Find or create process
    Process *proc = vmm_get_process(vmm, pid);
    if (!proc) {
        if (!vmm_add_process(vmm, pid)) {
            return false;
        }
        proc = vmm_get_process(vmm, pid);
    }

    if (!proc) {
        LOG_ERROR_MSG("Failed to get process %u", pid);
        return false;
    }

    // Record access
    metrics_record_access(vmm->metrics, pid, is_write);

    uint64_t vpn = virtual_addr / vmm->config.page_size;
    uint32_t pfn;

    // Step 1: TLB lookup
    if (tlb_lookup(vmm->tlb, pid, vpn, &pfn)) {
        // TLB hit
        metrics_record_tlb_hit(vmm->metrics, pid);

        // Update replacement policy
        replacement_on_access(vmm->replacement_policy, pfn, vmm->frame_allocator);

        // Update dirty bit if write
        if (is_write) {
            frame_set_dirty(vmm->frame_allocator, pfn, true);
            PageTableEntry *pte = pagetable_lookup(proc->page_table, virtual_addr);
            if (pte) {
                pte_set_dirty(pte, true);
            }
        }

        LOG_TRACE_MSG("Access: PID=%u, addr=0x%lx, TLB HIT -> frame %u", pid, virtual_addr, pfn);
        return true;
    }

    // TLB miss
    metrics_record_tlb_miss(vmm->metrics, pid);

    // Step 2: Page table lookup
    PageTableEntry *pte = pagetable_lookup(proc->page_table, virtual_addr);
    if (!pte) {
        LOG_ERROR_MSG("Invalid virtual address: 0x%lx", virtual_addr);
        return false;
    }

    if (pte_is_valid(pte)) {
        // Page is in memory, update TLB
        pfn = pte->frame_number;
        tlb_insert(vmm->tlb, pid, vpn, pfn);

        // Update replacement policy
        replacement_on_access(vmm->replacement_policy, pfn, vmm->frame_allocator);

        // Update dirty bit if write
        if (is_write) {
            frame_set_dirty(vmm->frame_allocator, pfn, true);
            pte_set_dirty(pte, true);
        }

        LOG_TRACE_MSG("Access: PID=%u, addr=0x%lx, PT HIT -> frame %u", pid, virtual_addr, pfn);
        return true;
    }

    // Step 3: Page fault
    if (!vmm_handle_page_fault(vmm, proc, virtual_addr, is_write)) {
        return false;
    }

    // Update TLB with new mapping
    pte = pagetable_lookup(proc->page_table, virtual_addr);
    if (pte && pte_is_valid(pte)) {
        tlb_insert(vmm->tlb, pid, vpn, pte->frame_number);
    }

    return true;
}

bool vmm_run_trace(VMM *vmm, Trace *trace)
{
    if (!vmm || !trace) {
        return false;
    }

    LOG_INFO_MSG("Running trace with %lu entries", trace->count);

    // Set trace for OPT algorithm
    if (vmm->replacement_policy->algorithm == REPLACE_OPT) {
        replacement_set_trace(vmm->replacement_policy, trace);
    }

    metrics_start_simulation(vmm->metrics);

    uint64_t max_accesses =
        (vmm->config.max_instructions < trace->count) ? vmm->config.max_instructions : trace->count;

    for (uint64_t i = 0; i < max_accesses; i++) {
        TraceEntry *entry = trace_get(trace, i);
        if (!entry) {
            break;
        }

        // Update OPT position
        if (vmm->replacement_policy->algorithm == REPLACE_OPT) {
            replacement_set_position(vmm->replacement_policy, i);
        }

        bool success = vmm_access(vmm, entry->pid, entry->virtual_addr, entry->op == OP_WRITE);
        if (!success) {
            LOG_WARN_MSG("Failed to access memory at index %lu", i);
        }

        // Periodic aging for approximate LRU
        if (vmm->replacement_policy->algorithm == REPLACE_APPROX_LRU && i % 1000 == 0) {
            frame_age_all(vmm->frame_allocator);
        }

        // Progress indicator
        if (vmm->config.verbose && i > 0 && i % 10000 == 0) {
            fprintf(stderr, "Progress: %lu / %lu accesses (%.1f%%)\r", i, max_accesses,
                    100.0 * i / max_accesses);
        }
    }

    if (vmm->config.verbose) {
        fprintf(stderr, "\n");
    }

    metrics_end_simulation(vmm->metrics);

    LOG_INFO_MSG("Trace execution completed");
    return true;
}

