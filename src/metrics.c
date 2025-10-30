/**
 * metrics.c - Performance metrics implementation
 */

#include "metrics.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

Metrics *metrics_create(uint32_t max_processes)
{
    Metrics *m = calloc(1, sizeof(Metrics));
    if (!m) {
        LOG_ERROR_MSG("Failed to allocate metrics");
        return NULL;
    }

    m->max_processes = max_processes;
    m->process_metrics = calloc(max_processes, sizeof(ProcessMetrics));
    if (!m->process_metrics) {
        LOG_ERROR_MSG("Failed to allocate process metrics");
        free(m);
        return NULL;
    }

    return m;
}

void metrics_destroy(Metrics *metrics)
{
    if (!metrics)
        return;
    free(metrics->process_metrics);
    free(metrics);
}

static ProcessMetrics *get_process_metrics(Metrics *m, uint32_t pid)
{
    // Find existing or create new
    for (uint32_t i = 0; i < m->num_processes; i++) {
        if (m->process_metrics[i].pid == pid) {
            return &m->process_metrics[i];
        }
    }

    // Add new process
    if (m->num_processes < m->max_processes) {
        m->process_metrics[m->num_processes].pid = pid;
        return &m->process_metrics[m->num_processes++];
    }

    return NULL;
}

void metrics_record_access(Metrics *m, uint32_t pid, bool is_write)
{
    if (!m)
        return;

    m->total_accesses++;
    if (is_write)
        m->total_writes++;
    else
        m->total_reads++;

    ProcessMetrics *pm = get_process_metrics(m, pid);
    if (pm) {
        pm->total_accesses++;
        if (is_write)
            pm->writes++;
        else
            pm->reads++;
    }
}

void metrics_record_tlb_hit(Metrics *m, uint32_t pid)
{
    if (!m)
        return;

    m->tlb_hits++;

    ProcessMetrics *pm = get_process_metrics(m, pid);
    if (pm) {
        pm->tlb_hits++;
    }
}

void metrics_record_tlb_miss(Metrics *m, uint32_t pid)
{
    if (!m)
        return;

    m->tlb_misses++;

    ProcessMetrics *pm = get_process_metrics(m, pid);
    if (pm) {
        pm->tlb_misses++;
    }
}

void metrics_record_page_fault(Metrics *m, uint32_t pid, bool is_major)
{
    if (!m)
        return;

    m->page_faults++;
    if (is_major)
        m->major_faults++;
    else
        m->minor_faults++;

    ProcessMetrics *pm = get_process_metrics(m, pid);
    if (pm) {
        pm->page_faults++;
    }
}

void metrics_record_swap_in(Metrics *m)
{
    if (m)
        m->swap_ins++;
}

void metrics_record_swap_out(Metrics *m)
{
    if (m)
        m->swap_outs++;
}

void metrics_record_replacement(Metrics *m)
{
    if (m)
        m->replacements++;
}

void metrics_start_simulation(Metrics *m)
{
    if (m)
        m->simulation_start_time_us = get_timestamp_us();
}

void metrics_end_simulation(Metrics *m)
{
    if (m)
        m->simulation_end_time_us = get_timestamp_us();
}

double metrics_get_page_fault_rate(Metrics *m)
{
    if (!m || m->total_accesses == 0)
        return 0.0;
    return (double)m->page_faults / m->total_accesses;
}

double metrics_get_tlb_hit_rate(Metrics *m)
{
    if (!m)
        return 0.0;
    uint64_t total_tlb_accesses = m->tlb_hits + m->tlb_misses;
    if (total_tlb_accesses == 0)
        return 0.0;
    return (double)m->tlb_hits / total_tlb_accesses;
}

double metrics_get_avg_memory_access_time(Metrics *m, AccessTimeConfig *config)
{
    if (!m || !config || m->total_accesses == 0)
        return 0.0;

    // AMT = TLB_hit_time + (TLB_miss_rate × page_table_time) + (page_fault_rate × page_fault_time)
    double tlb_hit_rate = metrics_get_tlb_hit_rate(m);
    double tlb_miss_rate = 1.0 - tlb_hit_rate;
    double page_fault_rate = metrics_get_page_fault_rate(m);

    double amt_ns = config->tlb_hit_time_ns +
                    (tlb_miss_rate * config->memory_access_time_ns) +
                    (page_fault_rate * config->page_fault_time_us * 1000);

    return amt_ns;
}

