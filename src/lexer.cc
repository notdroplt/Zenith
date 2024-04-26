#include "lexer.hh"
#include <array>
#include <cctype>
#include <cmath>

#ifdef ZENITH_LEXER_DEBUG
#include <iostream>
#endif
namespace
{
    enum prefixes
    {
        int_decimal = 10,
        int_binary = 2,
        int_octal = 8,
        int_hex = 16
    };

    using Lexer = zenith::lexer::Lexer;
    using Error = zenith::errors::Errors;
    using lexer_error = std::unexpected<Error>;
    using Token = zenith::lexer::Token;
    using lexer_return = std::expected<Token, Error>;

    template <typename T, bool use_opt = true>
    using return_wrapper = std::pair<Lexer, std::conditional_t<use_opt, std::optional<T>, T>>;

    
    ZENITH_CONST_NODISCARD_CONSTEXPR Lexer nextc(const Lexer lexer, const std::string_view str) noexcept
    {
        const auto [index, line, column] = lexer.m_pos;

#if defined(ZENITH_LEXER_DEBUG) && ZENITH_LEXER_DEBUG != 0
        std::clog << "Lexer log: {index " << index << ", column " << column << ", line " << line << "} char = '";
        if (str[index] == '\n') {
            std::clog << "\\n";
        } else if (str[index] == '\t') {
            std::clog << "\\t";
        } else {
            std::clog << str[index];
        }
        std::clog << "'\n";
#endif
        
        if (str.size() <= index + 1) [[unlikely]]
        {
            return Lexer{{index, line, column}, '\0'};
        }

        const auto chr = str[index + 1];


        if (chr == '\n') [[unlikely]]
        {
            return Lexer{{index + 1, static_cast<uint16_t>(line + 1), 1}, chr};
        }

        return Lexer{{index + 1, line, static_cast<uint16_t>(column + 1)}, chr};
    }

