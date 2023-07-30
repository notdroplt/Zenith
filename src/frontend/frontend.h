/**
 * @file frontend.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief define APIs for various input/output formats
 * @version 0.0.1
 * @date 2023-06-30
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#ifndef ZENITH_FRONTEND_H
#define ZENITH_FRONTEND_H

#include <types/types.h>
#include <string.h>
#include <stdio.h>

#define FRONTEND_INPUT "input"
#define FRONTEND_OUPTUT "output"
#define FRONTEND_VERBOSITY "verbose"
#define FRONTEND_OUTPUT_TYPE "output-type"


#include <platform.h>

/**
 * @defgroup frontend_apis
 * 
 * @brief apis written here accept a buffer instead of a file name because some of
 * them might not be used on normal files, but instead on sockets or pipes
 * @{
*/

enum json_types {
    json_unknown,
    json_bool,
    json_number_int,
    json_number_float, 
    json_string,
    json_array,
    json_object
};


/**
 * @brief loads a config in a ini format
 * 
 * 
 * values that are loaded in the form
 * 
 * [section]
 * key = value
 * 
 * can be achieved in the command line via
 * 
 * --section.key=value
 * 
 * ====
 * 
 * true/yes/enable/on   evaluate to 1
 * false/no/disable/off evaulate to 0
 * 
 * @param [inout] config table 
 * @param [inout] buffer buffer to load config from
 */
struct HashMap * load_conf(struct string_t buffer);

struct HashMap * load_json(struct string_t buffer);

struct HashMap * load_toml(struct string_t buffer);

struct HashMap * parse_commandline(int argc, char ** argv);

struct string_t dump_json(struct HashMap * map);

/** @} */

#endif
