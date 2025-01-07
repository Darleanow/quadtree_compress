/**
 * @file codec.h
 * @brief Main header for our image compression program
 */

#ifndef CODEC_H
#define CODEC_H

#include "config/config.h"

/**
 * @brief Different status codes for our functions
 */
typedef enum
{
    CODEC_SUCCESS = 0,         /* Everything worked fine */
    CODEC_ERROR_INVALID_PARAM, /* Something wrong with the parameters */
    CODEC_ERROR_FILE_IO,       /* Problem reading/writing files */
    CODEC_ERROR_MEMORY,        /* Not enough memory */
    CODEC_ERROR_FORMAT         /* File format problems */
} codec_status_t;

/**
 * @brief Takes an image and compresses it
 * @param config All the settings we need
 * @return How it went (success or what kind of error)
 */
codec_status_t codec_compress(const config_t *config);

/**
 * @brief Takes a compressed file and turns it back into an image
 * @param config All the settings we need
 * @return How it went (success or what kind of error)
 */
codec_status_t codec_decompress(const config_t *config);

/**
 * @brief Converts error codes into readable messages
 * @param status The status code to convert
 * @return The message explaining what happened
 */
const char *codec_status_string(codec_status_t status);

#endif /* CODEC_H */