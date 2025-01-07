/**
 * @file logger_utils.c
 * @brief Implementation of the methods
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "logger/logger.h"
#include "logger/logger_private.h"

LoggerState g_state = {
    .config = {
        .enabled = true,
        .use_colors = true,
        .show_timestamp = true
    },
    .output_stream = NULL,
    .is_progress_active = false,
    .is_initialized = false
};

static const struct TerminalSymbols s_symbols = {
    .bar_full = "█",
    .bar_empty = "░",
    .corner_tl = "╭",
    .corner_tr = "╮",
    .corner_bl = "╰",
    .corner_br = "╯",
    .line_h = "─",
    .line_v = "│",
    .bullet = "◆ "
};

static const struct ColorConfig s_colors = {
    .reset = "\x1b[0m",
    .bold = "\x1b[1m",
    .dim = "\x1b[2m",
    .italic = "\x1b[3m",
    .info = {
        .regular = "\x1b[38;5;39m",
        .bold = "\x1b[1;38;5;39m",
        .dim = "\x1b[2;38;5;39m"
    },
    .success = {
        .regular = "\x1b[38;5;82m",
        .bold = "\x1b[1;38;5;82m",
        .dim = "\x1b[2;38;5;82m"
    },
    .warning = {
        .regular = "\x1b[38;5;220m",
        .bold = "\x1b[1;38;5;220m",
        .dim = "\x1b[2;38;5;220m"
    },
    .error = {
        .regular = "\x1b[38;5;196m",
        .bold = "\x1b[1;38;5;196m",
        .dim = "\x1b[2;38;5;196m"
    }
};

static const struct ThemeConfig s_theme = {
    .header = "\x1b[38;5;105m",
    .border = "\x1b[38;5;60m",
    .label = "\x1b[38;5;248m",
    .value = "\x1b[38;5;252m",
    .timestamp = "\x1b[38;5;240m"
};

static const struct LayoutConfig s_layout = {
    .separator_width = 80,
    .progress_width = 50,
    .label_width = 20
};

/* Public constant references */
const struct TerminalSymbols *const SYMBOLS = &s_symbols;
const struct ColorConfig *const COLORS = &s_colors;
const struct ThemeConfig *const THEME = &s_theme;
const struct LayoutConfig *const LAYOUT = &s_layout;

/* Internal utilities */
void safe_vfprintf(FILE *out, const char *format, va_list args)
{
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
    vfprintf(out, format, args);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

void ensure_initialized(void)
{
    if (g_state.is_initialized)
        return;
    g_state.output_stream = stdout;
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    g_state.is_initialized = true;
}

ColorScheme get_level_colors(log_level_t level)
{
    if (!g_state.config.use_colors)
    {
        return (ColorScheme){.regular = "", .bold = "", .dim = ""};
    }
    switch (level)
    {
    case LOG_LEVEL_INFO:
        return COLORS->info;
    case LOG_LEVEL_SUCCESS:
        return COLORS->success;
    case LOG_LEVEL_WARN:
        return COLORS->warning;
    case LOG_LEVEL_ERROR:
        return COLORS->error;
    default:
        return COLORS->info;
    }
}

const char *get_level_symbol(log_level_t level)
{
    switch (level)
    {
    case LOG_LEVEL_INFO:
        return "ℹ";
    case LOG_LEVEL_SUCCESS:
        return "✓";
    case LOG_LEVEL_WARN:
        return "⚠";
    case LOG_LEVEL_ERROR:
        return "✗";
    default:
        return "•";
    }
}

static void print_timestamp(FILE *out)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(out, "%s[%02d:%02d:%02d]%s ",
            THEME->timestamp,
            t->tm_hour, t->tm_min, t->tm_sec,
            COLORS->reset);
}

/* Public API implementation */
void logger_configure(logger_config_t config)
{
    ensure_initialized();
    g_state.config = config;
}

void log_message(log_level_t level, const char *format, ...)
{
    ensure_initialized();
    if (!g_state.config.enabled)
        return;

    FILE *out = g_state.output_stream;
    ColorScheme colors = get_level_colors(level);

    if (g_state.config.show_timestamp)
    {
        print_timestamp(out);
    }

    fprintf(out, "%s%s%s ",
            colors.bold,
            get_level_symbol(level),
            COLORS->reset);

    va_list args;
    va_start(args, format);
    fprintf(out, "%s", colors.regular);
    safe_vfprintf(out, format, args);
    va_end(args);

    fprintf(out, "%s\n", COLORS->reset);
}

