#ifndef ZENITH_PLATFORM_H
#define ZENITH_PLATFORM_H 1

#ifdef _WIN32
#ifdef shared_EXPORTS
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT __declspec(dllimport)
#endif
#else
#define LIB_EXPORT
#endif

#if defined(__linux__) || defined(__APPLE__)
#define Color_Red "\x1b[31m"
#define Color_Green "\x1b[32m"
#define Color_Yellow "\x1b[33m"
#define Color_Blue "\x1b[34m"
#define Color_Magenta "\x1b[35m"
#define Color_Cyan "\x1b[36m"
#define Color_Bold "\x1b[1m"
#define Color_Reset "\x1b[0m"
#else // i am sorry windows users but no colors for you
#define Color_Red ""
#define Color_Green ""
#define Color_Yellow ""
#define Color_Blue ""
#define Color_Magenta ""
#define Color_Cyan ""
#define Color_Bold ""
#define Color_Reset ""
#endif

#ifdef __cplusplus
/**
 * @brief defines C linkage
 *
 */
#define extc extern "C"
#   include <cstdint>
#else
/**
 * @brief defines C linkage
 *
 */
#define extc
#   include <stdint.h>
#endif

#endif