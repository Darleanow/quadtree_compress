/**
 * @file quadtree_compress.c
 * @brief Implementation of lossless quadtree compression
 *
 * This file implements a lossless compression algorithm for quadtree structures.
 * It provides:
 * - Bit-level data encoding
 * - File header generation with metadata
 * - Node-by-node recursive compression
 * - Progress tracking with detailed logging
 * - Compression statistics computation
 */

#include "codec/compression.h"
#include "logger/logger.h"
#include "common/common.h"
#include "common/common.h"

#include <time.h>
#include <math.h>

#define MAGIC_BYTES "Q1"

/**
 * @brief Initialize a new compression state
 * @param file Output file pointer
 * @return Initialized compression state
 */
qtree_compress_state_t compress_init(FILE *file)
{
    return (qtree_compress_state_t){
        .buffer = 0,
        .bit_position = 0,
        .file = file,
        .bytes_written = 0,
        .total_bits = 0,
        .error = 0,
        .total_nodes = 0,
        .processed_nodes = 0};
}

/**
 * @brief Write a single bit to the output stream
 * @param state Current compression state
 * @param bit Bit to write (0 or 1)
 */
void compress_write_bit(qtree_compress_state_t *state, const uint8_t bit)
{
    if (state->error)
        return;

    state->buffer |= (bit & 1) << (7 - state->bit_position++);
    state->total_bits++;

    if (state->bit_position == 8)
    {
        if (fwrite(&state->buffer, 1, 1, state->file) != 1)
        {
            state->error = 1;
            return;
        }
        state->buffer = 0;
        state->bit_position = 0;
        state->bytes_written++;
    }
}

/**
 * @brief Write multiple bits to the output stream
 * @param state Current compression state
 * @param value Value to write
 * @param num_bits Number of bits to write
 */
void compress_write_bits(qtree_compress_state_t *state, const uint32_t value, const size_t num_bits)
{
    for (size_t i = 0; i < num_bits && !state->error; i++)
    {
        compress_write_bit(state, value >> (num_bits - 1 - i) & 1);
    }
}

/**
 * @brief Flush any remaining bits in the buffer
 * @param state Current compression state
 */
void compress_flush(qtree_compress_state_t *state)
{
    if (state->error || state->bit_position == 0)
        return;

    if (fwrite(&state->buffer, 1, 1, state->file) != 1)
    {
        state->error = 1;
        return;
    }

    state->buffer = 0;
    state->bit_position = 0;
    state->bytes_written++;
}

/**
 * @brief Calculate compression rate
 * @param total_bits Total bits after compression
 * @param original_size Original size in bits
 * @return Compression rate as percentage
 */
float compress_get_rate(const size_t total_bits, const size_t original_size)
{
    return (float)total_bits / (float)original_size * 100.0f;
}

/**
 * @brief Write file header including metadata
 *
 * Writes a header containing:
 * - Magic bytes ("Q1")
 * - Timestamp of the compression operation
 * - Compression rate (percentage)
 * - Tree depth (`n_levels`)
 *
 * @param file Output file pointer
 * @param tree Pointer to the quadtree being compressed
 * @param compression_rate Calculated compression rate
 * @return True if the header was written successfully, false otherwise
 */
static bool write_header(FILE *file, const qtree_t *tree, const float compression_rate)
{
    char timestamp[64];
    time_t now;

    // Write magic bytes and format version
    if (fwrite(MAGIC_BYTES, 1, 2, file) != 2)
        return false;
    if (fputc('\n', file) == EOF)
        return false;

    // Write timestamp
    time(&now);
    strftime(timestamp, sizeof(timestamp), "# %a %b %d %H:%M:%S %Y\n", localtime(&now));
    if (fputs(timestamp, file) == EOF)
        return false;

    // Write compression statistics
    if (fprintf(file, "# compression rate %.2f%%\n", compression_rate) < 0)
        return false;

    // Write tree depth
    return fwrite(&tree->n_levels, sizeof(uint8_t), 1, file) == 1;
}

/**
 * @brief Write node data to temporary buffer
 *
 * @param state Current compression state
 * @param node Pointer to the node to write
 * @param is_leaf True if the node is a leaf
 * @param is_interpolated True if the node's value is derived via interpolation
 */
