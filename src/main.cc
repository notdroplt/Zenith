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



static void print_instruction (uint64_t instruction) {
	uint8_t * bytes = (uint8_t*)&instruction;
	std::cerr << std::hex << (int) bytes[0] << ' '
	<< std::hex << (int)bytes[1] << ' '
	<< std::hex << (int)bytes[2] << ' '
	<< std::hex << (int)bytes[3] << ' '
	<< std::hex << (int)bytes[4] << ' '
	<< std::hex << (int)bytes[5] << ' '
	<< std::hex << (int)bytes[6] << ' '
	<< std::hex << (int)bytes[7] << '\n';
}

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

	for (auto && inst : comp.compile()) {
		print_instruction(inst);
	}
	VirtMac::run("test.bin");
	return nodes.empty();
}
