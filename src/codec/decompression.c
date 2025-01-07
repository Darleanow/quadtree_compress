/**
 * @file quadtree_decompress.c
 * @brief Enhanced quadtree decompression with beautiful logging and statistics
 */

#include "codec/decompression.h"
#include "logger/logger.h"
#include "common/common.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

static void extract_pixels(const qtree_node_t *node, uint8_t *pixels,
                           uint32_t row, uint32_t col,
                           uint32_t size, uint32_t total_size);

/**
 * @brief Enhanced statistics tracking for detailed progress reporting
 */
typedef struct
{
    struct
    {
        size_t total;     // Total nodes estimate
        size_t processed; // Currently processed count
        double rate;      // Processing rate (nodes/sec)
    } nodes;

    struct
    {
        size_t read;     // Bits read from input
        size_t original; // Original image size in bits
        double ratio;    // Compression ratio
    } bits;

    struct
    {
        uint32_t current; // Current processing level
        uint32_t max;     // Maximum tree depth
        double progress;  // Overall progress (0-1)
    } levels;

    clock_t start_time; // Processing start timestamp
} decompress_stats_t;

/**
 * @brief Enhanced bit reader with error tracking
 */
typedef struct
{
    uint8_t buffer;            // Current byte buffer
    size_t position;           // Current bit position
    FILE *file;                // Input file stream
    bool eof;                  // End-of-file indicator
    decompress_stats_t *stats; // Statistics reference
    bool has_error;            // Error state indicator
    const char *error_msg;     // Error message if any
} bit_reader_t;

static void update_progress(decompress_stats_t *stats)
{
    stats->levels.progress = (double)stats->levels.current / stats->levels.max;
    stats->nodes.rate = (double)stats->nodes.processed /
                        ((double)(clock() - stats->start_time) / CLOCKS_PER_SEC);

    // Update compression ratio
    if (stats->bits.original > 0)
    {
        stats->bits.ratio = (double)stats->bits.read / (double)stats->bits.original * 100.0;
    }

    log_progress((double)stats->levels.progress);
}

static decompress_stats_t init_stats(uint32_t max_levels, size_t image_size)
{
    const size_t total_nodes = (1ULL << (max_levels * 2)) - 1;
    const size_t original_bits = image_size * image_size * 8;

    return (decompress_stats_t){
        .nodes = {
            .total = total_nodes,
            .processed = 0,
            .rate = 0.0},
        .bits = {.read = 0, .original = original_bits, .ratio = 0.0},
        .levels = {.current = 0, .max = max_levels, .progress = 0.0},
        .start_time = clock()};
}

static uint8_t read_bit(bit_reader_t *reader)
{
    if (reader->has_error || reader->eof)
    {
        return 0;
    }

    if (reader->position == 8)
    {
        size_t bytes_read = fread(&reader->buffer, 1, 1, reader->file);
        if (bytes_read != 1)
        {
            reader->eof = true;
            reader->has_error = true;
            reader->error_msg = "Unexpected end of file while reading bits";
            return 0;
        }
        reader->position = 0;
    }

    uint8_t bit = (reader->buffer >> (7 - reader->position)) & 1;
    reader->position++;
    reader->stats->bits.read++;
    return bit;
}

static uint8_t read_bits(bit_reader_t *reader, size_t num_bits)
{
    if (reader->has_error || reader->eof || num_bits > 8)
    {
        return 0;
    }

    uint8_t value = 0;
    for (size_t i = 0; i < num_bits; i++)
    {
        value = (uint8_t)((value << 1) | read_bit(reader));
        if (reader->eof && i < num_bits - 1)
        {
            reader->has_error = true;
            reader->error_msg = "Incomplete byte while reading multiple bits";
            return 0;
        }
    }
    return value;
}

static void process_file_header(FILE *file, char *magic, uint8_t *levels)
{
    // Read and validate magic number
    if (fread(magic, 1, 2, file) != 2 || strncmp(magic, "Q1", 2) != 0 ||
        fgetc(file) != '\n')
    {
        log_message(LOG_LEVEL_ERROR, "Invalid file signature (expected 'Q1')");
        return;
    }
    log_item("Signature", "Q1 (valid)");

    // Process metadata comments
    char line[256];
    int comments_read = 0;
    while (comments_read < 2 && fgets(line, sizeof(line), file))
    {
        // Trim newline if present
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
        }
        log_message(LOG_LEVEL_INFO, "Header: %s", line);
        comments_read++;
    }

    // Read tree depth
    if (fread(levels, 1, 1, file) != 1)
    {
        log_message(LOG_LEVEL_ERROR, "Failed to read tree depth");
        return;
    }

    if (*levels < 1 || *levels > 32)
    {
        log_message(LOG_LEVEL_ERROR, "Invalid tree depth: %u (must be 1-32)", *levels);
        return;
    }

    log_item("Tree Depth", "%u levels", *levels);
}