static void write_node(qtree_compress_state_t *state, const qtree_node_t *node,
                       const bool is_leaf, const bool is_interpolated)
{
    if (state->error)
        return;

    // Write mean value if not interpolated
    if (!is_interpolated)
    {
        compress_write_bits(state, node->m, 8);
    }

    if (is_leaf)
        return;

    // Write error value (2 bits)
    compress_write_bits(state, node->e, 2);

    // Write uniformity bit if error is 0
    if (node->e == 0)
    {
        compress_write_bits(state, node->u, 1);
    }

    // Increment processed nodes count
    state->processed_nodes++;
}

/**
 * @brief Recursively compress a level of the quadtree
 *
 * Traverses the quadtree recursively and encodes nodes at the specified target level.
 * Nodes are processed in a specific order determined by `quadrant_order`.
 *
 * @param state Current compression state
 * @param tree Pointer to the quadtree being compressed
 * @param node Pointer to the current node
 * @param current_level Current depth in the tree
 * @param target_level Target depth to process
 * @param is_interpolated True if the current node is interpolated
 *
 * @note This function does nothing if the node is uniform or null.
 */
static void compress_tree_level(qtree_compress_state_t *state, const qtree_t *tree,
                                const qtree_node_t *node, const uint32_t current_level,
                                const uint32_t target_level, const bool is_interpolated)
{
    if (node == NULL || state->error)
        return;

    const bool is_leaf = node->e == 0 && node->u == 1 && current_level == tree->n_levels;

    // Write node data if we're at the target level
    if (current_level == target_level)
    {
        write_node(state, node, is_leaf, is_interpolated);
        return;
    }

    // Recursively process children if not uniform
    if (node->u == 0)
    {
        for (int i = 0; i < 4; i++)
        {
            compress_tree_level(state, tree, node->children[quadrant_order[i]],
                                current_level + 1, target_level, i == 3);
        }
    }
}

/**
 * @brief Compress quadtree data to a temporary buffer
 *
 * @param state Compression state
 * @param tree Quadtree to compress
 * @return True if compression succeeded, false otherwise
 */
static bool compress_tree_data(qtree_compress_state_t *state, const qtree_t *tree)
{
    log_item("Tree depth", "%u levels", tree->n_levels);
    log_item("Image size", "%ux%u pixels", tree->size, tree->size);

    // Process each level
    for (uint32_t level = 0; level <= tree->n_levels; level++)
    {
        compress_tree_level(state, tree, tree->root, 0, level, false);
        if (state->error)
        {
            log_message(LOG_LEVEL_ERROR, "Failed at level %u", level);
            return false;
        }

        const double progress = (double)level / (double)tree->n_levels;
        log_progress(progress);
    }

    compress_flush(state);
    return !state->error;
}

/**
 * @brief Compress a quadtree structure
 */
qtree_status_t compress(const qtree_t *tree, const char *output_filename, FILE *output_file)
{
    log_header("QUADTREE COMPRESSION");

    // Validate parameters
    if (!tree || !output_file || !tree->root)
    {
        log_message(LOG_LEVEL_ERROR, "Invalid compression parameters");
        return QTREE_ERROR_INVALID_PARAM;
    }

    // Log initial file information
    log_file_info("input.pgm", tree->size, tree->n_levels, 0.0);

    log_subheader("Preprocessing Data");

    // Create temporary buffer for two-pass compression
    FILE *temp_buffer = tmpfile();
    if (!temp_buffer)
    {
        log_message(LOG_LEVEL_ERROR, "Failed to create temporary buffer");
        return QTREE_ERROR_FORMAT;
    }

    // First pass: compress to get exact size
    qtree_compress_state_t temp_state = compress_init(temp_buffer);

    log_message(LOG_LEVEL_SUCCESS, "Successfully made first pass");

    // Track compression time
    clock_t start_time = clock();

    log_subheader("Compressing Data");
    if (!compress_tree_data(&temp_state, tree))
    {
        fclose(temp_buffer);
        log_message(LOG_LEVEL_ERROR, "Compression failed during data encoding");
        return QTREE_ERROR_FORMAT;
    }

    // Calculate compression statistics
    const size_t original_size = tree->size * tree->size * 8;
    const float compression_rate = compress_get_rate(temp_state.total_bits, original_size);
    const double cpu_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;

    log_end_progress();

    // Write header with actual compression rate
    log_subheader("Writing Output");
    log_item("Output path", "%s", output_filename);
    log_item("Writing header", "Q1 format");

    if (!write_header(output_file, tree, compression_rate))
    {
        fclose(temp_buffer);
        log_message(LOG_LEVEL_ERROR, "Failed to write file header");
        return QTREE_ERROR_FORMAT;
    }

    // Copy compressed data to output file
    log_item("Copying data", "%.2f KB", (double)temp_state.total_bits / 8192.0);

    rewind(temp_buffer);
    char buffer[4096];
    size_t bytes_read;
    size_t total_copied = 0;
    const size_t total_bytes = (temp_state.total_bits + 7) / 8;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), temp_buffer)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, output_file) != bytes_read)
        {
            fclose(temp_buffer);
            log_message(LOG_LEVEL_ERROR, "Failed to write compressed data");
            return QTREE_ERROR_FORMAT;
        }
        total_copied += bytes_read;
        log_progress((double)total_copied / (double)total_bytes);
    }

    log_end_progress();
    fclose(temp_buffer);

    // Log final statistics
    log_size_stats(original_size, temp_state.total_bits,
                   temp_state.processed_nodes, cpu_time);

    log_message(LOG_LEVEL_SUCCESS, "Compression completed with %.2f%% ratio",
                (double)compression_rate);

    return QTREE_SUCCESS;
}

