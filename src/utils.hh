#pragma once
#include <memory>
#include <utility>

#define ZENITH_CONST_NODISCARD [[nodiscard, gnu::const]]
#define ZENITH_CONST_NODISCARD_CONSTEXPR ZENITH_CONST_NODISCARD constexpr

//NOLINTBEGIN: needed to work with the prerocessor to disable some code areas on
// debug mode
#define ZENITH_DEBUG 1
#define ZENITH_DISABLE_LEXER_DEBUG 0
#define ZENITH_DISABLE_PARSER_DEBUG 0

#define ZENITH_LEXER_DEBUG (ZENITH_DEBUG && !ZENITH_DISABLE_LEXER_DEBUG)
#define ZENITH_PARSER_DEBUG (ZENITH_DEBUG && !ZENITH_DISABLE_PARSER_DEBUG)
//NOLINTEND

template <class T, class... Args>
[[nodiscard]] std::unique_ptr<T> make_unique_nothrow(Args &&...args) noexcept
{
    return std::unique_ptr<T>(new (std::nothrow) T(std::forward<Args>(args)...));
}

namespace zenith::errors
{
    /**
     * @brief any error that might occur in the
     *
     */
    enum class Errors
    {
        /**
         * @defgroup LexErrorCodes Lexer errors
         *
         * @brief define errors that might be returned by the lexer
         *
         * @{
         */
        CommentFinish,        /**< comment finished, need to run again*/
        ContentFinish,        /**< content finished but token was requested */
        CommentDidNotFinish,  /**< unfinished comment until the end of the file */
        UnknownNumberBase,    /**< an unknown base was used for a number */
        DecimalAfterDot,      /**< number string ended on a dot like `123.` */
        InvalidNumericChar,   /**< an invalid char was passed to the numeric reader*/
        EmptyExponent,        /**< there was no number after the exponent as in `1.4e`*/
        UnknownSymbol,        /**< lexer did not recognize this symbol */
        StringDidNotFinish,   /**< unfinished string until the end of the file */
        IdentifierPrefixOnly, /**< only the identifier prefix was given*/
        LexerErrorEnd,        /**< end lexer errors and start parser ones */
        /** @} */

        /**
         * @defgroup ParseErrorCodes Parser codes
         *
         * @brief define syntax errors that might be returned by the parser
         *
         * @{
         */
        OutOfMemory,                   /**< parser ran out of memory while processing */
        InvalidTokenVariant,           /**< token invariant was not expected one */
        UnrecognizedAtomicToken,       /**< switch on atomic nodes got to the default branch */
        ClosingParentesisOnExpression, /**< expression enclosed in () did not have a finish )*/
        ClosingBracketOnIndex,         /**< index stared but did not finish */
        PrefixToken,                   /**< token found on prefix is unknown */
        ExpressionAfterQuestion,       /**< expression was needed after a `?` on a ternary */
        ColonAfterExpression,          /**< there was not a colon in the middle of a ternary statement */
        ExpressionAfterColon,          /**< expression was needed after a `:` on a ternary */
        NameOnTopLevel,                /**< expected a name to start an expression*/
        EqualsOrColon,                 /**< expected an equals "=" or a colon ":" on top level expression */
        EmptyNameAssignment,           /**< name was assigned to nothing */
        NoTypesYet
        /** @} */
    };

}; // namespace zenith::errors