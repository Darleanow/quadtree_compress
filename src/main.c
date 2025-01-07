#include <stdlib.h>
#include <time.h>

#include "cli/cli.h"
#include "config/config.h"
#include "codec/codec.h"
#include "logger/logger.h"
#include "grid/segmentation_grid.h"

#define PROGRAM_NAME "Quadtree Image Codec"
#define PROGRAM_VERSION "1.0.0"

/**
 * @brief Program entry point
 *
 * @param[in] argc Argument count
 * @param[in] argv Argument vector
 * @return Exit status (EXIT_SUCCESS/EXIT_FAILURE)
 */
int main(int argc, char **argv)
{
    config_t config;
    int status = EXIT_SUCCESS;
    clock_t start_time;

    logger_configure((logger_config_t){
        .enabled = true,
        .use_colors = true,
        .show_timestamp = true});

    log_header(PROGRAM_NAME);
    log_item("Version", "%s", PROGRAM_VERSION);
    log_separator();

    log_info("Parsing command line arguments...");
    if (!cli_parse_arguments(argc, argv, &config))
    {
        log_error("Failed to parse command line arguments");
        return EXIT_FAILURE;
    }

    /* Start operation timing */
    start_time = clock();

    /* Log operation mode */
    if (config.compress)
    {
        log_info("Starting compression: %s -> %s",
                 config.input_file,
                 config.output_file);
    }
    else
    {
        log_info("Starting decompression: %s -> %s",
                 config.input_file,
                 config.output_file);
    }

    /* Execute requested operation */
    if (config.compress)
    {
        codec_status_t result = codec_compress(&config);
        if (result != CODEC_SUCCESS)
        {
            log_error("Compression failed: %s", codec_status_string(result));
            status = EXIT_FAILURE;
        }
    }
    else
    {
        codec_status_t result = codec_decompress(&config);
        if (result != CODEC_SUCCESS)
        {
            log_error("Decompression failed: %s", codec_status_string(result));
            status = EXIT_FAILURE;
        }
    }

    /* Log operation timing */
    double elapsed = (double)(clock() - start_time) / CLOCKS_PER_SEC;

    log_separator();
    if (status == EXIT_SUCCESS)
    {
        log_success("Operation completed in %.3f seconds", elapsed);
    }
    else
    {
        log_error("Operation failed after %.3f seconds", elapsed);
    }

    return status;
}