/**
 * @file platform.hpp
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
//!@cond
#define ZENITH_PLATFORM_H 1
//!@endcond
#include <cstdint>
#include <string_view>
#include <string>
#include <expected>
#include <type_traits>
#include <fstream>
#include <cmath>
#include <iostream>
#include <functional>
#include <variant>

/**
 * @brief a struct that forms a "cursor" for the lexer
 *
 * Size: 16 bytes
 */
struct pos_t
{
    //! current character count
    uint32_t index;

    //! index of the last newline character
    uint32_t last_line_pos;

    //! line count
    uint32_t line;

    //! column count
    uint32_t column;
};

/**
 * @brief a class that represents an error while on compilation
 *
 */
class ZenithError
{
    //! Error name
    std::string _err;

    //! Error description
    std::string _desc;

    //! Where the error started
    struct pos_t _start;

    //! where the error ended
    struct pos_t _end;

public:
    //! define whenever to use ANSI color codes
    bool hasColorSupport = false;

    /**
     * @brief Construct a new Zenith Error object
     *
     * @param error error name
     * @param description error description
     * @param start error start position
     * @param end error end position
     */
    ZenithError(const std::string &error, const std::string &description, pos_t start = pos_t(), pos_t end = pos_t()) : _err(error), _desc(description), _start(start), _end(end) {}

    /**
     * @brief error start position getter
     *
     * @return auto& start position
     */
    auto &start() { return this->_start; }

    /**
     * @brief error end position getter
     *
     * @return auto&
     */
    auto &end() { return this->_end; }

    /**
     * @brief transform error into a string
     *
     * @param filename name of the file to show the error
     * @return std::string error string
     */
    std::string to_string(const std::string &filename)
    {
        std::ifstream file;
        char c;
        file.open(filename);
        file.seekg(this->_start.last_line_pos);
        auto err = filename + ": " + this->_err + " error: " + _desc + '\n';
        err += ' ' + std::to_string(this->_start.line) + " | ";
        while ((c = file.get()) != '\n' && c != (char)EOF)
        {
            err += c;
        }

        err += "\n    " + std::string(std::floor(std::log10(this->_start.line)) - 1 + this->_start.column, ' ');

        int amount = this->_end.column - this->_start.column;

        if (amount < 0) {
            amount = this->_end.last_line_pos - this->_start.last_line_pos - this->_start.column + 1;
        }

        return err + std::string(amount, '^');
    }
};

/**
 * @brief define a return type which might have an error
 *
 * @tparam T expected return type
 */
template <typename T>
using returns = std::expected<T, ZenithError>;

/**
 * @brief construct an unexpected return type in place
 *
 * @tparam Ex std::expected complete type
 * @tparam ...Args types of parameters to Ex::error_type
 * @param ...args arguments to construct Ex::error_type
 * @return std::expected with an unexpected value
 */
template <typename Ex, typename... Args>
inline constexpr Ex make_unexpected(Args &&...args)
{
    using E = typename Ex::error_type;
    return Ex(std::unexpect, E(std::forward<Args>(args)...));
}

/**
 * @brief construct an unexpected value from an error
 *
 * @tparam Ex std::expected type
 * @param error constructed error
 * @return std::expected with an unexpected value
 */
template <typename Ex>
inline constexpr Ex make_unexpected(typename Ex::error_type &error)
{
    return Ex(std::unexpect, error);
}

/**
 * @brief construct an unique variant with one of the variant's members constructor
 *
 * @tparam V full variant type
 * @tparam T type to construct
 * @tparam ...Args type deducted constructor arguments
 * @param ...args constructor arguments
 * @return unique pointer to variant type
 */
template <typename V, typename T, typename... Args>
inline constexpr V make_unique_variant(Args &&...args)
{
    using Tx = typename V::element_type;
    return V(new Tx(T(std::forward<Args>(args)...)));
}

/**
 * @brief generate an expected value from an unique pointer's derived class constructor
 *
 * @tparam Ex full base expected value
 * @tparam Ty derived type
 * @tparam ...Args type deducted constructor arguments
 * @param ...args constructor arguments
 * @return the expected base class with a derived unique pointer value
 */
