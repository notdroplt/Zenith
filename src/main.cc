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
#include <bitset>

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
	Compiler::Assembler<Compiler::ConceptTarget<>> comp(nodes);

	//VirtMac::run("test.bin");
	return nodes.empty();
}
