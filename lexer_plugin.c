#include "plugin_loader.h"

struct zlp_token token_array[] = {
    {.id = TokenPlus, .view = view_from_string_lit("+")},
    {.id = TokenMinus, .view = view_from_string_lit("-")},
    {.id = TokenMultiply, .view = view_from_string_lit("*")},
    {.id = TokenDivide, .view = view_from_string_lit("/")},
    {.id = TokenLessThan, .view = view_from_string_lit("<")},
    {.id = TokenLessThanEqual, .view = view_from_string_lit("<=")},
    {.id = TokenLeftShift, .view = view_from_string_lit("<<")},
    {.id = TokenEqual, .view = view_from_string_lit("=")},
    {.id = TokenCompareEqual, .view = view_from_string_lit("==")},
    {.id = TokenNotEqual, .view = view_from_string_lit("!=")},
    {.id = TokenGreaterThan, .view = view_from_string_lit(">")},
    {.id = TokenGreaterThanEqual, .view = view_from_string_lit(">=")},
    {.id = TokenRightShift, .view = view_from_string_lit(">>")},
    {.id = TokenQuestion, .view = view_from_string_lit("?")},
    {.id = TokenColon, .view = view_from_string_lit(":")},
	{.id = TokenNot, .view= view_from_string_lit("!")},
    {.id = TokenLeftParentesis, .view = view_from_string_lit("(")},
    {.id = TokenRightParentesis, .view = view_from_string_lit(")")},
    {.id = TokenLeftSquareBracket, .view = view_from_string_lit("[")},
    {.id = TokenRightSquareBracket, .view = view_from_string_lit("]")},
    {.id = TokenLeftCurlyBracket, .view = view_from_string_lit("{")},
    {.id = TokenRightCurlyBracket, .view = view_from_string_lit("}")},
    {.id = TokenDot, .view = view_from_string_lit(".")},
    {.id = TokenComma, .view = view_from_string_lit(",")},
    {.id = TokenBitwiseOr, .view = view_from_string_lit("|")},
    {.id = TokenBinaryOr, .view = view_from_string_lit("||")},
    {.id = TokenBitwiseAnd, .view = view_from_string_lit("&")},
    {.id = TokenBinaryAnd, .view = view_from_string_lit("&&")},
    {.id = TokenBitwiseXor, .view = view_from_string_lit("^")},
    {.id = TokenTilda, .view = view_from_string_lit("~")},
    {.id = KeywordDo, .view = view_from_string_lit("do")},
	{.id = KeywordSwitch, .view= view_from_string_lit("switch")},
    {.id = KeywordEnd, .view = view_from_string_lit("end")},
    {.id = KeywordImport, .view = view_from_string_lit("import")},
    {.id = KeywordStruct, .view = view_from_string_lit("struct")},
    {.id = KeywordUnion, .view = view_from_string_lit("union")},
    {.id = KeywordElse, .view = view_from_string_lit("else")},
};

struct lexer_plugin_t lexer_plugin = {
    .string_delimiter = view_from_string_lit("\""),
    .identifier_prefix = view_from_string_lit(""),
    .single_line_comment_prefix = view_struct("//", 2),
    .multi_line_comment_endings = {
        view_struct("/*", 2), view_struct("*/", 2)
    },
    .number_reader = NULL,
    .string_reader = NULL,
    .identifier_reader = NULL,
    .next_token = NULL,
	.token_count = sizeof(token_array)/sizeof(struct zlp_token),
    .tokens = token_array,
	.tid_integer = TokenInt,
	.tid_decimal = TokenDouble,
	.tid_string = TokenString,
	.tid_identifier = TokenIdentifier,
};

struct plugin_t default_lexer_plugin = {
    .trigger = plugin_trigger(on_token),
    .data = NULL,
    .native_handler = NULL,
    .specific_data = &lexer_plugin,
    .on_load = NULL,
    .on_unload = NULL,
	.author = "notdroplt"
};

