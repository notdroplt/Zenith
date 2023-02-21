#pragma once
#ifndef ZENITH_DOTENV_H
#define ZENITH_DOTENV_H 1
#include <platform.h>

/**
 * @brief enviromental variable for the input file
 * 
 */
#define env_input "ZNH_INPUT"

/**
 * @brief enviromental variable for the output file
 * 
 */
#define env_output "ZNH_OUTPUT"

/**
 * @brief enviromental variable for the debugger 
 * 
 */
#define env_debug "ZNH_DEBUG"

/**
 * @brief enviromental variable to print verbose output to the terminal
 * 
 * @note not implemented
 * 
 */
#define env_verbose "ZNH_VERBOSE"

/**
 * @brief compile to a virtual machine binary
 * 
 */
#define env_compile_virtmac "ZNH_COMPILE_VIRTMAC"

/**
 * @brief compile to a simple Intel Hex format
 * 
 */
#define env_compile_ihex "ZNH_COMPILE_IHEX"

/**
 * @brief dump the syntax tree as a JSON
 * 
 */
#define env_dump_json "ZNH_DUMP_JSON"

/**
 * @brief print out the diassembled file
 * 
 */
#define env_print_disassemble "ZNH_PRINT_DISASSEMBLE"


/**
 * @brief loads a .env file
 * 
 * @return int 
 */
 int load_dotenv();

/**
 * @brief defines a type for the verbose printf function
 * 
 * when it `env_verbose` is set, this function variable points to
 * a function that calls fprintf(stderr)
 */
typedef int (*verbose_printf)(const char * format_string, ...);

/**
 * @brief variable that points to the deriesd verbose function
 * 
 */
extern verbose_printf vrprintf;


#endif