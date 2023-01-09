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

#include <cstdio>
#include <fstream>
#include <memory>
#include "zenith.hpp"

[[noreturn]] void Error(std::string_view error, std::string_view desc)
{
	std::cerr << error << ": " << desc << '\n';
	exit(EXIT_FAILURE);
}


int main(int argc, char **argv) 
{
	if (argc < 2) {
		Error("arguments", "expected a file name");
	}
	auto nodes = Parse::Parser(argv[1]).File();	

	return nodes.empty() || !nodes[0] ? 1 : Interpreter::interpreter_init(argc, nodes);

}