/**
 * @file segmentation_grid.h
 * @brief Makes a visualization of how our tree divides the image
 */

#ifndef SEGMENTATION_GRID_H
#define SEGMENTATION_GRID_H

#include "core/quadtree.h"
#include "io/pgm.h"

/**
 * @brief Creates an image showing how the quadtree divides regions
 * @param tree The tree to visualize
 * @param output_file Where to save the grid image
 * @return QTREE_SUCCESS if it worked
 */
qtree_status_t qtree_generate_grid(const qtree_t *tree, const char *output_file);

#endif /* SEGMENTATION_GRID_H */