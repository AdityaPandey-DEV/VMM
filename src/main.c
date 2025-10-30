/**
 * main.c - VMM simulator entry point
 * 
 * Command-line interface for the Virtual Memory Manager simulator.
 * 
 * Authors: Aditya Pandey, Kartik, Vivek, Gaurang
 * Copyright (c) 2025
 */

#include "vmm.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void print_usage(const char *prog_name)
{
    fprintf(stderr, "Usage: %s [OPTIONS]\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Virtual Memory Manager Simulator\n");
    fprintf(stderr, "Authors: Aditya Pandey, Kartik, Vivek, Gaurang\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Required:\n");
    fprintf(stderr, "  -t, --trace FILE       Input trace file (format: pid op addr)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Memory Configuration:\n");
    fprintf(stderr, "  -r, --ram SIZE         Physical RAM size in MB (default: 64)\n");
    fprintf(stderr, "  -p, --page-size SIZE   Page size in bytes (default: 4096)\n");
    fprintf(stderr, "  -s, --swap SIZE        Swap size in MB (default: 256)\n");
    fprintf(stderr, "  -v, --vspace SIZE      Virtual address space in MB (default: 4096)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Algorithms:\n");
    fprintf(stderr, "  -a, --algorithm ALGO   Replacement algorithm:\n");
    fprintf(stderr, "                         FIFO, LRU, APPROX_LRU, CLOCK, OPT (default: CLOCK)\n");
    fprintf(stderr, "  -T, --tlb-size SIZE    TLB entries (default: 64)\n");
    fprintf(stderr, "  --tlb-policy POLICY    TLB policy: FIFO, LRU (default: LRU)\n");
    fprintf(stderr, "  --pt-type TYPE         Page table type: SINGLE, TWO_LEVEL (default: SINGLE)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Simulation:\n");
    fprintf(stderr, "  -n, --max-accesses N   Stop after N memory accesses (default: all)\n");
    fprintf(stderr, "  --seed SEED            Random seed (default: 42)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Output:\n");
    fprintf(stderr, "  -o, --output FILE      Output file (JSON format)\n");
    fprintf(stderr, "  --csv FILE             CSV output file\n");
    fprintf(stderr, "  --config-name NAME     Configuration name for CSV (default: 'default')\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Verbosity:\n");
    fprintf(stderr, "  -V, --verbose          Verbose output\n");
    fprintf(stderr, "  -D, --debug            Debug output\n");
    fprintf(stderr, "  -q, --quiet            Quiet mode (errors only)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Other:\n");
    fprintf(stderr, "  -h, --help             Show this help message\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s -r 128 -p 4096 -t trace.txt -a LRU -T 32\n", prog_name);
    fprintf(stderr, "  %s -r 64 -a CLOCK -t working_set.trace -o results.json --csv results.csv\n",
            prog_name);
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
    // Default configuration
    VMMConfig config;
    vmm_config_init_default(&config);

    const char *trace_file = NULL;
    const char *output_file = NULL;
    const char *csv_file = NULL;
    const char *config_name = "default";

    // Long options
    static struct option long_options[] = {
        {"trace", required_argument, 0, 't'},
        {"ram", required_argument, 0, 'r'},
        {"page-size", required_argument, 0, 'p'},
        {"swap", required_argument, 0, 's'},
        {"vspace", required_argument, 0, 'v'},
        {"algorithm", required_argument, 0, 'a'},
        {"tlb-size", required_argument, 0, 'T'},
        {"tlb-policy", required_argument, 0, 1000},
        {"pt-type", required_argument, 0, 1001},
        {"max-accesses", required_argument, 0, 'n'},
        {"seed", required_argument, 0, 1002},
        {"output", required_argument, 0, 'o'},
        {"csv", required_argument, 0, 1003},
        {"config-name", required_argument, 0, 1004},
        {"verbose", no_argument, 0, 'V'},
        {"debug", no_argument, 0, 'D'},
        {"quiet", no_argument, 0, 'q'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "t:r:p:s:v:a:T:n:o:VDqh", long_options,
                              &option_index)) != -1) {
        switch (opt) {
        case 't':
            trace_file = optarg;
            break;
        case 'r':
            config.ram_size_mb = atoi(optarg);
            config.num_frames = (config.ram_size_mb * 1024 * 1024) / config.page_size;
            break;
        case 'p':
            config.page_size = atoi(optarg);
            config.num_frames = (config.ram_size_mb * 1024 * 1024) / config.page_size;
            break;
        case 's':
            config.swap_size_mb = atoi(optarg);
            break;
        case 'v':
            config.virtual_addr_space = (uint64_t)atoi(optarg) * 1024 * 1024;
            break;
        case 'a':
            if (strcasecmp(optarg, "FIFO") == 0)
                config.replacement_algo = REPLACE_FIFO;
            else if (strcasecmp(optarg, "LRU") == 0)
                config.replacement_algo = REPLACE_LRU;
            else if (strcasecmp(optarg, "APPROX_LRU") == 0)
                config.replacement_algo = REPLACE_APPROX_LRU;
            else if (strcasecmp(optarg, "CLOCK") == 0)
                config.replacement_algo = REPLACE_CLOCK;
            else if (strcasecmp(optarg, "OPT") == 0)
                config.replacement_algo = REPLACE_OPT;
            else {
                fprintf(stderr, "Unknown replacement algorithm: %s\n", optarg);
                return 1;
            }
            break;
        case 'T':
            config.tlb_size = atoi(optarg);
            break;
        case 1000: // --tlb-policy
            if (strcasecmp(optarg, "FIFO") == 0)
                config.tlb_policy = TLB_FIFO;
            else if (strcasecmp(optarg, "LRU") == 0)
                config.tlb_policy = TLB_LRU;
            else {
                fprintf(stderr, "Unknown TLB policy: %s\n", optarg);
                return 1;
            }
            break;
        case 1001: // --pt-type
            if (strcasecmp(optarg, "SINGLE") == 0)
                config.pt_type = PT_SINGLE_LEVEL;
            else if (strcasecmp(optarg, "TWO_LEVEL") == 0)
                config.pt_type = PT_TWO_LEVEL;
            else {
                fprintf(stderr, "Unknown page table type: %s\n", optarg);
                return 1;
            }
            break;
        case 'n':
            config.max_instructions = strtoull(optarg, NULL, 10);
            break;
        case 1002: // --seed
            config.random_seed = atoi(optarg);
            break;
        case 'o':
            output_file = optarg;
            break;
        case 1003: // --csv
            csv_file = optarg;
            break;
        case 1004: // --config-name
            config_name = optarg;
            break;
        case 'V':
            config.verbose = true;
            set_log_level(LOG_INFO);
            break;
        case 'D':
            config.debug = true;
            set_log_level(LOG_DEBUG);
            break;
        case 'q':
            set_log_level(LOG_ERROR);
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    // Validate required arguments
    if (!trace_file) {
        fprintf(stderr, "Error: Trace file is required\n\n");
        print_usage(argv[0]);
        return 1;
    }

    // Validate configuration
    if (!is_power_of_two(config.page_size)) {
        fprintf(stderr, "Error: Page size must be a power of 2\n");
        return 1;
    }

    if (config.tlb_size == 0) {
        fprintf(stderr, "Error: TLB size must be > 0\n");
        return 1;
    }

    // Print configuration
    printf("==================== VMM SIMULATOR ====================\n");
    vmm_config_print(&config, stdout);
    printf("Trace file:       %s\n", trace_file);
    printf("=======================================================\n\n");

    // Load trace
    Trace *trace = trace_load(trace_file);
    if (!trace) {
        fprintf(stderr, "Error: Failed to load trace file: %s\n", trace_file);
        return 1;
    }

    // Create VMM
    VMM *vmm = vmm_create(&config);
    if (!vmm) {
        fprintf(stderr, "Error: Failed to create VMM\n");
        trace_destroy(trace);
        return 1;
    }

    // Run simulation
    bool success = vmm_run_trace(vmm, trace);
    if (!success) {
        fprintf(stderr, "Error: Simulation failed\n");
        vmm_destroy(vmm);
        trace_destroy(trace);
        return 1;
    }

    // Print results
    metrics_print_summary(vmm->metrics, stdout, &config.access_times);
    if (config.verbose) {
        metrics_print_per_process(vmm->metrics, stdout);
    }

    // Save output files
    if (output_file) {
        metrics_save_json(vmm->metrics, output_file, &config.access_times);
    }

    if (csv_file) {
        metrics_save_csv(vmm->metrics, csv_file, config_name, &config.access_times);
    }

    // Cleanup
    vmm_destroy(vmm);
    trace_destroy(trace);

    printf("\nSimulation completed successfully.\n");
    return 0;
}

