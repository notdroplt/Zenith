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
#define ZENITH_PLATFORM_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief a struct that forms a "cursor" for the lexer
 *
 * Size: 16 bytes
 */
struct pos_t
{
    uint32_t index;         /*!< current character count */
    uint32_t last_line_pos; /*!< index of the last newline character */
    uint32_t line;          /*!< line count */
    uint32_t column;        /*!< column count */
};

#ifdef __cplusplus
#define extc extern "C"
#define extc_init \
    extern "C"    \
    {
#define extc_end }
#include <string_view>
#include <string>
#include <expected>
#include <type_traits>
class ZenithError
{

    std::string _err;
    std::string _desc;

    struct pos_t _start;
    struct pos_t _end;

public:
    bool hasColorSupport = false;

    ZenithError(const std::string &error, const std::string &description, pos_t start = pos_t(), pos_t end = pos_t()) : _err(error), _desc(description), _start(start), _end(end) {}

    constexpr std::string to_string(const std::string &filename)
    {
        return filename + ": " + this->_err + " error: " + _desc;
    }
};

template<typename T>
using returns = std::expected<T, ZenithError>;


template <typename Ex, typename... Args>
inline constexpr Ex make_unexpected(Args &&...args)
{
    using E = typename Ex::error_type;
    return Ex(std::unexpect, E(std::forward<Args>(args)...));
}

template <typename Ex, typename E>
inline constexpr Ex make_unexpected(const E & error)
{
    static_assert(std::is_same_v<Ex::error_type, E>, "error types diverge");
    return Ex(std::unexpect, error);
}

template <typename Ex, typename Ty = typename Ex::value_type, typename... Args>
inline constexpr Ex make_expected(Args &&...args) {
    using T = typename Ex::value_type;
    return Ex(T(Ty(std::forward<Args>(args)...)));
}

template <typename Ex, typename Ty, typename... Args>
inline constexpr Ex make_expected_unique(Args &&...args) {
    using T = typename Ex::value_type;
    return Ex(T(new Ty(std::forward<Args>(args)...)));
}

#else
#define extc
#define extc_init
#define extc_end
#endif

#ifdef _WIN32
#define HOSTNAME "Windows"
#elif __APPLE__
#define HOSTNAME "Macintosh"
#elif __linux__
#define HOSTNAME "Linux"
#elif __ANDROID__
#define HOSTNAME "Android"
#elif __unix__
#define HOSTNAME "Unix"
#else
#define HOSTNAME "Unknown"
#endif

#if CHAR_BIT * 8 == UINTPTR_WIDTH
#define BUS_SIZE 64
#elif CHAR_BIT * 4 == UINTPTR_WIDTH
#define BUS_SIZE 32
#elif CHAR_BIT * 2 == UINTPTR_WIDTH
#define BUS_SIZE 16
#endif

#if defined(__linux__) || defined(__APPLE__) || defined(doxygen)
#define Color_Red "\x1b[31m"     /*!< ANSI code for Red */
#define Color_Green "\x1b[32m"   /*!< ANSI code for Green */
#define Color_Yellow "\x1b[33m"  /*!< ANSI code for Yellow */
#define Color_Blue "\x1b[34m"    /*!< ANSI code for Blue */
#define Color_Magenta "\x1b[35m" /*!< ANSI code for Magenta */
#define Color_Cyan "\x1b[36m"    /*!< ANSI code for Cyan */
#define Color_Bold "\x1b[1m"     /*!< ANSI code for Bold */
#define Color_Reset "\x1b[0m"    /*!< ANSI code for reseting terminal color */
#else                            /* i am sorry windows users but no colors for you */
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
#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#define xstr(s) str(s)
#define str(s) #s
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
    uintmax_t size; /*!< string size */
    char *string;   /*!< string pointer */
#ifdef __cplusplus
    constexpr string_t(uintmax_t _size, char *_str) : size(_size), string(_str)
    {
    }
    constexpr string_t(std::string_view str) : size(str.size()), string((char *)str.data()){};
    string_t() = default;
    operator std::string_view() { return std::string_view(string, size); }
#endif
};

/*!< defines basic error code printing */
#define ZENITH_PRINT_ERR(fmt, ...) (void)fprintf(stderr, "Zenith Error: Z%04X" fmt "\n", __VA_ARGS__)

/*!< error when any part of the code goes out of memory */
#define ZenithOutOfMemory ZENITH_PRINT_ERR("", 0000);

/*!< error when a file is not found by any means*/
#define ZenithFileNotFound(filename) ZENITH_PRINT_ERR(":\"%s\"", 0001, filename)

#endif
