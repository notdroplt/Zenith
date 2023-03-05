/**
 * @file platform.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief defines platform specific
 * @version 
 * @date 2023-02-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once
#ifndef ZENITH_PLATFORM_H
#	define ZENITH_PLATFORM_H 1
#include <stdint.h>
#include <stdatomic.h>
#if defined(__linux__) || defined(__APPLE__) || defined(doxygen)
#	define Color_Red "\x1b[31m" /*!< ANSI code for Red */
#	define Color_Green "\x1b[32m" /*!< ANSI code for Green */
#define Color_Yellow "\x1b[33m" /*!< ANSI code for Yellow */
#define Color_Blue "\x1b[34m" /*!< ANSI code for Blue */
#define Color_Magenta "\x1b[35m" /*!< ANSI code for Magenta */
#define Color_Cyan "\x1b[36m" /*!< ANSI code for Cyan */
#define Color_Bold "\x1b[1m" /*!< ANSI code for Bold */
#define Color_Reset "\x1b[0m" /*!< ANSI code for reseting terminal color */
#else /* i am sorry windows users but no colors for you */
#define Color_Red ""
#define Color_Green ""
#define Color_Yellow ""
#define Color_Blue ""
#define Color_Magenta ""
#define Color_Cyan ""
#define Color_Bold ""
#define Color_Reset ""
#endif

/* this is just for macro concatenation, its nothing too much */

#ifndef doxygen
#	define CONCAT_IMPL(x, y) x##y 
#	define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#	define xstr(s) str(s)
#	define str(s) #s
#endif

/*!< compiler enviroment major version */
#define platform_ver_maj 1

/*!< compiler enviroment minor version */
#define platform_ver_min 0

/*!< compiler enviroment revision version */
#define platform_ver_rev 0

/*!< full version, as a string */
#define platform_ver_str xstr(MACRO_CONCAT(platform_ver_maj, MACRO_CONCAT(., MACRO_CONCAT(platform_ver_min, MACRO_CONCAT(., platform_ver_rev)))))

/**
 * @brief a c string_view object
 *
 * Size : 16 bytes
 */
struct string_t
{
	uintmax_t size;  /*!< string size */
	char *string; /*!< string pointer */
};

#include "types.h"


/*!< defines basic error code printing */
#define ZENITH_PRINT_ERR(fmt, ...) fprintf(stderr, "Zenith Error: %s" fmt "\n", __VA_ARGS__)

/*!< error when any part of the code goes out of memory */
#define ZenithOutOfMemory ZENITH_PRINT_ERR("", "Z0000");

/*!< error when a file is not found by any means*/
#define ZenithFileNotFound(filename) ZENITH_PRINT_ERR(":\"%s\"", "Z0001", filename)

#endif