    ZENITH_CONST_NODISCARD_CONSTEXPR bool is_digit(const char numchr, const prefixes prefix) noexcept
    {
        const int chr = std::tolower(numchr);
        switch (prefix)
        {
        case int_binary:
            return chr == '0' || chr == '1';
        case int_octal:
            return chr >= '0' && chr <= '7';
        case int_decimal:
            return chr >= '0' && chr <= '9';
        case int_hex:
            return (chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f');
        }

        return false;
    }

    ZENITH_CONST_NODISCARD_CONSTEXPR return_wrapper<int> get_digit(
        Lexer lexer, const std::string_view str, const prefixes prefix) noexcept
    {
        while (lexer.m_char == '_')
        {
            lexer = nextc(lexer, str);
            if (lexer.m_char != '\0') [[unlikely]]
            {
                return std::make_pair(lexer, std::nullopt);
            }
        }

        constexpr int hex_a_offset = 10;
        switch (prefix)
        {
        case int_hex:
            if (std::isdigit(lexer.m_char) == 0)
            {
                return std::make_pair(lexer, std::tolower(lexer.m_char) - 'a' + hex_a_offset);
            }
            return std::make_pair(lexer, lexer.m_char - '0');
        case int_binary:
        case int_octal:
        case int_decimal:
            return std::make_pair(lexer, lexer.m_char - '0');
        }

        return std::make_pair(lexer, std::tolower(lexer.m_char) - 'a' + hex_a_offset);
    }

    template <typename T>
    ZENITH_CONST_NODISCARD_CONSTEXPR T ipow(const T base, const uint64_t exp) noexcept
    {
        T res = base;

        for (unsigned i = 0; i < exp - 1; ++i)
        {
            res *= base;
        }

        return res;
    }

    ZENITH_CONST_NODISCARD_CONSTEXPR std::optional<prefixes> get_prefix(char chr) noexcept
    {
        switch (chr)
        {
        case 'b':
            return int_binary;
        case 'x':
        case 'h':
            return int_hex;
        case 'o':
            return int_octal;
        case 'd':
            return int_decimal;
        default:
            return std::nullopt;
        }
    }

    ZENITH_CONST_NODISCARD_CONSTEXPR return_wrapper<int64_t> handle_integer_part(Lexer lexer, const std::string_view str, const prefixes prefix) noexcept
    {
        int64_t value = 0;
        int64_t multiplier = 1;
        while (is_digit(lexer.m_char, prefix))
        {
            auto [lex, val] = get_digit(lexer, str, prefix);
            if (!val) [[unlikely]]
            {
                return std::make_pair(lexer, std::nullopt);
            }
            lexer = nextc(lex, str);
            value += *val * multiplier;
            multiplier *= prefix;
        }

        return std::make_pair(lexer, value);
    }

    ZENITH_CONST_NODISCARD_CONSTEXPR return_wrapper<double> handle_fractional_part(Lexer lexer, const std::string_view str, const prefixes prefix) noexcept
    {
        double value = 0.0;
        double divider = -1.0;

        while (is_digit(lexer.m_char, prefix))
        {
            auto [lex, digit] = get_digit(lexer, str, prefix);
            lexer = lex;
            if (!digit) [[unlikely]]
            {
                return std::make_pair(lexer, std::nullopt);
            }
            value = *digit * pow(static_cast<double>(prefix), divider) + value;
            --divider;
        }

        return std::make_pair(lexer, value);
    }

    ZENITH_CONST_NODISCARD_CONSTEXPR return_wrapper<int64_t> handle_exponent_part(Lexer lexer, const std::string_view str, const int64_t signal) noexcept
    {
        int64_t exp = 0;
        int64_t multiplier = 1;

        while (is_digit(lexer.m_char, int_decimal))
        {
            auto [lex, val] = get_digit(lexer, str, int_decimal);
            lexer = lex;
            if (!val)
            {
                return std::make_pair(lexer, std::nullopt);
            }
            exp += static_cast<int64_t>(*val) * multiplier;
            multiplier *= int_decimal;
        }

        return std::make_pair(lexer, signal * exp);
    }

    ZENITH_CONST_NODISCARD_CONSTEXPR return_wrapper<lexer_return, false> default_num_reader(Lexer lexer, const std::string_view str) noexcept
    {
        enum prefixes prefix = int_decimal;
        bool is_int = true;
        Token value;

        if (lexer.m_char == '0')
        {
            lexer = nextc(lexer, str);
            if (lexer.m_char != '\0')
            {
                return std::make_pair(lexer, 0);
            }

            auto pref = get_prefix(lexer.m_char);
            if (!pref)
            {
                return std::make_pair(lexer, lexer_error(Error::UnknownNumberBase));
            }
            prefix = *pref;
        }

        auto [lex1, integer] = handle_integer_part(lexer, str, prefix);
        if (!integer) {
            return std::make_pair(lexer, lexer_error(Error::InvalidNumericChar));
        }

        lexer = lex1;
        value = *integer;
        
        if (lexer.m_char != '.' && std::tolower(lexer.m_char) != 'e') {
            return std::make_pair(lexer, value);
        }

        if (lexer.m_char == '.')
        {
            is_int = false;
            value = static_cast<double>(std::get<int64_t>(value));
            if (lexer.m_char == '.')
            {
                lexer = nextc(lexer, str);
                auto [lex2, decimal] = handle_fractional_part(lexer, str, prefix);
                lexer = lex2;
                if (!decimal) {
                    return std::make_pair(lexer, lexer_error(Error::DecimalAfterDot));
                }
                value = static_cast<double>(std::get<int64_t>(value)) + *decimal;
            }
        }

        if (std::tolower(lexer.m_char) == 'e')
        {
            lexer = nextc(lexer, str);
            auto signal = str.front() == '-' ? -1 : 1;
            if (signal == -1)
            {
                lexer = nextc(lexer, str);
            }
            auto [lex2, exp] = handle_exponent_part(lexer, str, signal);
            lexer = lex2;
            if (!exp)
            {
                return std::make_pair(lexer, lexer_error(Error::EmptyExponent));
            }
            if (signal >= 0 && is_int)
            {
                std::get<int64_t>(value) *= ipow(static_cast<int>(prefix), *exp);
            }
            else
            {
                std::get<double>(value) *= pow(static_cast<double>(prefix), *exp);
            }
        }

        return std::make_pair(lexer, value);
    }

    ZENITH_CONST_NODISCARD_CONSTEXPR return_wrapper<lexer_return, false> default_str_reader(Lexer lexer, const std::string_view str) noexcept
    {
        lexer = nextc(lexer, str);

        auto start = lexer.m_pos.m_index;

        while (lexer.m_char != '"') 
        {
            if (lexer.m_char == '\0') [[unlikely]]
            {
                return std::make_pair(lexer, lexer_error(Error::StringDidNotFinish));
            }
            lexer = nextc(lexer, str);
        }

        lexer = nextc(lexer, str);

        return std::make_pair(lexer, std::make_pair(str.substr(start, lexer.m_pos.m_index - 1), false));
    }

    ZENITH_CONST_NODISCARD_CONSTEXPR return_wrapper<Error, false> skip_comment(Lexer lexer, const std::string_view str) noexcept
    {
        if (str.substr(lexer.index()).starts_with("//"))
        {
            while (lexer.m_char != '\n')
            {
                lexer = nextc(lexer, str);
            }
            return std::make_pair(lexer, Error::CommentFinish);
        }

        // we do advance twice
        lexer = nextc(lexer, str); // this skips '/'
        lexer = nextc(lexer, str); // this skips '*'

        while (lexer.m_char != '\0') [[likely]]
        {
            if (str.substr(lexer.index()).starts_with("*/")) [[likely]]
            {
                lexer = nextc(lexer, str); // this skips '*'
                lexer = nextc(lexer, str); // this skips '/'
                break;
            }
            else [[unlikely]]
            {
                lexer = nextc(lexer, str);
            }

            if (str.size() <= 2) 
            {
                return std::make_pair(lexer, Error::CommentDidNotFinish);
            }
        }

        return std::make_pair(lexer, Error::CommentFinish);
    }
}; // namespace 

namespace zenith::lexer
{

