/**
 * trace.c - Memory trace parsing and generation
 */

#include "trace.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

Trace *trace_create(uint64_t initial_capacity)
{
    Trace *trace = malloc(sizeof(Trace));
    if (!trace) {
        LOG_ERROR_MSG("Failed to allocate trace");
        return NULL;
    }

    trace->entries = calloc(initial_capacity, sizeof(TraceEntry));
    if (!trace->entries) {
        LOG_ERROR_MSG("Failed to allocate trace entries");
        free(trace);
        return NULL;
    }

    trace->count = 0;
    trace->capacity = initial_capacity;
    trace->filename = NULL;

    return trace;
}

void trace_destroy(Trace *trace)
{
    if (!trace)
        return;
    free(trace->entries);
    free(trace->filename);
    free(trace);
}

Trace *trace_load(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        LOG_ERROR_MSG("Failed to open trace file: %s", filename);
        return NULL;
    }

    Trace *trace = trace_create(10000);
    if (!trace) {
        fclose(fp);
        return NULL;
    }

    trace->filename = strdup(filename);

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        uint32_t pid;
        char op;
        uint64_t addr;

        if (sscanf(line, "%u %c 0x%lx", &pid, &op, &addr) == 3 ||
            sscanf(line, "%u %c %lu", &pid, &op, &addr) == 3) {
            MemoryOperation mem_op = (op == 'W' || op == 'w') ? OP_WRITE : OP_READ;
            trace_add(trace, pid, mem_op, addr);
        }
    }

    fclose(fp);
    LOG_INFO_MSG("Loaded trace from %s: %lu entries", filename, trace->count);
    return trace;
}

bool trace_save(Trace *trace, const char *filename)
{
    if (!trace)
        return false;

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        LOG_ERROR_MSG("Failed to create trace file: %s", filename);
        return false;
    }

    for (uint64_t i = 0; i < trace->count; i++) {
        fprintf(fp, "%u %c 0x%lx\n", trace->entries[i].pid,
                trace->entries[i].op == OP_WRITE ? 'W' : 'R', trace->entries[i].virtual_addr);
    }

    fclose(fp);
    LOG_INFO_MSG("Saved trace to %s: %lu entries", filename, trace->count);
    return true;
}

bool trace_add(Trace *trace, uint32_t pid, MemoryOperation op, uint64_t virtual_addr)
{
    if (!trace)
        return false;

    // Resize if needed
    if (trace->count >= trace->capacity) {
        uint64_t new_capacity = trace->capacity * 2;
        TraceEntry *new_entries = realloc(trace->entries, new_capacity * sizeof(TraceEntry));
        if (!new_entries) {
            LOG_ERROR_MSG("Failed to resize trace");
            return false;
        }
        trace->entries = new_entries;
        trace->capacity = new_capacity;
    }

    trace->entries[trace->count].pid = pid;
    trace->entries[trace->count].op = op;
    trace->entries[trace->count].virtual_addr = virtual_addr;
    trace->count++;

    return true;
}

TraceEntry *trace_get(Trace *trace, uint64_t index)
{
    if (!trace || index >= trace->count)
        return NULL;
    return &trace->entries[index];
}

// Random number generator state (for reproducibility)
static uint64_t rng_state = 12345;

static uint64_t rand_next(void)
{
    // Simple LCG
    rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return rng_state;
}

static void rand_seed(uint32_t seed)
{
    rng_state = seed;
}

Trace *trace_generate(TracePattern pattern, uint64_t num_accesses, uint32_t num_processes,
                      uint64_t address_space_size, uint32_t seed)
{
    rand_seed(seed);

    Trace *trace = trace_create(num_accesses);
    if (!trace)
        return NULL;

    LOG_INFO_MSG("Generating trace: pattern=%d, accesses=%lu, processes=%u", pattern,
                 num_accesses, num_processes);

    switch (pattern) {
    case PATTERN_SEQUENTIAL: {
        uint64_t addr = 0;
        for (uint64_t i = 0; i < num_accesses; i++) {
            uint32_t pid = (i / 100) % num_processes;
            MemoryOperation op = (rand_next() % 4 == 0) ? OP_WRITE : OP_READ;
            trace_add(trace, pid, op, addr);
            addr = (addr + 4096) % address_space_size; // Next page
        }
        break;
    }

    case PATTERN_RANDOM: {
        for (uint64_t i = 0; i < num_accesses; i++) {
            uint32_t pid = rand_next() % num_processes;
            MemoryOperation op = (rand_next() % 4 == 0) ? OP_WRITE : OP_READ;
            uint64_t addr = (rand_next() % (address_space_size / 4096)) * 4096;
            trace_add(trace, pid, op, addr);
        }
        break;
    }

    case PATTERN_WORKING_SET: {
        // Each process has a working set that slowly shifts
        uint64_t working_set_size = 64 * 4096; // 64 pages
        uint64_t *working_set_base = calloc(num_processes, sizeof(uint64_t));

        for (uint64_t i = 0; i < num_accesses; i++) {
            uint32_t pid = i % num_processes;
            MemoryOperation op = (rand_next() % 5 == 0) ? OP_WRITE : OP_READ;

            // 90% within working set, 10% outside
            uint64_t addr;
            if (rand_next() % 10 < 9) {
                addr = working_set_base[pid] + (rand_next() % working_set_size);
            } else {
                addr = rand_next() % address_space_size;
            }
            addr = (addr / 4096) * 4096; // Align to page

            trace_add(trace, pid, op, addr);

            // Slowly shift working set
            if (i % 500 == 0) {
                working_set_base[pid] =
                    (working_set_base[pid] + 4096) % (address_space_size - working_set_size);
            }
        }
        free(working_set_base);
        break;
    }

    case PATTERN_LOCALITY: {
        // Temporal and spatial locality
        uint64_t current_addr = 0;
        for (uint64_t i = 0; i < num_accesses; i++) {
            uint32_t pid = (i / 50) % num_processes;
            MemoryOperation op = (rand_next() % 4 == 0) ? OP_WRITE : OP_READ;

            // 70% nearby, 30% random jump
            if (rand_next() % 10 < 7) {
                // Nearby access (within 16 pages)
                int64_t offset = (int64_t)(rand_next() % 65536) - 32768;
                current_addr = (current_addr + offset) % address_space_size;
            } else {
                // Random jump
                current_addr = rand_next() % address_space_size;
            }
            current_addr = (current_addr / 4096) * 4096;

            trace_add(trace, pid, op, current_addr);
        }
        break;
    }

    case PATTERN_THRASHING: {
        // Access more pages than can fit in RAM, cycling through them
        uint32_t num_pages = 512; // More than typical RAM can hold
        for (uint64_t i = 0; i < num_accesses; i++) {
            uint32_t pid = i % num_processes;
            MemoryOperation op = OP_READ;
            uint64_t page = (i / num_processes) % num_pages;
            uint64_t addr = page * 4096;
            trace_add(trace, pid, op, addr);
        }
        break;
    }

    default:
        LOG_ERROR_MSG("Unknown trace pattern");
        trace_destroy(trace);
        return NULL;
    }

    LOG_INFO_MSG("Generated trace: %lu entries", trace->count);
    return trace;
}

