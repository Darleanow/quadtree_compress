/**
 * @file logger.h
 * @brief Simple logging library for terminal output
 *
 * Provides functionality for:
 * - Colored terminal output
 * - Progress tracking
 * - Message levels
 * - Formatted text output
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include <stdint.h>

/* Message types enumeration */
typedef enum
{
    LOG_LEVEL_INFO,
    LOG_LEVEL_SUCCESS,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level_t;

/* Logger configuration settings */
typedef struct
{
    bool enabled;
    bool use_colors;
    bool show_timestamp;
} logger_config_t;

/* Core functions */
void logger_configure(logger_config_t config);
void log_message(log_level_t level, const char *format, ...);
void log_progress(double percentage);
void log_end_progress(void);

/* Formatting functions */
void log_separator(void);
void log_header(const char *title);
void log_subheader(const char *title);
void log_item(const char *label, const char *format, ...);
void log_newline(void);

/* Specialized logging functions */
void log_file_info(const char *filename, uint32_t size, uint32_t levels, double ratio);
void log_size_stats(size_t original_size, size_t processed_size, size_t nodes, double time);

/* Convenience macros */
#define log_info(...) log_message(LOG_LEVEL_INFO, __VA_ARGS__)
#define log_success(...) log_message(LOG_LEVEL_SUCCESS, __VA_ARGS__)
#define log_warn(...) log_message(LOG_LEVEL_WARN, __VA_ARGS__)
#define log_error(...) log_message(LOG_LEVEL_ERROR, __VA_ARGS__)

#endif /* LOGGER_H */