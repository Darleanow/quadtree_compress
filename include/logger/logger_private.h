/**
 * @file logger_private.h
 * @brief Internal implementations for the logger
 *
 * Contains internal structures and constants for:
 * - Color schemes
 * - Terminal symbols
 * - Layout configurations
 * - Global state management
 */

#ifndef LOGGER_PRIVATE_H
#define LOGGER_PRIVATE_H

#include <stdio.h>
#include "logger.h"

/* Color management structure */
typedef struct
{
    const char *regular;
    const char *bold;
    const char *dim;
} ColorScheme;

/* Global state tracking */
typedef struct
{
    logger_config_t config;
    FILE *output_stream;
    bool is_progress_active;
    bool is_initialized;
} LoggerState;

/* Terminal symbols configuration */
struct TerminalSymbols
{
    const char *bar_full;
    const char *bar_empty;
    const char *corner_tl;
    const char *corner_tr;
    const char *corner_bl;
    const char *corner_br;
    const char *line_h;
    const char *line_v;
    const char *bullet;
};

/* Color configuration */
struct ColorConfig
{
    const char *reset;
    const char *bold;
    const char *dim;
    const char *italic;
    ColorScheme info;
    ColorScheme success;
    ColorScheme warning;
    ColorScheme error;
};

/* Theme settings */
struct ThemeConfig
{
    const char *header;
    const char *border;
    const char *label;
    const char *value;
    const char *timestamp;
};

/* Layout parameters */
struct LayoutConfig
{
    uint16_t separator_width;
    uint16_t progress_width;
    uint16_t label_width;
};

extern LoggerState g_state;
extern const struct TerminalSymbols *const SYMBOLS;
extern const struct ColorConfig *const COLORS;
extern const struct ThemeConfig *const THEME;
extern const struct LayoutConfig *const LAYOUT;

/* Internal utility functions */
void ensure_initialized(void);
void safe_vfprintf(FILE *out, const char *format, va_list args);
ColorScheme get_level_colors(log_level_t level);
const char *get_level_symbol(log_level_t level);

#endif /* LOGGER_PRIVATE_H */