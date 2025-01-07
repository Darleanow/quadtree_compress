/**
 * @file pgm.c
 * @brief Implementation of PGM image handling functions
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "io/pgm.h"
#include "common/common.h"

#define MAGIC_NUMBER "P5" /**< Binary PGM format identifier */
#define MAX_LINE_LEN 256  /**< Maximum length of header lines */

/**
 * @brief Skip whitespace and comments in PGM file
 *
 * Advances file pointer past:
 * - Whitespace (space, tab, newline)
 * - Comments (# to end of line)
 *
 * @param[in] file Open file handle
 * @return 0 on success, -1 on EOF or error
 */
static int skip_ws_and_comments(FILE *file)
{
    int c;

    while ((c = fgetc(file)) != EOF)
    {
        if (isspace(c))
        {
            continue;
        }
        if (c == '#')
        {
            while ((c = fgetc(file)) != EOF && c != '\n')
                ;
            if (c == EOF)
                return -1;
        }
        else
        {
            ungetc(c, file);
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Read and validate header of PGM file
 *
 * Parses and validates:
 * - Magic number (P5)
 * - Image dimensions
 * - Maximum value
 *
 * @param[in]  file Open file handle
 * @param[out] pgm  Structure to populate with header info
 * @return PGM_SUCCESS on success, error code otherwise
 */
static pgm_status_t read_header(FILE *file, pgm_t *pgm)
{
    char magic[3] = {0};
    unsigned int width, height, max_val;

    // Check magic number
    if (fgets(magic, sizeof(magic), file) == NULL ||
        strncmp(magic, MAGIC_NUMBER, 2) != 0)
    {
        return PGM_ERROR_FORMAT;
    }

    // Skip any comments/whitespace
    if (skip_ws_and_comments(file) < 0)
    {
        return PGM_ERROR_FORMAT;
    }

    // Read dimensions
    if (fscanf(file, "%u %u", &width, &height) != 2)
    {
        return PGM_ERROR_FORMAT;
    }

    // Validate dimensions
    if (width != height || !is_power_of_two(width))
    {
        return PGM_ERROR_SIZE;
    }
    pgm->size = width;

    // Skip to max value
    if (skip_ws_and_comments(file) < 0)
    {
        return PGM_ERROR_FORMAT;
    }

    // Read and validate max value
    if (fscanf(file, "%u", &max_val) != 1 || max_val > 255)
    {
        return PGM_ERROR_FORMAT;
    }
    pgm->max_value = (uint8_t)max_val;

    // Must have exactly one whitespace character after max value
    int c = fgetc(file);
    if (c == EOF || !isspace(c))
    {
        return PGM_ERROR_FORMAT;
    }

    return PGM_SUCCESS;
}

pgm_status_t pgm_read(const char *path, pgm_t *pgm)
{
    if (!path || !pgm)
    {
        return PGM_ERROR_FORMAT;
    }

    FILE *file = fopen(path, "rb");
    if (!file)
    {
        return PGM_ERROR_FILE;
    }

    // Initialize structure
    memset(pgm, 0, sizeof(pgm_t));

    // Read and validate header
    pgm_status_t status = read_header(file, pgm);
    if (status != PGM_SUCCESS)
    {
        fclose(file);
        return status;
    }

    // Allocate pixel buffer
    size_t pixel_count = (size_t)pgm->size * pgm->size;
    pgm->pixels = malloc(pixel_count);
    if (!pgm->pixels)
    {
        fclose(file);
        return PGM_ERROR_MEMORY;
    }

    // Read pixel data
    if (fread(pgm->pixels, 1, pixel_count, file) != pixel_count)
    {
        pgm_free(pgm);
        fclose(file);
        return PGM_ERROR_FORMAT;
    }

    fclose(file);
    return PGM_SUCCESS;
}

pgm_status_t pgm_write(const pgm_t *pgm, const char *path)
{
    if (!pgm || !pgm->pixels || !path)
    {
        return PGM_ERROR_FORMAT;
    }

    FILE *file = fopen(path, "wb");
    if (!file)
    {
        return PGM_ERROR_FILE;
    }

    // Write header
    if (fprintf(file, "%s\n%u %u\n%u\n",
                MAGIC_NUMBER, pgm->size, pgm->size, pgm->max_value) < 0)
    {
        fclose(file);
        return PGM_ERROR_FILE;
    }

    // Write pixel data
    size_t pixel_count = (size_t)pgm->size * pgm->size;
    if (fwrite(pgm->pixels, 1, pixel_count, file) != pixel_count)
    {
        fclose(file);
        return PGM_ERROR_FILE;
    }

    fclose(file);
    return PGM_SUCCESS;
}

void pgm_free(pgm_t *pgm)
{
    if (pgm)
    {
        free(pgm->pixels);
        pgm->pixels = NULL;
        pgm->size = 0;
        pgm->max_value = 0;
    }
}