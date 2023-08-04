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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include <compiler/compiler.h>
#include <frontend/frontend.h>
// #include <output/formats.h>
// #include <parsing/parser.h>
#include <platform.h>
#include <types/types.h>
#include <parsing/parser.hpp>
#include <runners/loader.hpp>

// #include <virtualmachine/zenithvm.h>

// static void print_help(void)
// {
// 	puts("Zenith version " platform_ver_str ", 2023 droplt\n"
// 		 "= Zenith does only accept argument via a \".env\" file, except :\n"
// 		 "= -v => print version to stdout as => v" platform_ver_str "\n"
// 		 "= any other line arguments get you to this help prompt.\n"
// 		 "check the documentation and code at https://github.com/notdroplt/Zenith");
// }

#include <iostream>

int main()
{
	auto parser = Parsing::Parser("source.znh");

	auto nodes = parser.parse();

	if (nodes) {
		std::cout << "got: " << nodes->size() << " node(s)" << std::endl;
	} 
	// struct Parser *parser = NULL;
	// struct Assembler *assembler = NULL;
	// struct array_t *nodes = NULL;
	// struct array_t *instructions = NULL;
	// struct HashMap * options = NULL;
	// FILE *fpointer = NULL;
	// char *content = NULL;

	// options = parse_commandline(argc, argv);
	// (void) options;
	// if (argc > 1 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0))
	// {
	// 	(void)fputs("v" platform_ver_str "\n", stdout);
	// 	return 0;
	// }

	// if (argc > 1)
	// {
	// 	print_help();
	// 	return 0;
	// }

	// parser = create_parser("source.znh");

	// if (!parser)
	// {
	// 	return 1;
	// }

	// content = parser->lexer.content;

	// nodes = translate_unit(parser);

	// if (!nodes)
	// {
	// 	return 1;
	// }

	// assembler = create_assembler(nodes);

	// if (!assembler)
	// {
	// 	return 1;
	// }

	// instructions = compile_unit(assembler);

	// if (!instructions)
	// {
	// 	return 1;
	// }

	// fpointer = fopen("output.zvm", "we");

	// if (!fpointer)
	// {
	// 	return 1;
	// }

	// (void)fwrite(
	// 	array_get_ptr(instructions),
	// 	array_size(instructions) * sizeof(uint64_t),
	// 	1, fpointer);

	// (void)fclose(fpointer);

	// disassemble_file("output.zvm");
	// delete_assembler(assembler);
	// free(content);
	// delete_array(instructions, NULL);

	// run("output.zvm", 0, NULL, debugger_func);
	return 0;
}
