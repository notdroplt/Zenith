#pragma once
#ifndef ZENITH_PARSER_H
#define ZENITH_PARSER_H

#include "nodes.h"
#include "types.h"

/**
 * @brief struct necessary to generate syntax trees
 * 
 * the parser is responsible for transforming written text into a
 * structure that is easier for the compiler to understand, and those
 * are trees 
*/
struct Parser {
	struct token_t current_token; 	/*!< current parsed token*/
	struct pos_t current_position; 	/*!< current parser position */
	struct lex_t lexer;				/*!< lexer config struct Node */

	struct List * strings;	/*!< define data strings */
	struct List * symbols; 	/*!< define identifier strings */
	struct HashMap * table; /*!< define table of symbols*/

	const char * filename; 	/*!< define current file name */
	uint32_t filesize; 		/*!< define current file size*/
	bool sucessfull; 		/*!< defines if the compilation was successfull */
};

/**
 * @brief Create a parser object
 * 
 * @param filename file to parse
 * @return struct Parser* parser struct
 */
struct Parser* create_parser(const char * filename);

void delete_parser(struct Parser * parser);

struct Array* translate_unit(struct Parser * parser);

void parser_note(struct Parser * parser, char * note, char * details);
void parser_warn(struct Parser * parser, char * warn, char * details);
void parser_error(struct Parser * parser, char * error, char * details);

#endif
