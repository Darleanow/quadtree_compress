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
    case QTREE_ERROR_FORMAT:
        return CODEC_ERROR_FORMAT;
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
    case PGM_ERROR_FORMAT:
        return CODEC_ERROR_FORMAT;
    case PGM_ERROR_MEMORY:
        return CODEC_ERROR_MEMORY;
    case PGM_ERROR_SIZE:
        return CODEC_ERROR_FORMAT;
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

    pgm_t pgm = {0};
    qtree_t tree = {0};
    FILE *output = NULL;
    codec_status_t status = CODEC_SUCCESS;
    bool pgm_initialized = false;
    bool tree_initialized = false;
    qtree_status_t op_status;

    log_subheader("Compression Operation");
    log_item("Input", "%s", config->input_file);
    log_item("Output", "%s", config->output_file);

    // Read input PGM
    pgm_status_t pgm_result = pgm_read(config->input_file, &pgm);
    if (pgm_result != PGM_SUCCESS)
    {
        log_error("Failed to read PGM file");
        status = convert_pgm_status(pgm_result);
        goto cleanup;
    }
    pgm_initialized = true;

    // Create and initialize quadtree
    op_status = qtree_init(&tree, pgm.size);
    if (op_status != QTREE_SUCCESS)
    {
        log_error("Failed to initialize quadtree");
        status = convert_qtree_status(op_status);
        goto cleanup;
    }
    tree_initialized = true;

    // Build quadtree from image data
    op_status = qtree_build(&tree, pgm.pixels, pgm.size, config->input_file);
    if (op_status != QTREE_SUCCESS)
    {
        log_error("Failed to build quadtree");
        status = convert_qtree_status(op_status);
        goto cleanup;
    }

    // Apply lossy compression if requested
    if (config->alpha > 1.0f)
    {
        op_status = apply_lossy_compression(&tree, config->alpha);
        if (op_status != QTREE_SUCCESS)
        {
            log_error("Failed to apply lossy compression");
            status = convert_qtree_status(op_status);
            goto cleanup;
        }
    }

    // Open output file - only after all preprocessing is done
    output = fopen(config->output_file, "wb");
    if (!output)
    {
        log_error("Failed to open output file: %s", config->output_file);
        status = CODEC_ERROR_FILE_IO;
        goto cleanup;
    }

    // Perform compression
    op_status = compress(&tree, config->output_file, output);
    if (op_status != QTREE_SUCCESS)
    {
        log_error("Failed to compress data");
        status = convert_qtree_status(op_status);
        goto cleanup;
    }

    if (config->generate_grid)
    {
        qtree_generate_grid(&tree, config->grid_file);
    }

    log_success("Compression completed successfully");

cleanup:
    if (output)
    {
        fclose(output);
    }
    if (tree_initialized)
    {
        free_quadtree_recursive(tree.root);
    }
    if (pgm_initialized)
    {
        pgm_free(&pgm);
    }
    return status;
}

codec_status_t codec_decompress(const config_t *config)
{
    if (!config || !config->input_file || !config->output_file)
    {
        log_error("Invalid decompression parameters");
        return CODEC_ERROR_INVALID_PARAM;
    }

    qtree_t tree = {0};
    pgm_t pgm = {0};
    codec_status_t status = CODEC_SUCCESS;
    bool tree_initialized = false;
    bool pgm_initialized = false;
    FILE *input = NULL;
    qtree_status_t op_status;

    log_subheader("Decompression Operation");
    log_item("Input", "%s", config->input_file);
    log_item("Output", "%s", config->output_file);

    // Open input file
    input = fopen(config->input_file, "rb");
    if (!input)
    {
        log_error("Failed to open input file: %s", config->input_file);
        status = CODEC_ERROR_FILE_IO;
        goto cleanup;
    }

    // Read compressed data
    op_status = qtree_decompress(input, config->input_file, &tree);
    if (op_status != QTREE_SUCCESS)
    {
        log_error("Failed to read compressed data");
        status = convert_qtree_status(op_status);
        goto cleanup;
    }
    tree_initialized = true;

    // Convert to PGM format
    op_status = qtree_to_pgm(&tree, config->output_file, &pgm);
    if (op_status != QTREE_SUCCESS)
    {
        log_error("Failed to convert to PGM format");
        status = convert_qtree_status(op_status);
        goto cleanup;
    }
    pgm_initialized = true;

    // Write output file
    pgm_status_t write_status = pgm_write(&pgm, config->output_file);
    if (write_status != PGM_SUCCESS)
    {
        log_error("Failed to write PGM file");
        status = convert_pgm_status(write_status);
        goto cleanup;
    }

    if (config->generate_grid)
    {
        qtree_generate_grid(&tree, config->grid_file);
    }

    log_success("Decompression completed successfully");

cleanup:
    if (input)
    {
        fclose(input);
    }
    if (pgm_initialized)
    {
        pgm_free(&pgm);
    }
    if (tree_initialized)
    {
        free_quadtree_recursive(tree.root);
    }
    return status;
}