void log_progress(double percentage)
{
    ensure_initialized();
    if (!g_state.config.enabled)
        return;

    FILE *out = g_state.output_stream;
    ColorScheme colors = get_level_colors(LOG_LEVEL_INFO);
    const int filled = (int)(LAYOUT->progress_width * percentage);
    const int empty = LAYOUT->progress_width - filled;

    fprintf(out, "\r%s%s%s",
            colors.regular,
            SYMBOLS->line_v,
            g_state.config.use_colors ? COLORS->reset : "");

    for (int i = 0; i < filled; i++)
    {
        fprintf(out, "%s%s%s",
                colors.bold,
                SYMBOLS->bar_full,
                g_state.config.use_colors ? COLORS->reset : "");
    }

    for (int i = 0; i < empty; i++)
    {
        fprintf(out, "%s%s%s",
                colors.dim,
                SYMBOLS->bar_empty,
                g_state.config.use_colors ? COLORS->reset : "");
    }

    fprintf(out, "%s%s %s%.1f%%%s",
            colors.regular,
            SYMBOLS->line_v,
            colors.bold,
            percentage * 100.0,
            g_state.config.use_colors ? COLORS->reset : "");

    fflush(out);
    g_state.is_progress_active = true;
}

void log_end_progress(void)
{
    ensure_initialized();
    if (!g_state.config.enabled || !g_state.is_progress_active)
        return;
    fprintf(g_state.output_stream, "\n");
    g_state.is_progress_active = false;
}

void log_separator(void)
{
    ensure_initialized();
    if (!g_state.config.enabled)
        return;

    fprintf(g_state.output_stream, "%s", THEME->border);
    for (uint16_t i = 0; i < LAYOUT->separator_width; i++)
    {
        fprintf(g_state.output_stream, "%s", SYMBOLS->line_h);
    }
    fprintf(g_state.output_stream, "%s\n", COLORS->reset);
}

void log_header(const char *title)
{
    ensure_initialized();
    if (!g_state.config.enabled)
        return;

    FILE *out = g_state.output_stream;
    const size_t title_len = strlen(title);
    const size_t padding = (LAYOUT->separator_width - title_len - 4) / 2;

    fprintf(out, "%s%s", THEME->border, SYMBOLS->corner_tl);
    for (size_t i = 0; i < padding; i++)
    {
        fprintf(out, "%s", SYMBOLS->line_h);
    }

    fprintf(out, " %s%s%s ", THEME->header, title, THEME->border);

    for (size_t i = 0; i < padding; i++)
    {
        fprintf(out, "%s", SYMBOLS->line_h);
    }

    fprintf(out, "%s%s\n", SYMBOLS->corner_tr, COLORS->reset);
}

void log_subheader(const char *title)
{
    ensure_initialized();
    if (!g_state.config.enabled)
        return;

    ColorScheme colors = get_level_colors(LOG_LEVEL_INFO);
    fprintf(g_state.output_stream, "\n%s%s%s\n",
            colors.bold,
            title,
            g_state.config.use_colors ? COLORS->reset : "");
}

void log_item(const char *label, const char *format, ...)
{
    ensure_initialized();
    if (!g_state.config.enabled)
        return;

    FILE *out = g_state.output_stream;
    fprintf(out, "%s%s%s%-*s: %s",
            THEME->border, SYMBOLS->bullet, THEME->label,
            LAYOUT->label_width - 2,
            label,
            THEME->value);

    va_list args;
    va_start(args, format);
    safe_vfprintf(out, format, args);
    va_end(args);

    fprintf(out, "%s\n", COLORS->reset);
}

void log_newline(void)
{
    ensure_initialized();
    if (!g_state.config.enabled)
        return;
    fprintf(g_state.output_stream, "\n");
}

void log_file_info(const char *filename, uint32_t size, uint32_t levels, const double ratio)
{
    ensure_initialized();
    if (!g_state.config.enabled)
        return;

    log_subheader("File Information");
    log_item("Name", "%s", filename);
    log_item("Dimensions", "%u×%u pixels", size, size);
    log_item("Tree depth", "%u levels", levels);

    if (ratio > 0.0)
    {
        const char *efficiency =
            ratio < 50.0 ? "Excellent" : ratio < 70.0 ? "Good"
                                     : ratio < 85.0   ? "Fair"
                                                      : "Poor";

        log_item("Compression", "%.2f%% (%s%s%s)",
                 ratio,
                 g_state.config.use_colors ? COLORS->italic : "",
                 efficiency,
                 g_state.config.use_colors ? COLORS->reset : "");
    }
}

void log_size_stats(size_t original_size, size_t processed_size, size_t nodes, double time)
{
    ensure_initialized();
    if (!g_state.config.enabled)
        return;

    const double compression_ratio = 100.0 * (double)processed_size / (double)original_size;
    const double throughput = (double)nodes / time;
    const double orig_kb = (double)original_size / 1024.0;
    const double proc_kb = (double)processed_size / 1024.0;

    log_subheader("Processing Statistics");
    log_item("Original size", "%.2f KB", orig_kb);
    log_item("Processed size", "%.2f KB (%.2f%% of original)", proc_kb, compression_ratio);
    log_item("Nodes processed", "%zu (%.1f nodes/sec)", nodes, throughput);
    log_item("Processing time", "%.3f seconds", time);
}