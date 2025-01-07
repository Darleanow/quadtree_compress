/**
 * @file config.c
 * @brief Program configuration implementation
 */

#include <string.h>
#include "config/config.h"

void config_init(config_t *config)
{
    memset(config, 0, sizeof(config_t));
    config->alpha = DEFAULT_ALPHA;
}