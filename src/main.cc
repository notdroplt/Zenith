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
#include <dotenv.h>

void Error(std::string_view error, std::string_view desc)
{
	std::cerr << Color_Red << error << Color_Reset << ": " << desc << '\n';
} 

int main(int argc, char **argv) 
{
	int res = 0;
	load_dotenv();
	
	auto nodes = Parse::Parser(getenv(env_input)).File();	
	Compiler::Assembler comp(nodes);

	auto vec = comp.compile();
	auto file = std::ofstream(getenv(env_output), std::ios_base::binary);

    file.write(reinterpret_cast<char *>(vec.data()), vec.size() * sizeof(uint64_t));
    file.close();

	if (*getenv(env_compile_ihex) == '1') 
		ihex_create_file(vec.data(), vec.size() * 8, "out.hex");

	if (*getenv(env_print_disassemble) == '1')
		VirtMac::disassemble_file("out.zvm");
	
	if (*getenv(env_debug) == '1')
		res = VirtMac::run(getenv(env_output), argc, argv, VirtMac::debugger_func);
	else 
		res = VirtMac::run(getenv(env_output), argc, argv, NULL);

	return res;
}
