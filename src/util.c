/**
 * util.c - Utility function implementations
 */

#include "util.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

static LogLevel current_log_level = LOG_INFO;

void set_log_level(LogLevel level)
{
    current_log_level = level;
}

void log_message(LogLevel level, const char *file, int line, const char *fmt, ...)
{
    if (level > current_log_level)
        return;

    const char *level_str[] = {"ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
    const char *level_color[] = {"\033[1;31m", "\033[1;33m", "\033[1;32m", "\033[1;34m",
                                  "\033[1;35m"};

    fprintf(stderr, "%s[%s]\033[0m ", level_color[level], level_str[level]);

    if (level <= LOG_WARN) {
        // Include file and line for errors and warnings
        const char *basename = strrchr(file, '/');
        fprintf(stderr, "(%s:%d) ", basename ? basename + 1 : file, line);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

uint64_t get_timestamp_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

void print_binary(uint64_t value, int bits)
{
    for (int i = bits - 1; i >= 0; i--) {
        printf("%d", (int)((value >> i) & 1));
        if (i > 0 && i % 4 == 0)
            printf(" ");
    }
}

uint32_t next_power_of_two(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

bool is_power_of_two(uint32_t v)
{
    return v && !(v & (v - 1));
}

