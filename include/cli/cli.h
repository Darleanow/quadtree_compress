/**
 * @file cli.h
 * @brief Handles command line stuff for our program
 */

#ifndef CLI_H
#define CLI_H

#include "config/config.h"

/**
 * @brief Processes the arguments passed to the program
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @param config Where to store the settings
 * @return true if arguments are valid, false if not
 */
bool cli_parse_arguments(int argc, char **argv, config_t *config);

/**
 * @brief Shows how to use the program
 */
void cli_print_help(void);

#endif /* CLI_H */