void metrics_print_summary(Metrics *m, FILE *out, AccessTimeConfig *config)
{
    if (!m || !out)
        return;

    fprintf(out, "\n");
    fprintf(out, "==================== SIMULATION SUMMARY ====================\n");
    fprintf(out, "\n");

    // Memory accesses
    fprintf(out, "Memory Accesses:\n");
    fprintf(out, "  Total:        %12lu\n", m->total_accesses);
    fprintf(out, "  Reads:        %12lu (%.1f%%)\n", m->total_reads,
            m->total_accesses ? 100.0 * m->total_reads / m->total_accesses : 0.0);
    fprintf(out, "  Writes:       %12lu (%.1f%%)\n", m->total_writes,
            m->total_accesses ? 100.0 * m->total_writes / m->total_accesses : 0.0);
    fprintf(out, "\n");

    // Page faults
    fprintf(out, "Page Faults:\n");
    fprintf(out, "  Total:        %12lu\n", m->page_faults);
    fprintf(out, "  Major:        %12lu (required swap-in)\n", m->major_faults);
    fprintf(out, "  Minor:        %12lu (no I/O)\n", m->minor_faults);
    fprintf(out, "  Fault Rate:   %12.4f%%\n", 100.0 * metrics_get_page_fault_rate(m));
    fprintf(out, "\n");

    // TLB
    fprintf(out, "TLB Performance:\n");
    fprintf(out, "  Hits:         %12lu\n", m->tlb_hits);
    fprintf(out, "  Misses:       %12lu\n", m->tlb_misses);
    fprintf(out, "  Hit Rate:     %12.2f%%\n", 100.0 * metrics_get_tlb_hit_rate(m));
    fprintf(out, "\n");

    // Swap I/O
    fprintf(out, "Swap I/O:\n");
    fprintf(out, "  Swap-ins:     %12lu\n", m->swap_ins);
    fprintf(out, "  Swap-outs:    %12lu\n", m->swap_outs);
    fprintf(out, "  Replacements: %12lu\n", m->replacements);
    fprintf(out, "\n");

    // Timing
    if (config) {
        double amt = metrics_get_avg_memory_access_time(m, config);
        fprintf(out, "Average Memory Access Time:\n");
        fprintf(out, "  AMT:          %12.2f ns\n", amt);
        fprintf(out, "  Slowdown:     %12.2fx (vs TLB hit)\n",
                amt / config->tlb_hit_time_ns);
        fprintf(out, "\n");
    }

    // Simulation time
    uint64_t sim_time_us = m->simulation_end_time_us - m->simulation_start_time_us;
    fprintf(out, "Simulation Time:\n");
    fprintf(out, "  Wall time:    %12.3f ms\n", sim_time_us / 1000.0);
    fprintf(out, "  Throughput:   %12.1f accesses/ms\n",
            sim_time_us > 0 ? (double)m->total_accesses / (sim_time_us / 1000.0) : 0.0);
    fprintf(out, "\n");

    fprintf(out, "============================================================\n");
}

void metrics_print_per_process(Metrics *m, FILE *out)
{
    if (!m || !out)
        return;

    fprintf(out, "\n");
    fprintf(out, "==================== PER-PROCESS METRICS ====================\n");
    fprintf(out, "\n");
    fprintf(out, "  PID | Accesses  | Reads     | Writes    | Faults    | TLB Hits  | TLB Misses\n");
    fprintf(out, "------+-----------+-----------+-----------+-----------+-----------+-----------\n");

    for (uint32_t i = 0; i < m->num_processes; i++) {
        ProcessMetrics *pm = &m->process_metrics[i];
        fprintf(out, " %4u | %9lu | %9lu | %9lu | %9lu | %9lu | %9lu\n", pm->pid,
                pm->total_accesses, pm->reads, pm->writes, pm->page_faults, pm->tlb_hits,
                pm->tlb_misses);
    }

    fprintf(out, "=================================================================\n");
}

