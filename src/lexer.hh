#pragma once

#include "utils.hh"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>

namespace zenith::lexer
{
	/**
	 * @brief tokens that the lexer uses
	 */
	enum Tokens
	{
		TokenPlus,				 /**< "+" */
		TokenMinus,				 /**< "-" */
		TokenMultiply,			 /**< "*" */
		TokenDivide,			 /**< "/" */
		TokenModule,			 /**< "%" */
		TokenLessThan,			 /**< "<" */
		TokenLessThanEqual,		 /**< "<=" */
		TokenLeftShift,			 /**< "<<" */
		TokenRightShift,		 /**< ">>" */
		TokenCompareEqual,		 /**< "==" */
		TokenNotEqual,			 /**< "!=" */
		TokenGreaterThan,		 /**< ">" */
		TokenGreaterThanEqual,	 /**< ">=" */
		TokenQuestion,			 /**< "?" */
		TokenColon,				 /**< ":" */
		TokenEqual,				 /**< "=" */
		TokenNot,				 /**< "!" */
		TokenLeftParentesis,	 /**< "(" */
		TokenRightParentesis,	 /**< ")" */
		TokenLeftSquareBracket,	 /**< "[" */
		TokenRightSquareBracket, /**< "]" */
		TokenLeftCurlyBracket,	 /**< "{" */
		TokenRightCurlyBracket,	 /**< "}" */
		TokenDot,				 /**< "." */
		TokenComma,				 /**< "," */
		TokenBitwiseOr,			 /**< "|" */
		TokenBinaryOr,			 /**< "||" */
		TokenBitwiseAnd,		 /**< "&" */
		TokenBinaryAnd,			 /**< "&&" */
		TokenBitwiseXor,		 /**< "^" */
		TokenTilda,				 /**< "~" */
		TokenArrow,				 /**< "->" */
		TokenBind,				 /**< ">>=" */
		KeywordDo,				 /**< "do" */
		KeywordMatch,			 /**< "match" */
		KeywordElse,			 /**< "else" */
		KeywordEnd,				 /**< "end" */
		KeywordImport,			 /**< "import" */
		KeywordStruct,			 /**< "struct" */
		KeywordUnion,			 /**< "union" */

		/*== from now on these actions are just aliases ==*/
		ActionIndex = TokenLeftSquareBracket, /** a[i] */
		ActionMember = TokenDot, /** a.b */
		ActionPointerMember = TokenArrow, /** a->b */
		ActionReference = TokenBitwiseAnd, /** &a */
		ActionPointer = TokenMultiply, /** *a */
		ActionValuefull = TokenNot, /** !a */
		ActionForall = TokenQuestion /** ?a */

	};

	using StringToken = std::pair<std::string_view, bool>;

	/**
	 * @brief define a token as an union of different types
	 */
	using Token = std::variant<Tokens, StringToken, int64_t, uint64_t, double>;

	enum TokenVariants {
		VarTokens,
		VarString,
		VarInt,
		VarUint,
		VarDouble
	};

	struct Lexer final
	{
		/** current position inside the file*/
		struct position_t
		{
			uint32_t m_index = 0;
			uint16_t m_line = 1;
			uint16_t m_column = 1;
		} m_pos;

		char m_char = '\0';

		constexpr explicit Lexer(std::string_view str) :  m_pos{0, 1, 1}, m_char(str.front()) {}
		constexpr Lexer(position_t pos, char chr) : m_pos{pos}, m_char{chr} {}
		constexpr Lexer() = default;
		/**
		 * @brief get the position inside the lexer
		 * 
		 * @return position struct 
		 */
		ZENITH_CONST_NODISCARD_CONSTEXPR auto pos() const noexcept
		{ return this->m_pos; }

		/**
		 * @brief get the index of the lexer inside the content
		 * 
		 * @return uint32_t index
		 */
		ZENITH_CONST_NODISCARD_CONSTEXPR auto index() const noexcept
		{ return this->m_pos.m_index; }

		/**
		 * @brief get the line the lexer in
		 * 
		 * @return uint16_t line
		 */
		ZENITH_CONST_NODISCARD_CONSTEXPR auto line() const noexcept
		{ return this->m_pos.m_line; }

		/**
		 * @brief get the column the lexer is in
		 * 
		 * @return uint16_t column
		 */
		ZENITH_CONST_NODISCARD_CONSTEXPR auto column() const noexcept
		{ return this->m_pos.m_column; }

		/**
		 * @brief get the current char
		 * 
		 * @return char current char
		 */
		ZENITH_CONST_NODISCARD_CONSTEXPR auto chr() const noexcept
		{ return this->m_char; }

	};

	using lexer_return = std::expected<Token, errors::Errors>;
	
	ZENITH_CONST_NODISCARD Lexer init_lexer(std::string_view content) noexcept;

	/**
	 * @brief get a new token from the token string
	 *
	 * @param lexer lexer to run
	 * @param content content to tokenize
	 *
	 * @returns token when tokenization is sucessful
	 * @returns int when something fails
	 *
	 * @note content is changed as the string is tokenized
	 */
	ZENITH_CONST_NODISCARD std::pair<Lexer, lexer_return> next_token(Lexer lexer, std::string_view content) noexcept;
}; // namespace zenith::lexer
