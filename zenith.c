#include "plugin_loader.h"
#include "lexer_plugin.h"
#include <stdio.h>

static void print_token(struct zlp_token * token) {
	if (token->id == TokenInt)
		printf("{ int: %ld }\n", token->integer);
	else if (token->id == TokenDouble)
		printf("{ double: %lf }\n", token->decimal);
	else if (token->id == TokenIdentifier)
		printf("{ identifier: %.*s }\n", (int)token->view.size, token->view.ptr);
	else if (token->id == TokenString)
		printf("{ str: %.*s }\n", (int)token->view.size, token->view.ptr);
	else
		printf("{ id: %ld }\n", token->id);
}

int main() {
	FILE * fp  = fopen("main.znt", "r");
	string_view content = null_view(char);
	struct zlp_token token;
	char * string_content;
	if (!fp) {
		fprintf(stderr, "could not open file\n");
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	content.size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// just to be absolutely safe
	content.ptr = alloc(NULL, sizeof(char), alignof(char), content.size, 0);
	if (fread(content.ptr, sizeof(char), content.size, fp) != content.size) {
		fprintf(stderr, "unable to read file\n");
		return 1;
	}
	string_content = content.ptr;

	token.id  = -1;
	while (token.id != EndToken) {
		token = next_token(&default_lexer_plugin, &content);
		print_token(&token);
	}
	printf("hello world\n");

	dealloc(NULL, string_content, 0);
	return 0;
}