bool metrics_save_csv(Metrics *m, const char *filename, const char *config_name,
                      AccessTimeConfig *time_config)
{
    if (!m || !filename)
        return false;

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        LOG_ERROR_MSG("Failed to create CSV file: %s", filename);
        return false;
    }

    // Header
    fprintf(fp, "config,total_accesses,reads,writes,page_faults,pf_rate,tlb_hits,tlb_misses,"
                "tlb_hit_rate,swap_ins,swap_outs,replacements,amt_ns,runtime_ms\n");

    // Data
    double amt = time_config ? metrics_get_avg_memory_access_time(m, time_config) : 0.0;
    uint64_t runtime_us = m->simulation_end_time_us - m->simulation_start_time_us;

    fprintf(fp, "%s,%lu,%lu,%lu,%lu,%.6f,%lu,%lu,%.4f,%lu,%lu,%lu,%.2f,%.3f\n",
            config_name ? config_name : "default", m->total_accesses, m->total_reads,
            m->total_writes, m->page_faults, metrics_get_page_fault_rate(m), m->tlb_hits,
            m->tlb_misses, metrics_get_tlb_hit_rate(m), m->swap_ins, m->swap_outs,
            m->replacements, amt, runtime_us / 1000.0);

    fclose(fp);
    LOG_INFO_MSG("Saved CSV metrics to %s", filename);
    return true;
}

bool metrics_save_json(Metrics *m, const char *filename, AccessTimeConfig *time_config)
{
    if (!m || !filename)
        return false;

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        LOG_ERROR_MSG("Failed to create JSON file: %s", filename);
        return false;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"total_accesses\": %lu,\n", m->total_accesses);
    fprintf(fp, "  \"reads\": %lu,\n", m->total_reads);
    fprintf(fp, "  \"writes\": %lu,\n", m->total_writes);
    fprintf(fp, "  \"page_faults\": %lu,\n", m->page_faults);
    fprintf(fp, "  \"major_faults\": %lu,\n", m->major_faults);
    fprintf(fp, "  \"minor_faults\": %lu,\n", m->minor_faults);
    fprintf(fp, "  \"page_fault_rate\": %.6f,\n", metrics_get_page_fault_rate(m));
    fprintf(fp, "  \"tlb_hits\": %lu,\n", m->tlb_hits);
    fprintf(fp, "  \"tlb_misses\": %lu,\n", m->tlb_misses);
    fprintf(fp, "  \"tlb_hit_rate\": %.4f,\n", metrics_get_tlb_hit_rate(m));
    fprintf(fp, "  \"swap_ins\": %lu,\n", m->swap_ins);
    fprintf(fp, "  \"swap_outs\": %lu,\n", m->swap_outs);
    fprintf(fp, "  \"replacements\": %lu,\n", m->replacements);

    if (time_config) {
        fprintf(fp, "  \"avg_memory_access_time_ns\": %.2f,\n",
                metrics_get_avg_memory_access_time(m, time_config));
    }

    uint64_t runtime_us = m->simulation_end_time_us - m->simulation_start_time_us;
    fprintf(fp, "  \"simulation_time_ms\": %.3f,\n", runtime_us / 1000.0);

    fprintf(fp, "  \"per_process\": [\n");
    for (uint32_t i = 0; i < m->num_processes; i++) {
        ProcessMetrics *pm = &m->process_metrics[i];
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"pid\": %u,\n", pm->pid);
        fprintf(fp, "      \"accesses\": %lu,\n", pm->total_accesses);
        fprintf(fp, "      \"reads\": %lu,\n", pm->reads);
        fprintf(fp, "      \"writes\": %lu,\n", pm->writes);
        fprintf(fp, "      \"page_faults\": %lu,\n", pm->page_faults);
        fprintf(fp, "      \"tlb_hits\": %lu,\n", pm->tlb_hits);
        fprintf(fp, "      \"tlb_misses\": %lu\n", pm->tlb_misses);
        fprintf(fp, "    }%s\n", i < m->num_processes - 1 ? "," : "");
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");

    fclose(fp);
    LOG_INFO_MSG("Saved JSON metrics to %s", filename);
    return true;
}

