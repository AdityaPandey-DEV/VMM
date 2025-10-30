/**
 * trace_gen.c - Trace generator utility
 * 
 * Standalone tool for generating synthetic memory access traces.
 */

#include "trace.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void print_usage(const char *prog_name)
{
    fprintf(stderr, "Usage: %s [OPTIONS]\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Memory Access Trace Generator\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Required:\n");
    fprintf(stderr, "  -o, --output FILE      Output trace file\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -t, --type PATTERN     Trace pattern (default: sequential):\n");
    fprintf(stderr, "                         sequential, random, working_set, locality, thrashing\n");
    fprintf(stderr, "  -n, --num-accesses N   Number of memory accesses (default: 10000)\n");
    fprintf(stderr, "  -p, --num-processes N  Number of processes (default: 4)\n");
    fprintf(stderr, "  -a, --addr-space SIZE  Virtual address space in MB (default: 1024)\n");
    fprintf(stderr, "  -s, --seed SEED        Random seed (default: 42)\n");
    fprintf(stderr, "  -h, --help             Show this help\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s -t sequential -n 1000 -o sequential.trace\n", prog_name);
    fprintf(stderr, "  %s -t working_set -n 10000 -p 8 -o working_set.trace\n", prog_name);
    fprintf(stderr, "  %s -t thrashing -n 20000 -o thrashing.trace\n", prog_name);
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
    const char *output_file = NULL;
    TracePattern pattern = PATTERN_SEQUENTIAL;
    uint64_t num_accesses = 10000;
    uint32_t num_processes = 4;
    uint64_t addr_space_mb = 1024;
    uint32_t seed = 42;

    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"type", required_argument, 0, 't'},
        {"num-accesses", required_argument, 0, 'n'},
        {"num-processes", required_argument, 0, 'p'},
        {"addr-space", required_argument, 0, 'a'},
        {"seed", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "o:t:n:p:a:s:h", long_options, &option_index)) != -1) {
        switch (opt) {
        case 'o':
            output_file = optarg;
            break;
        case 't':
            if (strcasecmp(optarg, "sequential") == 0)
                pattern = PATTERN_SEQUENTIAL;
            else if (strcasecmp(optarg, "random") == 0)
                pattern = PATTERN_RANDOM;
            else if (strcasecmp(optarg, "working_set") == 0)
                pattern = PATTERN_WORKING_SET;
            else if (strcasecmp(optarg, "locality") == 0)
                pattern = PATTERN_LOCALITY;
            else if (strcasecmp(optarg, "thrashing") == 0)
                pattern = PATTERN_THRASHING;
            else {
                fprintf(stderr, "Unknown pattern: %s\n", optarg);
                return 1;
            }
            break;
        case 'n':
            num_accesses = strtoull(optarg, NULL, 10);
            break;
        case 'p':
            num_processes = atoi(optarg);
            break;
        case 'a':
            addr_space_mb = strtoull(optarg, NULL, 10);
            break;
        case 's':
            seed = atoi(optarg);
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!output_file) {
        fprintf(stderr, "Error: Output file is required\n\n");
        print_usage(argv[0]);
        return 1;
    }

    printf("Generating trace:\n");
    printf("  Pattern:       %d\n", pattern);
    printf("  Accesses:      %lu\n", num_accesses);
    printf("  Processes:     %u\n", num_processes);
    printf("  Addr space:    %lu MB\n", addr_space_mb);
    printf("  Seed:          %u\n", seed);
    printf("  Output:        %s\n", output_file);
    printf("\n");

    uint64_t addr_space_bytes = addr_space_mb * 1024 * 1024;
    Trace *trace = trace_generate(pattern, num_accesses, num_processes, addr_space_bytes, seed);
    if (!trace) {
        fprintf(stderr, "Error: Failed to generate trace\n");
        return 1;
    }

    if (!trace_save(trace, output_file)) {
        fprintf(stderr, "Error: Failed to save trace\n");
        trace_destroy(trace);
        return 1;
    }

    trace_destroy(trace);
    printf("Trace generated successfully: %lu entries\n", num_accesses);
    return 0;
}

