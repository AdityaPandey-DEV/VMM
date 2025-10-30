/**
 * util.h - Utility functions for VMM simulator
 * 
 * Provides logging, error handling, configuration parsing, and helper functions.
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Logging levels
typedef enum {
    LOG_ERROR = 0,
    LOG_WARN = 1,
    LOG_INFO = 2,
    LOG_DEBUG = 3,
    LOG_TRACE = 4
} LogLevel;

// Set global log level
void set_log_level(LogLevel level);

// Logging macros
void log_message(LogLevel level, const char *file, int line, const char *fmt, ...);

#define LOG_ERROR_MSG(...) log_message(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN_MSG(...) log_message(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO_MSG(...) log_message(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG_MSG(...) log_message(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_TRACE_MSG(...) log_message(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)

// Utility functions
uint64_t get_timestamp_us(void);  // Microsecond timestamp
void print_binary(uint64_t value, int bits);
uint32_t next_power_of_two(uint32_t v);
bool is_power_of_two(uint32_t v);

// Bit manipulation helpers
static inline uint32_t extract_bits(uint64_t value, int start, int length)
{
    return (value >> start) & ((1ULL << length) - 1);
}

static inline uint64_t align_down(uint64_t addr, uint64_t alignment)
{
    return addr & ~(alignment - 1);
}

static inline uint64_t align_up(uint64_t addr, uint64_t alignment)
{
    return (addr + alignment - 1) & ~(alignment - 1);
}

#endif // UTIL_H

