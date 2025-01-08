/**
 * @file quadtree.c
 * @brief Quadtree implementation with progress tracking
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "core/quadtree.h"
#include "logger/logger.h"
#include "common/common.h"

/* Node tracking for progress updates */
typedef struct
{
    uint32_t processed;
    uint32_t total;
} progress_tracker_t;

/**
 * @brief Create and initialize a new node
 */
static qtree_node_t *create_node(void)
{
    qtree_node_t *node = malloc(sizeof(qtree_node_t));
    if (node)
    {
        node->m = 0;
        node->e = 0;
        node->u = 0;
        memset(node->children, 0, sizeof(node->children));
    }
    return node;
}

/**
 * @brief Calculate total nodes needed for progress tracking
 */
static uint32_t calculate_total_nodes(uint32_t levels)
{
    uint32_t total = 0;
    uint32_t nodes_at_level = 1;

    for (uint32_t i = 0; i <= levels; i++)
    {
        total += nodes_at_level;
        nodes_at_level *= 4;
    }
    return total;
}

/**
 * @brief Create a leaf node
 */
static qtree_node_t *create_leaf_node(const uint8_t *pixels, uint32_t size,
                                      uint32_t row, uint32_t col)
{
    qtree_node_t *node = create_node();
    if (!node)
        return NULL;

    node->m = pixels[row * size + col];
    node->e = 0;
    node->u = 1;

    return node;
}

/**
 * @brief Calculate node properties and check uniformity
 */
static bool calculate_node_properties(qtree_node_t *node)
{
    if (!node || !node->children[0])
        return false;

    const uint8_t m1 = node->children[0]->m;
    const uint8_t m2 = node->children[1]->m;
    const uint8_t m3 = node->children[2]->m;
    const uint8_t m4 = node->children[3]->m;

    // Using uint32_t to safely hold sum of four uint8_t values
    const uint32_t sum = (uint32_t)m1 + (uint32_t)m2 + (uint32_t)m3 + (uint32_t)m4;
    const uint8_t mean = (uint8_t)(sum / 4);
    const uint8_t error_val = (uint8_t)(sum % 4);

    const bool all_uniform = node->children[0]->u && node->children[1]->u &&
                            node->children[2]->u && node->children[3]->u;
    const bool all_same = (m1 == m2) && (m2 == m3) && (m3 == m4);
    const bool is_uniform = (error_val == 0) && all_uniform && all_same;

    node->m = mean;
    node->e = (unsigned char)(error_val & 0x3);  // Ensure 2-bit value
    node->u = (unsigned char)(is_uniform ? 1 : 0);

    return is_uniform;
}

/**
 * @brief Build quadtree recursively with progress tracking
 */
static qtree_node_t *build_recursive(const uint8_t *pixels, uint32_t size,
                                     uint32_t level, uint32_t row, uint32_t col,
                                     progress_tracker_t *progress)
{
    // Create node and update progress
    qtree_node_t *node = create_node();
    if (!node)
        return NULL;

    progress->processed++;
    if (progress->processed % (progress->total / 100) == 0 ||
        progress->processed == progress->total)
    {
        double percent = (double)progress->processed / (double)progress->total;
        log_progress(percent);
    }

    // Handle leaf nodes
    if (level == 0)
    {
        qtree_node_t *leaf = create_leaf_node(pixels, size, row, col);
        free(node); // Free the unnecessary node
        return leaf;
    }

    // Process children
    uint32_t step = 1 << (level - 1);
    for (int i = 0; i < 4; i++)
    {
        int q = quadrant_order[i];
        uint32_t new_row = row;
        uint32_t new_col = col;

        if (q & 2)
            new_row += step; // Bottom quadrants
        if ((q & 1) ^ ((q & 2) >> 1))
            new_col += step; // Right quadrants

        node->children[q] = build_recursive(pixels, size, level - 1,
                                            new_row, new_col, progress);
        if (!node->children[q])
        {
            free_quadtree_recursive(node);
            return NULL;
        }
    }

    // Process node properties
    if (calculate_node_properties(node))
    {
        // Convert to leaf if uniform
        for (int i = 0; i < 4; i++)
        {
            free_quadtree_recursive(node->children[i]);
            node->children[i] = NULL;
        }
    }

    return node;
}

qtree_status_t qtree_init(qtree_t *tree, uint32_t size)
{
    if (!tree || size == 0 || (size & (size - 1)) != 0)
    {
        log_message(LOG_LEVEL_ERROR, "Invalid parameters for quadtree initialization");
        return QTREE_ERROR_INVALID_PARAM;
    }

    tree->size = size;
    tree->n_levels = (uint32_t)(floor(log2(size)));
    tree->root = NULL;

    log_message(LOG_LEVEL_INFO, "Initialized quadtree structure (%ux%u)", size, size);
    return QTREE_SUCCESS;
}

