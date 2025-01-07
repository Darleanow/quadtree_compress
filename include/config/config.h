/**
 * @file config.h
 * @brief Settings for our program
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

/* Default output names if none given */
#define DEFAULT_COMPRESS_OUTPUT "default_compress_output.qtc"
#define DEFAULT_DECOMPRESS_OUTPUT "default_compress_input.pgm"
#define DEFAULT_ALPHA 1.0f

/**
 * @brief Structure to hold all our program settings
 */
typedef struct
{
    bool compress;           /* Are we compressing? */
    bool decompress;         /* Are we decompressing? */
    bool verbose;            /* Should we show extra info? */
    bool generate_grid;      /* Should we make a grid view? */
    const char *input_file;  /* Which file to read */
    const char *output_file; /* Where to save the result */
    const char *grid_file;   /* Where to save the grid */
    float alpha;             /* How much to compress */
} config_t;

/**
 * @brief Sets up config with normal values
 * @param config Where to store the settings
 */
void config_init(config_t *config);

#endif /* CONFIG_H */