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

#include "zenith.hpp"
#include <iostream>
#include <string.h>
#include "formats.h"

[[noreturn]] void Error(std::string_view error, std::string_view desc)
{
	std::cerr << Color_Red << error << Color_Reset << ": " << desc << '\n';
	exit(EXIT_FAILURE);
} 

int main(int argc, char **argv) 
{
	if (argc < 2) {
		Error("arguments", "expected a file name");
	}
	auto nodes = Parse::Parser(argv[1]).File();	
	Compiler::Assembler comp(nodes);

	comp.compile();

	ihex_create_file(comp.instructions.data(), comp.instructions.size() * 8, "out.hex");
	VirtMac::disassemble_file("out.zvm");
	
	return VirtMac::run("out.zvm", argc, argv, NULL);
}
