#pragma once

#include "view.h"
#include "lexer_plugin.h"
#include "arena_allocator.h"

/**
 * @brief config headers recognized
 * */
enum config_headers {
	header_core, /*!< header defines core setting, which can be read by plugins */
	header_plugin, /*!< header to plugin specific data, which actually increases for every plugin added, making core as plugin#0 */
};

enum config_keys {
	config_core_warn_level = 0,
	config_core_debug,
	config_core_name_mangle,
	config_core_target,
	config_plugin_name = 0,
	config_plugin_trigger,
	config_plugin_path,
};


/**
 * @brief tags for parsed values
 */
enum config_values {
	config_decimal, /*!< value is a positive decimal value */
	config_integer, /*!< value is a positive integer value */
	config_string, /*!< value is a string */
};

/**
 * @brief config file possible value
 * of course, always trying to avoid strings, as they consume a considerable amount of memory compared to floats and integers
 */
union config_value_t {
	long double decimal; /*!< float value */
	uint64_t integer; /*!< integer value */
	string_view str; /*!< string value */
};

/**
 * @brief config option value
 */
struct config_option_t {
	union config_value_t value;
	enum config_headers header;
	enum config_keys key;
	enum config_values tag;
};

/**
 * @brief read a config file instead of command line arguments
 *
 * @param[in] filename name of the config file
 * @param[in] file_content allocator for contents
 * @param[in] scratch scratch allocator, does not need to be as big as the file content
 * @param[out] size size of configs read
 *
 * scratch is cleared but not freed by the function after returning
 */
struct config_option_t * read_config_file(char * filename, struct arena_allocator * file_content, struct arena_allocator * scratch, int * size);

