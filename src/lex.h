#include <stdint.h>
#include <stddef.h>

/**
 * @enum TokenTypes
 * @brief All possible token types
 * <br>
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
 * <br>
 * size: 1 byte
 */
enum KeywordTypes
{
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
	KW_include,	 /*!< "include" */

	KW_Unknown /*!< error code for keywords */
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

struct lex_t
{
	struct pos_t position;
	uint64_t file_size;
	char *content;
	char current_char;
};

/**
 * @brief a c string_view object
 *
 * Size : 16 bytes
 */
#ifndef __cplusplus
struct string_t
{
	size_t size;
	char *string;
};

typedef struct string_t string_t;
#else 
typedef std::string_view string_t;
#endif
struct token_t
{
	union
	{
		double number;			   /*!< decimal numbers */
		uint64_t integer;		   /*!< integer numbers */
		enum KeywordTypes keyword; /*!< language keywords */
	};
	string_t string;		   /*!< strings or identifiers */

	enum TokenTypes type; /*!< specify token type */
};

/**
 * @brief gets a new token from the content array
 *
 * @param lex lex object
 *
 * @return the next tokenized object
 */
struct token_t getNextToken(struct lex_t *lex);