/**
 * @file pgm.h
 * @brief Functions to handle PGM image files
 */

#ifndef PGM_H
#define PGM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Different things that can happen when working with images
 */
typedef enum
{
    PGM_SUCCESS = 0,  /* Everything worked */
    PGM_ERROR_FILE,   /* Problems with files */
    PGM_ERROR_FORMAT, /* Bad file format */
    PGM_ERROR_MEMORY, /* Not enough memory */
    PGM_ERROR_SIZE,   /* Image size isn't right */
    PGM_ERROR_PARAM,  /* Bad parameters */
} pgm_status_t;

/**
 * @brief How we store an image in memory
 *
 * Rules for images:
 * - Must be square (width = height)
 * - Size must be power of 2 (like 256, 512...)
 * - Uses 8 bits per pixel (grayscale)
 *
 * Remember to use pgm_free() when done!
 */
typedef struct
{
    uint8_t *pixels;   /* The actual image data */
    uint32_t size;     /* Width/height of image */
    uint8_t max_value; /* Brightest possible pixel */
} pgm_t;

/**
 * @brief Loads an image from a file
 * @param path Which file to load
 * @param pgm Where to store the image
 * @return PGM_SUCCESS if loading worked
 */
pgm_status_t pgm_read(const char *path, pgm_t *pgm);

/**
 * @brief Saves an image to a file
 * @param pgm The image to save
 * @param path Where to save it
 * @return PGM_SUCCESS if saving worked
 */
pgm_status_t pgm_write(const pgm_t *pgm, const char *path);

/**
 * @brief Cleans up image memory
 * @param pgm The image to clean up
 */
void pgm_free(pgm_t *pgm);

#endif /* PGM_H */