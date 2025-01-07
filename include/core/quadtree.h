/**
 * @file quadtree.h
 * @brief Core quadtree stuff that everything else uses
 */

#ifndef QUADTREE_H
#define QUADTREE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief One node in our tree
 *
 * Each node keeps track of:
 * - Mean value of the area (m)
 * - Small error from rounding (e)
 * - If it's uniform or not (u)
 * - How much variation there is (v)
 * - Its four children, if any
 */
typedef struct qtree_node
{
    uint8_t m;                      /* Average intensity */
    uint8_t e : 2;                  /* Rounding error */
    uint8_t u : 1;                  /* Is it uniform? */
    float v;                        /* How much variation */
    struct qtree_node *children[4]; /* Sub-regions */
} qtree_node_t;

/**
 * @brief Info about how varied our image is
 */
typedef struct
{
    float median_variance; /* Typical variation */
    float max_variance;    /* Biggest variation */
} qtree_variance_stats_t;

/**
 * @brief The whole quadtree structure
 */
typedef struct
{
    qtree_node_t *root; /* Top of the tree */
    uint32_t n_levels;  /* How many layers */
    uint32_t size;      /* Image size (must be 2^n) */
} qtree_t;

/**
 * @brief What can go wrong
 */
typedef enum
{
    QTREE_SUCCESS = 0,         /* All good */
    QTREE_ERROR_MEMORY,        /* Out of memory */
    QTREE_ERROR_INVALID_PARAM, /* Bad input */
    QTREE_ERROR_FORMAT,        /* File problems */
} qtree_status_t;

/**
 * @brief Gets a tree ready for use
 */
qtree_status_t qtree_init(qtree_t *tree, uint32_t size);

/**
 * @brief Makes a tree from an image
 */
qtree_status_t qtree_build(qtree_t *tree, const uint8_t *pixels,
                           uint32_t size, const char *input_filename);

/**
 * @brief Cleans up memory
 */
void free_quadtree_recursive(qtree_node_t *node);

/**
 * @brief Finds a node's parent
 */
uint32_t qtree_parent_index(uint32_t index);

/**
 * @brief Finds a node's first child
 */
uint32_t qtree_first_child_index(uint32_t index);

/**
 * @brief Checks if a node is at the bottom
 */
bool qtree_is_leaf(const qtree_node_t *node);

/**
 * @brief Calculates how varied our image is
 */
qtree_variance_stats_t calculate_variance_stats(const qtree_t *tree);

#endif /* QUADTREE_H */