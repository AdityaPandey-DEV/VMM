/**
 * trace.h - Memory access trace parsing and generation
 * 
 * Supports reading trace files and generating synthetic traces with various patterns.
 */

#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Memory operation types
typedef enum { OP_READ = 0, OP_WRITE = 1 } MemoryOperation;

// Trace entry
typedef struct {
    uint32_t pid;
    MemoryOperation op;
    uint64_t virtual_addr;
} TraceEntry;

// Trace structure
typedef struct Trace {
    TraceEntry *entries;
    uint64_t count;
    uint64_t capacity;
    char *filename;
} Trace;

// Trace generation patterns
typedef enum {
    PATTERN_SEQUENTIAL,  // Sequential access
    PATTERN_RANDOM,      // Uniform random
    PATTERN_WORKING_SET, // Localized working set
    PATTERN_LOCALITY,    // Temporal/spatial locality
    PATTERN_THRASHING    // Pathological thrashing pattern
} TracePattern;

// Trace operations
Trace *trace_create(uint64_t initial_capacity);
void trace_destroy(Trace *trace);

// Load trace from file (format: "pid op virtual_addr" per line)
Trace *trace_load(const char *filename);

// Save trace to file
bool trace_save(Trace *trace, const char *filename);

// Add entry to trace
bool trace_add(Trace *trace, uint32_t pid, MemoryOperation op, uint64_t virtual_addr);

// Generate synthetic trace
Trace *trace_generate(TracePattern pattern, uint64_t num_accesses, uint32_t num_processes,
                      uint64_t address_space_size, uint32_t seed);

// Get trace entry
TraceEntry *trace_get(Trace *trace, uint64_t index);

#endif // TRACE_H

