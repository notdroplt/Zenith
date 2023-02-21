#pragma once
#ifndef ZENITH_PARSER_H
#define ZENITH_PARSER_H
#include "nodes.h"
#include "types.h"

struct Parser {
	struct token_t current_token;
	struct pos_t current_position;
	struct lex_t lexer;

	struct List * strings;
	struct List * symbols;
	struct HashMap * table;

	const char * filename;
	uint32_t filesize;
	bool sucessfull;
};

struct Parser* create_parser(const char * filename);
void delete_parser(struct Parser * parser);

struct Array* translate_unit(struct Parser * parser);

void parser_note(struct Parser * parser, char * note, char * details);
void parser_warn(struct Parser * parser, char * warn, char * details);
void parser_error(struct Parser * parser, char * error, char * details);

#endif