    ZENITH_CONST_NODISCARD std::pair<Lexer, lexer_return> next_token(Lexer lexer, std::string_view content) noexcept
    {
        while (isspace(lexer.m_char) != 0)
        {
            lexer = nextc(lexer, content);
        }

        if (lexer.index() >= content.size()) [[unlikely]]
        {
            return std::make_pair(lexer, lexer_return(std::unexpect, Error::ContentFinish));
        }

        if (content.substr(lexer.index()).starts_with("//") || content.substr(lexer.index()).starts_with("/*"))
        {
            auto [lex, status] = skip_comment(lexer, content);
            return std::make_pair(lex, lexer_return(std::unexpect, status));
        }

        static constexpr const std::array<std::string_view, 40> tokens{
            "union", "struct", "import", "end", "else", "match",
            "do", ">>=", "->", "~", "^", "&&", "&", "||", "|", ",",
            ".", "}", "{", "]", "[", ")", "(", "!", "=", ":", "?",
            ">=", ">", "!=", "==", ">>", "<<", "<=", "<", "%", "/",
            "*", "-", "+"};

        if (std::isdigit(lexer.m_char) != 0)
        {
            return default_num_reader(lexer, content);
        }

        int count = KeywordUnion;
        for (auto &&tok : tokens)
        {
            if (content.substr(lexer.index()).starts_with(tok)) 
            {
                for (size_t i = 0; i < tok.length(); ++i)
                {
                    lexer = nextc(lexer, content);
                }
                return std::make_pair(lexer, static_cast<Tokens>(count));
            }
            --count;
        }

        if (isalpha(lexer.m_char) != 0 || lexer.m_char == '_')
        {
            const auto start = lexer.index();

            while ((std::isalnum(lexer.m_char) != 0 || lexer.m_char == '_') && lexer.m_char != '\0')
            {
                lexer = nextc(lexer, content);
            }

            return std::make_pair(lexer, std::make_pair(content.substr(start, lexer.index() - start), true));
        }

        if (content.starts_with('"'))
        {
            return default_str_reader(lexer, content);
        }

        return std::make_pair(lexer, lexer_return(std::unexpect, Error::UnknownSymbol));
    }
} // namespace zenith::lexer
