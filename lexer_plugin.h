#pragma once

#include "plugin_loader.h"
#include "view.h"

typedef template_view(char) string_view;

#ifndef ZLP_COMMENT_DID_NOT_FINISH
//!< when a comment doesn't finish before the end of the file
#define ZLP_COMMENT_DID_NOT_FINISH -1
#endif

#ifndef ZLP_UNKNOWN_NUMBER_BASE
//!< a base given to the number parser is not 0b, 0o, 0d, 0h or 0x
#define ZLP_UNKNOWN_NUMBER_BASE -2
#endif

#ifndef ZLP_UNKNOWN_SYMBOL
//!< whatever reached the end of the lexer function will be returned here
#define ZLP_UNKNOWN_SYMBOL -3
#endif
/**
 * @brief default tokens that the lexer uses
 *
 * @note those are just the default, the plugin loader does any limitation on
 * what the tokens mean, just the token format
 *
 */
enum DefaultTokens
{
    TokenInt,                /*! integer numbers */
	TokenUint,				 /*! unsigned integer numbers */
    TokenDouble,             /*! decimal numbers */
    TokenString,             /*! character strings */
    TokenIdentifier,         /*! unmanaged names */
    TokenPlus,               /*! "+" */
    TokenMinus,              /*! "-" */
    TokenMultiply,           /*! "*" */
    TokenDivide,             /*! "/" */
    TokenLessThan,           /*! "<" */
    TokenLessThanEqual,      /*! "<=" */
    TokenLeftShift,          /*! "<<" */
    TokenRightShift,         /*! ">>" */
    TokenCompareEqual,       /*! "==" */
    TokenNotEqual,           /*! "!=" */
    TokenGreaterThan,        /*! ">" */
    TokenGreaterThanEqual,   /*! ">=" */
    TokenQuestion,           /*! "?" */
    TokenColon,              /*! ":" */
    TokenEqual,              /*! "=" */
    TokenNot,                /*! "!" */
    TokenLeftParentesis,     /*! "(" */
    TokenRightParentesis,    /*! ")" */
    TokenLeftSquareBracket,  /*! "[" */
    TokenRightSquareBracket, /*! "]" */
    TokenLeftCurlyBracket,   /*! "{" */
    TokenRightCurlyBracket,  /*! "}" */
    TokenDot,                /*! "." */
    TokenComma,              /*! "," */
    TokenBitwiseOr,          /*! "|" */
    TokenBinaryOr,           /*! "||" */
    TokenBitwiseAnd,         /*! "&" */
    TokenBinaryAnd,          /*! "&&" */
    TokenBitwiseXor,         /*! "^" */
    TokenTilda,              /*! "~" */
    KeywordDo,               /*! "do" */
    KeywordSwitch,           /*! "switch" */
    KeywordElse,             /*! "else" */
    KeywordEnd,              /*! "end" */
    KeywordImport,           /*! "import" */
    KeywordStruct,           /*! "struct" */
    KeywordUnion,            /*! "union" */
    EndToken				 /*! End tokenizing after receiving this token */
};

/**
 * @brief define a token for the lexer
 *
 * @todo make this more flexible by allowing any token structure order
 *
 */
struct zlp_token
{
	union {
	    size_t values[2];
		string_view view;
		int64_t integer;
		uint64_t uinteger;
		double decimal;
	};
    size_t id;
};

// forward declaration
struct lexer_plugin_t;

/**
 * @brief define an integer reader for the languge
 * @param [in] plugin lexer plugin data
 * @param [in] str content string
 * @param [out] integer integer value possibly returned by the function
 * @param [out] decimal decimal value possibly returned by the function
 * @returns 1 if the function returned an integer
 * @returns 0 if the function returned a double
 * @returns < 0 on error
 *
 */
typedef int (*zlp_num_reader_t)(struct lexer_plugin_t *, string_view *, uint64_t *, double *);
typedef string_view (*zlp_str_reader_t)(struct lexer_plugin_t *, string_view *);

/**
 * @brief define an identifier reader for the language
 *
 */
typedef string_view (*zlp_id_reader_t)(struct lexer_plugin_t *, string_view *);

/**
 * @brief if this function is non-null on the lexer plugin variables, use this instead of the
 * default lexer driver code
 */
typedef struct zlp_token *(*zlp_next_token)(allocator_param(pool_t, struct plugin_t *, string_view *));


struct lexer_plugin_t
{
    /*! define what should trigger a %str call */
    string_view string_delimiter;

    /*! define any prefix to an %id call, if NULL, any value which returns non zero on isalpha */
    string_view identifier_prefix;

    /*! define a prefix that ignores the rest of the characters until a new line */
    string_view single_line_comment_prefix;

    string_view multi_line_comment_endings[2];
    zlp_num_reader_t number_reader;
    zlp_str_reader_t string_reader;
    zlp_id_reader_t identifier_reader;
    zlp_next_token next_token;
	size_t token_count;
    struct zlp_token *tokens;
	uint8_t tid_integer;
	uint8_t tid_decimal;
	uint8_t tid_string;
	uint8_t tid_identifier;
};

struct zlp_token * expose_tokens(struct plugin_t * lexer);

struct zlp_token next_token(struct plugin_t * plugin, string_view * content);
