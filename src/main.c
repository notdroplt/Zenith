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
#include "dotenv.h"
#include "formats.h"
#include "parser.h"

int main() 
{
    load_dotenv();
	
	struct Parser * parser = create_parser(getenv(env_input));
	if (!parser) return 1;

	struct Array * nodes = translate_unit(parser);
	if (!nodes) return 1;

	delete_array(nodes, (deleter_func) delete_node);
	return 0;
}
