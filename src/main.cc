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
#include <fstream>
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

	auto vec = comp.compile();

	auto file = std::ofstream("out.zvm", std::ios_base::binary);
    file.write(reinterpret_cast<char *>(vec.data()), vec.size() * sizeof(uint64_t));
    file.close();

	ihex_create_file(vec.data(), vec.size() * 8, "out.hex");
	VirtMac::disassemble_file("out.zvm");
	
	return VirtMac::run("out.zvm", argc, argv, VirtMac::debugger_func);
}
