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

//#undef _FORTIFY_SOURCE // for some reason code in cmake gets strange with it

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include <compiler/compiler.h>
// #include <frontend/frontend.h>
// #include <output/formats.h>
// #include <parsing/parser.h>
#include "platform.hpp"
#include "parsing/parser.hpp"

#include <iostream>
#include <optional>
int main()
{
	auto parser = Parsing::Parser("source.znh");

	auto nodes = parser.parse();

	nodes.transform([](auto &node_list) -> int {
		std::cout << "got: " << node_list.size() << " node(s)" << std::endl;

		return 0;
	});
	
	return 0;
}