qtree_status_t qtree_build(qtree_t *tree, const uint8_t *pixels, uint32_t size, const char *input_filename)
{
    if (!tree || !pixels || size == 0 || size != tree->size)
    {
        log_message(LOG_LEVEL_ERROR, "Invalid parameters for quadtree build");
        return QTREE_ERROR_INVALID_PARAM;
    }

    log_header("QUADTREE CONSTRUCTION");

    // Log initial image information
    log_subheader("Image Information");
    log_item("Input path", input_filename);
    log_item("Dimensions", "%ux%u pixels", size, size);
    log_item("Tree depth", "%u levels", tree->n_levels);
    log_item("Maximum nodes", "%u nodes", calculate_total_nodes(tree->n_levels));

    // Setup progress tracking
    progress_tracker_t progress = {
        .processed = 0,
        .total = calculate_total_nodes(tree->n_levels)};

    // Track construction time
    clock_t start_time = clock();

    log_subheader("Building Tree Structure");
    tree->root = build_recursive(pixels, size, tree->n_levels, 0, 0, &progress);

    double cpu_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
    log_end_progress();

    if (!tree->root)
    {
        log_message(LOG_LEVEL_ERROR, "Failed to build quadtree");
        return QTREE_ERROR_MEMORY;
    }

    // Log final statistics
    log_subheader("Construction Statistics");
    log_item("Total nodes", "%u nodes", progress.processed);
    log_item("Processing time", "%.3f seconds", cpu_time);
    log_item("Processing rate", "%.2f MNodes/s",
                (progress.processed / cpu_time) / 1000000.0);
    log_item("Memory usage", "%.2f MB",
                (progress.processed * sizeof(qtree_node_t)) / (1024.0 * 1024.0));

    log_separator();
    log_message(LOG_LEVEL_SUCCESS, "Quadtree construction completed successfully");

    return QTREE_SUCCESS;
}

void free_quadtree_recursive(qtree_node_t *node)
{
    if (!node)
        return;
    for (int i = 0; i < 4; i++)
    {
        free_quadtree_recursive(node->children[i]);
    }
    free(node);
}

uint32_t qtree_parent_index(uint32_t index)
{
    return (index - 1) / 4;
}

uint32_t qtree_first_child_index(uint32_t index)
{
    return 4 * index + 1;
}

bool qtree_is_leaf(const qtree_node_t *node)
{
    if (!node)
        return false;
    for (int i = 0; i < 4; i++)
    {
        if (node->children[i])
            return false;
    }
    return true;
}

/**
 * @brief Calculate local variance for a node and its subtree
 *
 * μ = Σ(νk² + (m - mk)²) for k=0 to 3
 * ν = √(μ/4)
 */
static void calculate_node_variance(qtree_node_t *node)
{
    if (!node || qtree_is_leaf(node))
    {
        node->v = 0.0f;
        return;
    }

    float μ = 0.0f;
    for (int k = 0; k < 4; k++)
    {
        if (node->children[k])
        {
            float diff = (float)node->m - (float)node->children[k]->m;
            μ += node->children[k]->v * node->children[k]->v + diff * diff;
        }
    }

    node->v = sqrtf(μ / 4.0f);
}

/**
 * @brief Recursively calculate variances for all nodes in the tree
 */
static void calculate_variances_recursive(qtree_node_t *node, float *variances, size_t *count)
{
    if (!node)
        return;

    for (int i = 0; i < 4; i++)
    {
        calculate_variances_recursive(node->children[i], variances, count);
    }

    calculate_node_variance(node);

    // Store non-zero variances for statistics
    if (node->v > 0.0f)
    {
        variances[*count] = node->v;
        (*count)++;
    }
}

/**
 * @brief Compare function for qsort
 */
static int compare_floats(const void *a, const void *b)
{
    float fa = *(const float *)a;
    float fb = *(const float *)b;
    return (fa > fb) - (fa < fb);
}

qtree_variance_stats_t calculate_variance_stats(const qtree_t *tree)
{
    qtree_variance_stats_t stats = {0.0f, 0.0f};
    if (!tree || !tree->root)
        return stats;

    // Allocate space for all possible variances
    uint32_t max_nodes = calculate_total_nodes(tree->n_levels);
    float *variances = malloc(max_nodes * sizeof(float));
    if (!variances)
        return stats;

    // Calculate variances and collect non-zero ones
    size_t count = 0;
    calculate_variances_recursive(tree->root, variances, &count);

    if (count > 0)
    {
        // Sort variances to find median and max
        qsort(variances, count, sizeof(float), compare_floats);
        stats.median_variance = variances[count / 2];
        stats.max_variance = variances[count - 1];
    }

    free(variances);
    return stats;
}