static qtree_node_t *decompress_node(bit_reader_t *reader, uint32_t level,
                                     uint32_t max_level, qtree_node_t *parent,
                                     int child_index)
{
    if (reader->has_error)
    {
        return NULL;
    }

    qtree_node_t *node = calloc(1, sizeof(qtree_node_t));
    if (!node)
    {
        reader->has_error = true;
        reader->error_msg = "Memory allocation failed for new node";
        return NULL;
    }

    reader->stats->nodes.processed++;

    // Handle mean value computation
    if (child_index < 3)
    {
        node->m = read_bits(reader, 8);
    }
    else
    {
        if (!parent || !parent->children[0] || !parent->children[1] ||
            !parent->children[2])
        {
            reader->has_error = true;
            reader->error_msg = "Invalid parent node structure";
            free(node);
            return NULL;
        }
        node->m = calculate_fourth_mean(parent->m, parent->e,
                                        parent->children[0]->m,
                                        parent->children[1]->m,
                                        parent->children[2]->m);
    }

    // Process node attributes based on level
    if (level < max_level)
    {
        node->e = read_bits(reader, 2);
        node->u = (node->e == 0) ? read_bit(reader) : 0;
    }
    else
    {
        node->e = 0;
        node->u = 1;
    }

    return node;
}

static qtree_node_t **decompress_level(bit_reader_t *reader, uint32_t level,
                                       uint32_t max_level, qtree_node_t **prev_level,
                                       size_t prev_level_size)
{
    // Count non-uniform nodes at previous level
    size_t non_uniform_count = 0;
    for (size_t i = 0; i < prev_level_size; i++)
    {
        if (prev_level[i] && !prev_level[i]->u)
        {
            non_uniform_count++;
        }
    }

    // Allocate new level
    size_t current_level_size = non_uniform_count * 4;
    qtree_node_t **current_level = malloc(sizeof(qtree_node_t *) * current_level_size);
    if (!current_level)
    {
        reader->has_error = true;
        reader->error_msg = "Memory allocation failed for level nodes";
        return NULL;
    }

    // Process nodes at current level
    size_t current_idx = 0;
    for (size_t i = 0; i < prev_level_size && !reader->has_error; i++)
    {
        qtree_node_t *parent = prev_level[i];
        if (!parent || parent->u)
        {
            continue;
        }

        for (int j = 0; j < 4; j++)
        {
            current_level[current_idx] = decompress_node(reader, level, max_level,
                                                         parent, j);
            if (!current_level[current_idx])
            {
                for (size_t k = 0; k < current_idx; k++)
                {
                    free(current_level[k]);
                }
                free(current_level);
                return NULL;
            }
            parent->children[j] = current_level[current_idx];
            current_idx++;
        }
    }

    // Update progress
    reader->stats->levels.current = level;
    update_progress(reader->stats);

    return current_level;
}

qtree_status_t qtree_decompress(FILE *file, const char *input_filename, qtree_t *tree)
{
    log_header("QUADTREE DECOMPRESSION");

    // Validate input parameters
    if (!file || !tree)
    {
        log_message(LOG_LEVEL_ERROR, "Invalid input parameters");
        return QTREE_ERROR_INVALID_PARAM;
    }

    // Process file header
    char magic[3] = {0};
    uint8_t n_levels;

    log_subheader("Processing File Header");
    log_item("Input path", input_filename);

    process_file_header(file, magic, &n_levels);

    // Initialize tree structure
    tree->n_levels = n_levels;
    tree->size = 1 << n_levels;

    // Initialize statistics and bit reader
    decompress_stats_t stats = init_stats(n_levels, tree->size);
    bit_reader_t reader = {
        .buffer = 0,
        .position = 8,
        .file = file,
        .eof = false,
        .stats = &stats,
        .has_error = false,
        .error_msg = NULL};

    // Display initial file information
    log_file_info("input.qtc", tree->size, n_levels, 0.0);

    log_subheader("Decompressing Data");

    // Process root node
    tree->root = decompress_node(&reader, 0, n_levels, NULL, 0);
    if (!tree->root)
    {
        log_message(LOG_LEVEL_ERROR, "Root node decompression failed: %s",
                    reader.error_msg ? reader.error_msg : "Unknown error");
        return QTREE_ERROR_FORMAT;
    }

    // Process remaining levels
    qtree_node_t **prev_level = malloc(sizeof(qtree_node_t *));
    if (!prev_level)
    {
        log_message(LOG_LEVEL_ERROR, "Memory allocation failed");
        return QTREE_ERROR_MEMORY;
    }

    prev_level[0] = tree->root;
    size_t prev_level_size = 1;

    for (uint32_t level = 1; level <= n_levels && !reader.has_error; level++)
    {
        qtree_node_t **current_level = decompress_level(&reader, level, n_levels,
                                                        prev_level, prev_level_size);
        if (!current_level)
        {
            log_message(LOG_LEVEL_ERROR, "Level %u decompression failed: %s",
                        level, reader.error_msg ? reader.error_msg : "Unknown error");
            free(prev_level);
            return QTREE_ERROR_FORMAT;
        }

        // Calculate new level size
        size_t non_uniform_count = 0;
        for (size_t i = 0; i < prev_level_size; i++)
        {
            if (prev_level[i] && !prev_level[i]->u)
            {
                non_uniform_count++;
            }
        }

        free(prev_level);
        prev_level = current_level;
        prev_level_size = non_uniform_count * 4;
    }

    // Calculate final statistics
    double cpu_time = (double)(clock() - stats.start_time) / CLOCKS_PER_SEC;

    log_end_progress();
    log_size_stats(stats.bits.original, stats.bits.read,
                   stats.nodes.processed, cpu_time);

    if (!reader.has_error)
    {
        log_message(LOG_LEVEL_SUCCESS, "Decompression completed successfully");
    }

    free(prev_level);
    return reader.has_error ? QTREE_ERROR_FORMAT : QTREE_SUCCESS;
}

