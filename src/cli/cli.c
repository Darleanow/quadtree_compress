/**
 * @file cli.c
 * @brief Command line interface handling implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cli/cli.h"
#include "config/config.h"

void cli_print_help(void)
{
    printf("Usage: codec [options]\n"
           "Options:\n"
           "  -c              Compress the input PGM file\n"
           "  -u              Decompress the input QTC file\n"
           "  -i <input>      Input file path\n"
           "  -o <output>     Output file path\n"
           "  -g              Generate segmentation grid\n"
           "  -a <alpha>      Compression parameter (default: 1.0)\n"
           "  -h              Display this help\n"
           "  -v              Enable verbose mode\n");
}

static bool handle_option_with_argument(const char opt, const char *arg,
                                        int *i, int argc, char **argv,
                                        config_t *config)
{
    if (++(*i) >= argc)
    {
        fprintf(stderr, "Error: Missing argument for -%c\n", opt);
        return false;
    }

    switch (opt)
    {
    case 'i':
        config->input_file = argv[*i];
        break;
    case 'o':
        config->output_file = argv[*i];
        break;
    case 'a':
    {
        float alpha = (float)atof(argv[*i]);
        if (alpha <= 0)
        {
            fprintf(stderr, "Error: Invalid alpha value\n");
            return false;
        }
        config->alpha = alpha;
        break;
    }
    case 'g':
        config->grid_file = argv[*i];
        config->generate_grid = true;
        break;
    default:
        fprintf(stderr, "Error: Invalid option '-%c'\n", opt);
        return false;
    }

    return true;
}

static bool handle_flag_option(const char opt, config_t *config)
{
    switch (opt)
    {
    case 'c':
        config->compress = true;
        break;
    case 'u':
        config->decompress = true;
        break;
    case 'v':
        config->verbose = true;
        break;
    case 'h':
        cli_print_help();
        exit(EXIT_SUCCESS);
    default:
        fprintf(stderr, "Error: Unknown option '-%c'\n", opt);
        return false;
    }
    return true;
}

static bool validate_config(config_t *config)
{
    if (config->compress && config->decompress)
    {
        fprintf(stderr, "Error: Cannot specify both compression and decompression\n");
        return false;
    }

    if (!config->compress && !config->decompress)
    {
        fprintf(stderr, "Error: Must specify either compression or decompression\n");
        return false;
    }

    if (!config->input_file)
    {
        fprintf(stderr, "Error: Input file not specified\n");
        return false;
    }

    // Set default output file if not specified
    if (!config->output_file)
    {
        config->output_file = config->compress ? DEFAULT_COMPRESS_OUTPUT : DEFAULT_DECOMPRESS_OUTPUT;
    }

    return true;
}

bool cli_parse_arguments(int argc, char **argv, config_t *config)
{
    // Initialize config with defaults
    config_init(config);

    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];

        if (arg[0] != '-' || arg[1] == '\0')
        {
            fprintf(stderr, "Error: Invalid argument format: %s\n", arg);
            return false;
        }

        // Handle options that require additional arguments
        if (strchr("ioga", arg[1]))
        {
            if (!handle_option_with_argument(arg[1], arg, &i, argc, argv, config))
            {
                return false;
            }
        }
        // Handle flag options
        else if (!handle_flag_option(arg[1], config))
        {
            return false;
        }
    }

    return validate_config(config);
}