template <typename Ex, typename Ty, typename... Args>
inline constexpr Ex make_expected_unique(Args &&...args)
{
    using T = typename Ex::value_type;
    return Ex(T(new Ty(std::forward<Args>(args)...)));
}

/**
 * @brief returns the error of two monads, where either of them is guaranteed to have failed
 *
 * @tparam T expected type of the monad
 * @tparam E error type of the monad
 * @param monad1 first monad to check
 * @param monad2 second monad to check
 * @return constexpr std::expected<T, E> error
 */
template <typename T, typename E>
constexpr std::expected<T, E> monad_bind_unexpected_return(std::expected<T, E> &&monad1, std::expected<T, E> &&monad2)
{
    // one of the two values has an error
    if (!monad1)
        return std::move(monad1);
    return std::move(monad2);
}

/**
 * @brief returns the error of multiple monads, where at least one of them has failed
 *
 * @tparam Tx type of the result monad
 * @tparam T expected type of all other monads
 * @tparam E error type
 * @param monad1 first monad
 * @param monads other monads
 * @return constexpr std::expected<Tx, E> error thrown by one of them
 */
template <typename Tx, typename... T, typename E>
inline constexpr std::expected<Tx, E> monad_bind_unexpected_return(std::expected<Tx, E> &&monad1, std::expected<T, E> &&...monads)
{
    if (!monad1)
        return std::move(monad1);
    return monad_bind_unexpected_return(std::move(monads)...);
}

/**
 * @brief mind multiple monads with a binding function
 *
 * @tparam F type deducted of the function
 * @tparam T1 type deducted first monad
 * @tparam T type deducted rest of the monads
 * @param func binder function
 * @param first first monad
 * @param args other monads
 * @return constexpr T1 return monad
 */
template <typename F, typename T1, typename... T>
inline constexpr T1 monad_bind(F &&func, T1 &&first, T &&...args)
{
    if (!first || !(... || args))
        return monad_bind_unexpected_return(std::move(first), std::move(args)...);
    return std::invoke(std::forward<F>(func), std::move(*first), std::move(*args)...);
}

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
#elif doxygen
//!@cond
#define HOSTNAME "doxygen"
//!@endcond
#else
#define HOSTNAME "Unknown"
#endif

#if defined(__linux__) || defined(__APPLE__) || defined(doxygen)
//! ANSI code for Red */
#define Color_Red "\x1b[31m"

//! ANSI code for Green */
#define Color_Green "\x1b[32m"

//! ANSI code for Yellow */
#define Color_Yellow "\x1b[33m"

//! ANSI code for Blue */
#define Color_Blue "\x1b[34m"

//! ANSI code for Magenta */
#define Color_Magenta "\x1b[35m"

//! ANSI code for Cyan */
#define Color_Cyan "\x1b[36m"

//! ANSI code for Bold */
#define Color_Bold "\x1b[1m"

//! ANSI code for reseting terminal color */
#define Color_Reset "\x1b[0m"
#else
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

//!@cond
#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#define xstr(s) str(s)
#define str(s) #s
//!@endcond

//! compiler enviroment major version
#define platform_ver_maj 1

//! compiler enviroment minor version
#define platform_ver_min 0

//! compiler enviroment revision version
#define platform_ver_rev 0

//! full version, as a string
#define platform_ver_str xstr(MACRO_CONCAT(platform_ver_maj, MACRO_CONCAT(., MACRO_CONCAT(platform_ver_min, MACRO_CONCAT(., platform_ver_rev)))))

//! defines basic error code printing
#define ZENITH_PRINT_ERR(fmt, ...) (void)fprintf(stderr, "Zenith Error: Z%04X" fmt "\n", __VA_ARGS__)

//! error when any part of the code goes out of memory
#define ZenithOutOfMemory ZENITH_PRINT_ERR("", 0000);

//! error when a file is not found by any means
#define ZenithFileNotFound(filename) ZENITH_PRINT_ERR(":\"%s\"", 0001, filename)

#endif