qtree_status_t qtree_to_pgm(const qtree_t *tree, const char *output_filename, pgm_t *pgm)
{
    log_header("PGM CONVERSION");

    // Validate input parameters
    if (!tree || !tree->root || !pgm)
    {
        log_message(LOG_LEVEL_ERROR, "Invalid conversion parameters");
        return QTREE_ERROR_INVALID_PARAM;
    }

    log_subheader("Initializing Conversion");
    log_item("Output path", output_filename);

    // Initialize PGM structure with proper dimensions
    pgm->size = tree->size;
    pgm->max_value = 255;

    // Calculate memory requirements and allocate pixel buffer
    size_t total_pixels = tree->size * tree->size;
    pgm->pixels = calloc(total_pixels, sizeof(uint8_t));
    if (!pgm->pixels)
    {
        log_message(LOG_LEVEL_ERROR, "Memory allocation failed for pixel data");
        return QTREE_ERROR_MEMORY;
    }

    // Log conversion parameters and memory usage
    log_subheader("Converting Data");
    log_item("Image dimensions", "%zux%zu pixels", tree->size, tree->size);
    log_item("Memory allocated", "%.2f KB", (double)(total_pixels) / 1024.0);

    // Start pixel extraction with progress tracking
    clock_t start_time = clock();

    // Extract pixels recursively from the quadtree
    extract_pixels(tree->root, pgm->pixels, 0, 0, tree->size, tree->size);

    // Calculate and log performance metrics
    double cpu_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
    double pixels_per_sec = (double)total_pixels / cpu_time;

    log_item("Processing rate", "%.2f MP/s", pixels_per_sec / 1000000.0);
    log_item("Processing time", "%.3f seconds", cpu_time);

    log_separator();
    log_message(LOG_LEVEL_SUCCESS, "PGM conversion completed successfully");

    return QTREE_SUCCESS;
}

/**
 * @brief Extract pixels from quadtree nodes into linear pixel array
 *
 * This function recursively traverses the quadtree and fills the pixel array
 * according to the quadrant ordering defined in the display configuration.
 * For uniform nodes, it efficiently fills the entire block with the same value.
 */
static void extract_pixels(const qtree_node_t *node, uint8_t *pixels,
                           uint32_t row, uint32_t col,
                           uint32_t size, uint32_t total_size)
{
    if (!node)
        return;

    // Handle leaf nodes and uniform regions
    if (node->u || size == 1)
    {
        for (uint32_t i = row; i < row + size && i < total_size; i++)
        {
            for (uint32_t j = col; j < col + size && j < total_size; j++)
            {
                pixels[i * total_size + j] = node->m;
            }
        }
        return;
    }

    // Process child nodes in the correct display order
    uint32_t half_size = size / 2;
    for (int i = 0; i < 4; i++)
    {
        uint32_t new_row = row;
        uint32_t new_col = col;

        // Calculate position based on quadrant
        switch (quadrant_order[i])
        {
        case QUADRANT_TOP_RIGHT:
            new_col += half_size;
            break;
        case QUADRANT_BOTTOM_RIGHT:
            new_row += half_size;
            new_col += half_size;
            break;
        case QUADRANT_BOTTOM_LEFT:
            new_row += half_size;
            break;
        default: // QUADRANT_TOP_LEFT
            break;
        }

        // Recursively process each quadrant
        extract_pixels(node->children[quadrant_order[i]], pixels,
                       new_row, new_col, half_size, total_size);
    }
}
