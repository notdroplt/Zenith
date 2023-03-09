/**
 * file main.cc
 * \author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * \brief main file
 * \version
 * \date 2022-12-02
 *
 * \copyright Copyright (c) 2022
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"
#include "platform.h"
#include "dotenv.h"
#include "formats.h"
#include "parser.h"
#include "types.h"
#include "zenithvm.h"

static void print_help(void) {
	puts("Zenith version " platform_ver_str ", 2023 droplt\n"
		 "= Zenith does only accept argument via a \".env\" file, except :\n"
		 "= -v => print version to stdout as => v" platform_ver_str "\n"
		 "= any other line arguments get you to this help prompt.\n"
		 "check the documentation and code at https://github.com/notdroplt/Zenith");
}


int main(int argc, char ** argv) 
{
	struct Parser * parser = NULL;
	struct Assembler * assembler = NULL;
	struct Array * nodes = NULL, * instructions = NULL;
	FILE * fp = NULL;
	char * content = NULL;

	if (argc > 1 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
		fputs("v" platform_ver_str "\n", stdout);
		return 0;
	} else if (argc > 1) {
		print_help();
		return 0;
	}

    load_dotenv();
	
	parser = create_parser(getenv(env_input));

	content = parser->lexer.content;

	if (!parser) return 1;

	nodes = translate_unit(parser);

	if (!nodes) return 1;

	assembler = create_assembler(nodes);

	if (!assembler) return 1;

	instructions = compile_unit(assembler);

	if (!instructions) return 1;

	fp = fopen(getenv(env_output), "w");
	if (!fp) return 1;

	fwrite(
		array_get_ptr(instructions), 
		array_size(instructions) * sizeof(uint64_t),
		1,
		fp
	);

	fclose(fp);

	disassemble_file(getenv(env_output));
	delete_array(nodes, (deleter_func) delete_node);
	delete_array(instructions, NULL);
	free(content);
	return 0;
}
