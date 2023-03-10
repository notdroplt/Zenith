/**
 * @file lex.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief definitions for the tokenizer
 * @version 0.0.1
 * @date 2023-01-15
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#ifndef ZENITH_LEX_H
#define ZENITH_LEX_H 1

#include <stdint.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <platform.h>


/**
 * @enum TokenTypes
 * @brief All possible token types
 * 
 * size : 1 byte
 */

enum TokenTypes
{
	TT_Unknown,			   /*!< used as the default token in case of errors */
	TT_Int,				   /*!< integer numbers */
	TT_Double,			   /*!< decimal numbers */
	TT_String,			   /*!< character strings */
	TT_Identifier,		   /*!< unmanaged names */
	TT_Keyword,			   /*!< language-specific names */
	TT_Domain,			   /*!< language domains */
	TT_Plus,			   /*!< "+" */
	TT_Increment,		   /*!< "++" */
	TT_Minus,			   /*!< "-" */
	TT_Decrement,		   /*!< "--" */
	TT_Multiply,		   /*!< "*" */
	TT_Divide,			   /*!< "/" */
	TT_LessThan,		   /*!< "<" */
	TT_LessThanEqual,	   /*!< "<=" */
	TT_LeftShift,		   /*!< "<<" */
	TT_RightShift,		   /*!< ">>" */
	TT_CompareEqual,	   /*!< "==" */
	TT_NotEqual,		   /*!< "!=" */
	TT_GreaterThan,		   /*!< ">" */
	TT_GreaterThanEqual,   /*!< ">=" */
	TT_Question,		   /*!< "?" */
	TT_Colon,			   /*!< ":" */
	TT_Equal,			   /*!< "=" */
	TT_Not,				   /*!< "!" */
	TT_LeftParentesis,	   /*!< "(" */
	TT_RightParentesis,	   /*!< ")" */
	TT_LeftSquareBracket,  /*!< "[" */
	TT_RightSquareBracket, /*!< "]" */
	TT_LeftCurlyBracket,   /*!< "{" */
	TT_RightCurlyBracket,  /*!< "}" */
	TT_Arrow,			   /*!< "=>" */
	TT_Dot,				   /*!< "." */
	TT_Comma,			   /*!< "," */
	TT_Semicolon,		   /*!< ";" */
	TT_At,				   /*!< "@" */
	TT_BitwiseOr,		   /*!< "|" */
	TT_BinaryOr,		   /*!< "||" */
	TT_BitwiseAnd,		   /*!< "&" */
	TT_BinaryAnd,		   /*!< "&&" */
	TT_BitWiseXor,		   /*!< "^" */
	TT_Tilda			   /*!< "~" */

};

/**
 * @enum KeywordTypes
 * @brief Saves space by deallocating space for keywords
 *
 *
 * size: 1 byte
 */
enum KeywordTypes
{
	KW_Unknown, /*!< error code for keywords */

	KW_var,		 /*!< "var" */
	KW_function, /*!< "function" */
	KW_as,		 /*!< "as" */
	KW_do,		 /*!< "do" */
	KW_switch,	 /*!< "switch" */
	KW_default,	 /*!< "default" */
	KW_if,		 /*!< "if" */
	KW_then,	 /*!< "if" */
	KW_else,	 /*!< "else" */
	KW_end,		 /*!< "end" */
	KW_return,	 /*!< "return" */
	KW_include	 /*!< "include" */

};

/**
 * @enum DomainTypes
 * @brief type for domain definitions
 * 
 * size: 1 byte
*/
enum DomainTypes
{
	DM_byte,  /*!< 1 byte:  N ??? [0, 2???)  */
	DM_hword, /*!< 2 bytes: N ??? [0, 2?????) */
	DM_word,  /*!< 4 bytes: N ??? [0, 2????) */
	DM_dword, /*!< 8 bytes: N ??? [0, 2??????) */

	DM_char,  /*!< 1 byte: N ??? [-2???, 2???) */
	DM_short, /*!< 2 bytes: N ??? [-2?????, 2?????) */
	DM_int,	  /*!< 4 bytes: N ??? [-2????, 2????) */
	DM_long,  /*!< 8 bytes: N ??? [-2?????, 2?????) */

	DM_unknown /*!< error code for domains*/
};

/**
 * @brief a struct that forms a "cursor" for the lexer
 *
 * Size: 16 bytes
 */
struct pos_t
{
	uint32_t index;			/*!< current character count */
	uint32_t last_line_pos; /*!< index of the last newline character */
	uint32_t line;			/*!< line count */
	uint32_t column;		/*!< column count */
};

/**
 * @brief defines a tokenizer object
 *
 */
struct lex_t
{
	struct pos_t position; /*!< current tokenizer cursor position*/
	uint64_t file_size;	   /*!< total file size */
	char *content;		   /*!< file content */
	char current_char;	   /*!< current cursor char*/
};


/**
 * @brief defines a struct that carries tokens
 *
 */
struct token_t
{
	union values
	{
		double number;				/*!< decimal numbers */
		uint64_t integer;			/*!< integer numbers */
		enum KeywordTypes keyword;	/*!< language keywords */
	} val;							/*!< possible values */		
	struct string_t string; /*!< strings or identifiers */
	enum TokenTypes type;	/*!< specify token type */
};

#define UNDEFINED_TOK (struct token_t){.type = TT_Unknown, .val.integer = 0, .string.string = NULL, .string.size = 0}
/**
 * @brief gets a new token from the content array
 *
 * @param lex lex object
 *
 * @return the next tokenized object
 */
struct token_t getNextToken(struct lex_t *lex);

/**
 * @brief compares a C string view with a C string
 *
 *
 * @param s1 C string view
 * @param [in] s2 C string
 * @returns 0 if equal
 * @returns non-zero if different
 */
int strvcmp(const struct string_t s1, const char *s2);

#endif
