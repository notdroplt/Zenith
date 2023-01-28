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

[[noreturn]] void Error(std::string_view error, std::string_view desc)
{
	std::cerr << AnsiFormat::Red << error << AnsiFormat::Reset << ": " << desc << '\n';
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

	

	VirtMac::disassemble_file("out.zvm");
	if (argc > 2 && strcmp(argv[2], "debug") == 0) 
		return VirtMac::run("out.zvm", argc, argv, VirtMac::debugger_func);
	
	return VirtMac::run("out.zvm", argc, argv, NULL);
}
