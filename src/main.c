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
#include "platform.h"
#include "dotenv.h"
#include "formats.h"
#include "parser.h"

static void print_help() {
	puts("Zenith version " platform_ver_str ", 2023 droplt\n"
		 "= Zenith does only accept argument via a \".env\" file, except :\n"
		 "= -v => print version to stdout as => v" platform_ver_str "\n"
		 "= any other line arguments get you to this help prompt.\n"
		 "check the documentation and code at https://github.com/notdroplt/Zenith");
}


int main(int argc, char ** argv) 
{
	if (argc > 1 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
		fputs("v" platform_ver_str "\n", stdout);
		return 0;
	} else if (argc > 1) {
		print_help();
		return 0;
	}
    load_dotenv();
	
	struct Parser * parser = create_parser(getenv(env_input));
	if (!parser) return 1;

	struct Array * nodes = translate_unit(parser);
	if (!nodes) return 1;

	delete_array(nodes, (deleter_func) delete_node);
	return 0;
}
