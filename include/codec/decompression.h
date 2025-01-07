/**
 * @file decompression.h
 * @brief Functions to decompress our quadtree images
 */

#ifndef QUADTREE_DECOMPRESS_H
#define QUADTREE_DECOMPRESS_H

#include "core/quadtree.h"
#include "io/pgm.h"
#include <stdio.h>

/**
 * @brief Reads a compressed file and rebuilds the quadtree
 * @param file The compressed file to read from
 * @param input_filename Path to the file
 * @param tree Where to store the rebuilt tree
 * @return QTREE_SUCCESS if it worked
 */
qtree_status_t qtree_decompress(FILE *file, const char *input_filename, qtree_t *tree);

/**
 * @brief Converts our quadtree back into a normal image
 * @param tree The quadtree to convert
 * @param output_filename Where to save the image
 * @param pgm Where to store the image data
 * @return QTREE_SUCCESS if it worked
 */
qtree_status_t qtree_to_pgm(const qtree_t *tree, const char *output_filename, pgm_t *pgm);

#endif /* QUADTREE_DECOMPRESS_H */