static void update_node_variance(qtree_node_t *node)
{
    if (!node || qtree_is_leaf(node))
        return;

    float sum = 0.0f;
    for (int i = 0; i < 4; i++)
    {
        if (node->children[i])
        {
            // Add squared variance of child
            sum += node->children[i]->v * node->children[i]->v;
            // Add squared difference of means
            float diff = node->m - node->children[i]->m;
            sum += diff * diff;
        }
    }
    node->v = sqrtf(sum / 4.0f);
}

static bool is_uniform_block(qtree_node_t *node)
{
    if (!node)
        return true;
    if (node->e != 0)
        return false;

    // Check if all children are present and uniform
    for (int i = 0; i < 4; i++)
    {
        if (node->children[i] && !node->children[i]->u)
        {
            return false;
        }
    }

    // Check if all child means are equal
    if (node->children[0])
    {
        uint8_t mean = node->children[0]->m;
        for (int i = 1; i < 4; i++)
        {
            if (node->children[i] && node->children[i]->m != mean)
            {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Apply variance-based filtering to a node and its subtree
 */
static bool filter_node_recursive(qtree_node_t *node, float threshold, float alpha)
{
    if (!node || qtree_is_leaf(node))
    {
        return true;
    }

    // First calculate variance properly
    update_node_variance(node);

    // Process children with increased threshold
    bool all_children_uniform = true;
    for (int i = 0; i < 4; i++)
    {
        if (node->children[i])
        {
            if (!filter_node_recursive(node->children[i], threshold * alpha, alpha))
            {
                all_children_uniform = false;
            }
        }
    }

    // Node can be made uniform if:
    // 1. Its variance is below threshold AND
    // 2. All its children are uniform or it's at the leaf level
    if ((node->v <= threshold) && all_children_uniform)
    {
        // Make node uniform and free children
        node->u = 1;
        node->e = 0;
        for (int i = 0; i < 4; i++)
        {
            if (node->children[i])
            {
                free_quadtree_recursive(node->children[i]);
                node->children[i] = NULL;
            }
        }
        return true;
    }
    else
    {
        // Update uniformity flag based on children
        node->u = is_uniform_block(node);
        return node->u;
    }
}

qtree_status_t apply_lossy_compression(qtree_t *tree, float alpha)
{
    if (!tree || !tree->root || alpha <= 1.0f)
        return QTREE_ERROR_INVALID_PARAM;

    log_subheader("Applying Lossy Filtering");
    log_item("Alpha parameter", "%.2f", (double)alpha);

    // Calculate initial statistics
    qtree_variance_stats_t stats = calculate_variance_stats(tree);
    float initial_threshold = stats.median_variance / stats.max_variance;

    log_item("Initial threshold", "%.4f", (double)initial_threshold);
    log_item("Median variance", "%.4f", (double)stats.median_variance);
    log_item("Maximum variance", "%.4f", (double)stats.max_variance);

    // Apply filtering
    filter_node_recursive(tree->root, initial_threshold, alpha);

    log_message(LOG_LEVEL_SUCCESS, "Lossy filtering applied successfully");
    return QTREE_SUCCESS;
}