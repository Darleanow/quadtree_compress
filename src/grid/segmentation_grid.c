/**
 * @file quadtree_grid.c
 * @brief Implementation of grid generation functionality
 */

#include <stdlib.h>
#include <string.h>
#include "grid/segmentation_grid.h"
#include "common/common.h"

#define GRID_LINE_THICKNESS 1
#define GRID_COLOR 128 // Mid-gray

/**
 * @brief Draw a horizontal line in the grid
 */
static void draw_horizontal_line(uint8_t *pixels, const size_t size,
                                 const size_t x, const size_t y,
                                 const size_t width)
{
    for (size_t i = 0; i < width; i++)
    {
        for (size_t t = 0; t < GRID_LINE_THICKNESS; t++)
        {
            if (y + t < size)
            {
                pixels[(y + t) * size + x + i] = GRID_COLOR;
            }
        }
    }
}

/**
 * @brief Draw a vertical line in the grid
 */
static void draw_vertical_line(uint8_t *pixels, const size_t size,
                               const size_t x, const size_t y,
                               const size_t height)
{
    for (size_t i = 0; i < height; i++)
    {
        for (size_t t = 0; t < GRID_LINE_THICKNESS; t++)
        {
            if (x + t < size)
            {
                pixels[(y + i) * size + x + t] = GRID_COLOR;
            }
        }
    }
}

/**
 * @brief Recursively draw grid lines for a node
 */
static void draw_node_grid(uint8_t *pixels, const size_t size,
                           const qtree_node_t *node,
                           const size_t x, const size_t y,
                           const size_t node_size)
{
    if (!node || node_size <= 1)
    {
        return;
    }

    if (!qtree_is_leaf(node))
    {
        size_t half_size = node_size / 2;

        draw_horizontal_line(pixels, size, x, y + half_size, node_size);

        draw_vertical_line(pixels, size, x + half_size, y, node_size);

        if (node->children[QUADRANT_TOP_LEFT])
        {
            draw_node_grid(pixels, size, node->children[QUADRANT_TOP_LEFT],
                           x, y, half_size);
        }
        if (node->children[QUADRANT_TOP_RIGHT])
        {
            draw_node_grid(pixels, size, node->children[QUADRANT_TOP_RIGHT],
                           x + half_size, y, half_size);
        }
        if (node->children[QUADRANT_BOTTOM_LEFT])
        {
            draw_node_grid(pixels, size, node->children[QUADRANT_BOTTOM_LEFT],
                           x, y + half_size, half_size);
        }
        if (node->children[QUADRANT_BOTTOM_RIGHT])
        {
            draw_node_grid(pixels, size, node->children[QUADRANT_BOTTOM_RIGHT],
                           x + half_size, y + half_size, half_size);
        }
    }
}

qtree_status_t qtree_generate_grid(const qtree_t *tree, const char *output_file)
{
    if (!tree || !tree->root || !output_file)
    {
        return QTREE_ERROR_INVALID_PARAM;
    }

    // Create blank PGM image
    pgm_t grid_pgm;
    grid_pgm.size = tree->size;
    grid_pgm.max_value = 255;
    grid_pgm.pixels = calloc(tree->size * tree->size, sizeof(uint8_t));

    if (!grid_pgm.pixels)
    {
        return QTREE_ERROR_MEMORY;
    }

    // Draw grid lines recursively
    draw_node_grid(grid_pgm.pixels, tree->size, tree->root,
                   0, 0, tree->size);

    // Draw outer border
    draw_horizontal_line(grid_pgm.pixels, tree->size, 0, 0, tree->size);
    draw_horizontal_line(grid_pgm.pixels, tree->size, 0, tree->size - 1, tree->size);
    draw_vertical_line(grid_pgm.pixels, tree->size, 0, 0, tree->size);
    draw_vertical_line(grid_pgm.pixels, tree->size, tree->size - 1, 0, tree->size);

    // Write PGM file
    pgm_status_t status = pgm_write(&grid_pgm, output_file);
    free(grid_pgm.pixels);

    return (status == PGM_SUCCESS) ? QTREE_SUCCESS : QTREE_ERROR_FORMAT;
}