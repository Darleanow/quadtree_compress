/**
 * @file compression.h
 * @brief Header for the compression part of our quadtree codec
 */

#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <stdint.h>
#include <stdio.h>
#include "core/quadtree.h"

/**
 * @brief Structure to keep track of compression progress
 */
typedef struct {
    uint8_t buffer;          /* Stores bits before writing them */
    size_t bit_position;     /* Which bit we're currently at */
    FILE *file;             /* Where we write the compressed data */
    size_t bytes_written;    /* How many bytes we've written */
    size_t total_bits;       /* Total bits processed */
    int error;              /* Tracks if something went wrong */
    size_t total_nodes;     /* Total nodes to process */
    size_t processed_nodes;  /* How many we've done so far */
} qtree_compress_state_t;

/**
 * @brief Sets up everything needed for compression
 * @param file Where to write the compressed data
 * @return The initialized state
 */
qtree_compress_state_t compress_init(FILE *file);

/**
 * @brief Writes one bit to the file
 * @param state Current compression state
 * @param bit The bit to write (0 or 1)
 */
void compress_write_bit(qtree_compress_state_t *state, uint8_t bit);

/**
 * @brief Writes multiple bits at once
 * @param state Current compression state
 * @param value The bits to write
 * @param num_bits How many bits to write (max 32)
 */
void compress_write_bits(qtree_compress_state_t *state, uint32_t value, size_t num_bits);

/**
 * @brief Writes any remaining bits and cleans up
 * @param state Current compression state
 */
void compress_flush(qtree_compress_state_t *state);

/**
 * @brief Checks how well we compressed the data
 * @param total_bits Size after compression
 * @param original_size Size before compression
 * @return Compression rate in percentage
 */
float compress_get_rate(size_t total_bits, size_t original_size);

/**
 * @brief Main function to compress a quadtree
 * @param tree The tree to compress
 * @param output_filename Path for the output file
 * @param output_file File already opened for writing
 * @return QTREE_SUCCESS if everything went well
 */
qtree_status_t compress(const qtree_t *tree, const char *output_filename, FILE *output_file);

/**
 * @brief Makes compression better by filtering small differences
 * @param tree The tree to filter
 * @param alpha How aggressive the filtering should be
 * @return QTREE_SUCCESS if filtering worked
 */
qtree_status_t apply_lossy_compression(qtree_t *tree, float alpha);

#endif /* COMPRESSION_H */