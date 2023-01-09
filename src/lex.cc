#include <cctype>

#include "zenith.hpp"


static auto lexNext(Parse::Parser *parser) -> char
{
	auto &pos = parser->current_position;
	if (pos.index >= parser->file_size) {
		return '\0';
	}
	++pos.index;
	++pos.column;

	parser->current_char = pos.index < parser->file_size ? parser->content[pos.index] : '\0';

	if (parser->current_char == '\n') {
		pos.column = 1;
		pos.last_line_pos = pos.index;
		++pos.line;
	}
	return parser->current_char;
}

inline auto lexString(Parse::Parser *parser) -> Parse::Token
{
	auto offset = 0ULL;
	const auto start = parser->current_position.index + 1; /* skip the first " */

	lexNext(parser);
	while (parser->current_char != '"' && (parser->current_char != 0)) {
		lexNext(parser);
	}

	lexNext(parser);

	if (!parser->current_char) {
		parser->Error("Lexer", "current string to finish before the end of the file");
		return Parse::Token();
	}

	offset = parser->current_position.index - start - 1; /* remove the outer " */

	return Parse::Token(parser->content.substr(start, offset), Parse::TT_String);
}

static inline auto lexNumber(Parse::Parser *parser) -> Parse::Token
{
	bool dot = false;
	std::string numstr;

	do {
		if (parser->current_char == '.' && dot) {
			parser->Error("Lexer", "no more than one dot to a integer constant");
			return Parse::Token();
		} else if (parser->current_char == '.') {
			dot = true;
		}
		numstr += parser->current_char;

	} while (lexNext(parser) && (isdigit(parser->current_char) || parser->current_char == '.'));

	return dot ? Parse::Token(std::stod(numstr))
			   : Parse::Token(static_cast<uint64_t>(std::stoull(numstr)));
}

static inline auto lexNewToken(Parse::Parser *parser, Parse::TokenTypes val) -> Parse::Token
{
	Parse::Token tok(val);
	lexNext(parser);
	return tok;
}

static inline auto lexIdentifier(Parse::Parser *parser) -> Parse::Token
{
	uint64_t offset = 0;
	auto start = parser->current_position.index;

	static const std::vector<std::string_view> keywords{"var",	  "function", "as",		 "do",
														"switch", "default",  "if",		 "else",
														"end",	  "return",	  "include", ""};
	do {
		lexNext(parser);
	} while (isalnum(parser->current_char) || parser->current_char == '_');

	offset = parser->current_position.index - start;

	Parse::Token tok(parser->content.substr(start, offset), Parse::TT_Identifier);

	for (size_t i = 0; i < Parse::KW_Unknown; ++i) {
		if (tok.string == keywords.at(i)) {
			tok.type = Parse::TT_Keyword;
			tok.keyword = static_cast<Parse::KeywordTypes>(i);
			break;
		}
	}

	return tok;
}

static inline auto lexCompare(Parse::Parser *parser, Parse::TokenTypes value) -> Parse::Token
{
	Parse::Token tok(value);
	lexNext(parser);
	if (parser->current_char == '=') {
		tok.type = static_cast<Parse::TokenTypes>(value + 1);
		lexNext(parser);
	}
	return tok;
}

static inline auto lexEqual(Parse::Parser *parser) -> Parse::Token
{
	Parse::Token tok(Parse::TT_Equal);
	lexNext(parser);

	if (parser->current_char == '=') {
		tok.type = Parse::TT_CompareEqual;
		lexNext(parser);
	} else if (parser->current_char == '>') {
		tok.type = Parse::TT_Arrow;
		lexNext(parser);
	}
	return tok;
}

static inline auto lexNot(Parse::Parser *parser) -> Parse::Token
{
	Parse::Token tok(Parse::TT_Not);
	lexNext(parser);
	if (parser->current_char == '=') {
		tok.type = Parse::TT_NotEqual;
		lexNext(parser);
	}

	return tok;
}

static inline auto lexMinus(Parse::Parser *parser) -> Parse::Token
{
	Parse::Token tok(Parse::TT_Minus);
	lexNext(parser);
	if (parser->current_char == '>') {
		tok.type = Parse::TT_Arrow;
		lexNext(parser);
	}
	return tok;
}

void Parse::Parser::next()
{
	auto &tok = this->current_token;
	switch (this->current_char) {
		case '\0':
			tok = Token();
			break;
		case '\n':
		case '\t':
		case '\v':
		case '\r':
		case ' ':
			lexNext(this);
			this->next();
			break;
		case '"':
			tok = lexString(this);
			break;
		case '#':
			while (this->current_char != '\0' && this->current_char != '\n') {
				lexNext(this);
			}
			this->next();
			break;
		case ';':
			tok = lexNewToken(this, Parse::TT_Semicolon);
			break;
		case '+':
			tok = lexNewToken(this, Parse::TT_Plus);
			break;
		case '-':
			tok = lexMinus(this);
			break;
		case '/':
			tok = lexNewToken(this, Parse::TT_Divide);
			break;
		case '*':
			tok = lexNewToken(this, Parse::TT_Multiply);
			break;
		case '=':
			tok = lexEqual(this);
			break;
		case '!':
			tok = lexNot(this);
			break;
		case '?':
			tok = lexNewToken(this, Parse::TT_Question);
			break;
		case ':':
			tok = lexNewToken(this, Parse::TT_Colon);
			break;
		case '>':
			tok = lexCompare(this, Parse::TT_GreaterThan);
			break;
		case '<':
			tok = lexCompare(this, Parse::TT_LessThan);
			break;
		case '(':
			tok = lexNewToken(this, Parse::TT_LeftParentesis);
			break;
		case ')':
			tok = lexNewToken(this, Parse::TT_RightParentesis);
			break;
		case '[':
			tok = lexNewToken(this, Parse::TT_LeftSquareBracket);
			break;
		case ']':
			tok = lexNewToken(this, Parse::TT_RightSquareBracket);
			break;
		case '{':
			tok = lexNewToken(this, Parse::TT_LeftSquareBracket);
			break;
		case '}':
			tok = lexNewToken(this, Parse::TT_RightSquareBracket);
			break;
		case ',':
			tok = lexNewToken(this, Parse::TT_Comma);
			break;
		case '.':
			tok = lexNewToken(this, Parse::TT_Dot);
			break;
		case '@':
			tok = lexNewToken(this, Parse::TT_At);
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			tok = lexNumber(this);
			break;
		default:
			tok = lexIdentifier(this);
			break;
	}
}
