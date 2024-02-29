#include "parser.h"
#include <stdio.h> //later todo: make this use a custom "file i/o module"
#include "allocator.h"

struct Parser * create_parser(allocator_param(pool_t pool, struct plugin_t * const lexer, char * filename)) {
	struct Parser * parser = alloc(pool, sizeof(struct Parser), alignof(struct Parser), 1, 0);
	FILE * fp = NULL;
	if (!parser) return NULL;

	parser->lexer = lexer;

	parser->content = alloc(pool, sizeof(struct Parser), alignof(struct Parser), 1, 0);

	if (!parser.content) goto destructor_0;

	fp = fopen(filename, "r");

	if (!fp) goto destructor_1;

	fseek(fp, 0, SEEK_END);
	parser->content.size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	parser->content.ptr = alloc(NULL, sizeof(char), alignof(char), parser->content.size, 0);
	if (fread(parser->content.ptr, sizeof(char), parser->content.size, fp) != parser->content.size) {
		fprintf(stderr, "unable to read file \"%s\"\n", );
		return 1;
	}


	return parser;
destructor_2:
	fclose(fp);
destructor_1:
	dealloc(pool, parser.content, 0);
destructor_0:
	dealloc(pool, parser, 0);
	return NULL;
}
