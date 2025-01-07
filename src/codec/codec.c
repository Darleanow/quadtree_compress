#include <stdio.h>
#include <stdlib.h>

#include "codec/codec.h"
#include "codec/compression.h"
#include "codec/decompression.h"
#include "core/quadtree.h"
#include "io/pgm.h"
#include "logger/logger.h"
#include "grid/segmentation_grid.h"

/* Internal status conversion utilities */
static const char *status_strings[] = {
    [CODEC_SUCCESS] = "Success",
    [CODEC_ERROR_INVALID_PARAM] = "Invalid parameters",
    [CODEC_ERROR_FILE_IO] = "File I/O error",
    [CODEC_ERROR_MEMORY] = "Memory allocation error",
    [CODEC_ERROR_FORMAT] = "Invalid file format"};

const char *codec_status_string(codec_status_t status)
{
    if (status < 0 || status >= sizeof(status_strings) / sizeof(status_strings[0]))
    {
        return "Unknown error";
    }
    return status_strings[status];
}

/* Convert internal error codes to codec status */
static codec_status_t convert_qtree_status(qtree_status_t status)
{
    switch (status)
    {
    case QTREE_SUCCESS:
        return CODEC_SUCCESS;
    case QTREE_ERROR_INVALID_PARAM:
        return CODEC_ERROR_INVALID_PARAM;
    case QTREE_ERROR_MEMORY:
        return CODEC_ERROR_MEMORY;
    default:
        return CODEC_ERROR_FORMAT;
    }
}

static codec_status_t convert_pgm_status(pgm_status_t status)
{
    switch (status)
    {
    case PGM_SUCCESS:
        return CODEC_SUCCESS;
    case PGM_ERROR_PARAM:
        return CODEC_ERROR_INVALID_PARAM;
    case PGM_ERROR_FILE:
        return CODEC_ERROR_FILE_IO;
    default:
        return CODEC_ERROR_FORMAT;
    }
}

codec_status_t codec_compress(const config_t *config)
{
    if (!config || !config->input_file || !config->output_file)
    {
        log_error("Invalid compression parameters");
        return CODEC_ERROR_INVALID_PARAM;
    }

    pgm_t pgm;
    qtree_t tree;
    FILE *output = NULL;
    codec_status_t status = CODEC_SUCCESS;

    log_subheader("Compression Operation");
    log_item("Input", "%s", config->input_file);
    log_item("Output", "%s", config->output_file);

    /* Read input PGM */
    log_info("Reading input PGM file...");
    pgm_status_t pgm_result = pgm_read(config->input_file, &pgm);
    if (pgm_result != PGM_SUCCESS)
    {
        log_error("Failed to read PGM file");
        return convert_pgm_status(pgm_result);
    }

    /* Create and initialize quadtree */
    log_info("Initializing quadtree structure...");

    if (qtree_init(&tree, pgm.size) != QTREE_SUCCESS)
    {
        log_error("Failed to initialize quadtree");
        pgm_free(&pgm);
        return CODEC_ERROR_MEMORY;
    }

    /* Open output file */
    log_info("Opening output file...");
    if (!(output = fopen(config->output_file, "wb")))
    {
        log_error("Failed to open output file: %s", config->output_file);
        pgm_free(&pgm);
        free_quadtree_recursive(tree.root);
        return CODEC_ERROR_FILE_IO;
    }

    /* Build quadtree from image data */
    log_info("Building quadtree from image data...");
    qtree_status_t build_status = qtree_build(&tree, pgm.pixels, pgm.size, config->input_file);
    if (build_status != QTREE_SUCCESS)
    {
        log_error("Failed to build quadtree");
        status = convert_qtree_status(build_status);
        goto cleanup;
    }

    /* Apply lossy compression if requested */
    if (config->alpha > 1.0f)
    {
        log_info("Applying lossy compression (alpha = %.2f)...", (double)config->alpha);
        qtree_status_t lossy_status = apply_lossy_compression(&tree, config->alpha);
        if (lossy_status != QTREE_SUCCESS)
        {
            log_error("Failed to apply lossy compression");
            status = convert_qtree_status(lossy_status);
            goto cleanup;
        }
    }

    /* Perform compression */
    log_info("Compressing data...");
    qtree_status_t compress_status = compress(&tree, config->output_file, output);
    if (compress_status != QTREE_SUCCESS)
    {
        log_error("Failed to compress data");
        status = convert_qtree_status(compress_status);
        goto cleanup;
    }

    log_success("Compression completed successfully");

    if (config->generate_grid)
    {
        qtree_generate_grid(&tree, config->grid_file);
    }

cleanup:
    pgm_free(&pgm);
    free_quadtree_recursive(tree.root);
    if (output)
        fclose(output);
    return status;
}

codec_status_t codec_decompress(const config_t *config)
{
    if (!config || !config->input_file || !config->output_file)
    {
        log_error("Invalid decompression parameters");
        return CODEC_ERROR_INVALID_PARAM;
    }

    qtree_t tree;
    pgm_t pgm;
    FILE *input = NULL;
    codec_status_t status = CODEC_SUCCESS;

    log_subheader("Decompression Operation");
    log_item("Input", "%s", config->input_file);
    log_item("Output", "%s", config->output_file);

    /* Open input file */
    log_info("Opening input file...");
    if (!(input = fopen(config->input_file, "rb")))
    {
        log_error("Failed to open input file: %s", config->input_file);
        return CODEC_ERROR_FILE_IO;
    }

    /* Read compressed data */
    log_info("Reading compressed data...");
    qtree_status_t read_status = qtree_decompress(input, config->input_file, &tree);
    if (read_status != QTREE_SUCCESS)
    {
        log_error("Failed to read compressed data");
        status = convert_qtree_status(read_status);
        goto cleanup;
    }

    /* Convert to PGM format */
    log_info("Converting to PGM format...");
    qtree_status_t convert_status = qtree_to_pgm(&tree, config->output_file, &pgm);
    if (convert_status != QTREE_SUCCESS)
    {
        log_error("Failed to convert to PGM format");
        status = convert_qtree_status(convert_status);
        goto cleanup_tree;
    }

    /* Write output file */
    log_info("Writing output file...");
    pgm_status_t write_status = pgm_write(&pgm, config->output_file);
    if (write_status != PGM_SUCCESS)
    {
        log_error("Failed to write PGM file");
        status = convert_pgm_status(write_status);
        pgm_free(&pgm);
    }
    else
    {
        log_success("Decompression completed successfully");
    }

    if (config->generate_grid)
    {
        qtree_generate_grid(&tree, config->grid_file);
    }

cleanup_tree:
    free_quadtree_recursive(tree.root);
cleanup:
    if (input)
        fclose(input);
    return status;
}