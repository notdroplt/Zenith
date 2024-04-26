#include <sstream>
#include <fstream>
#include "lexer.hh"
#include "parser.hh"
#include <iostream>

std::string readfile(const char * name) {
	std::ifstream file(name);
	std::stringstream strstr;
	file >> strstr.rdbuf();
	return strstr.str();
}


int main() 
{
	std::string content = readfile("main.znt");
	auto parser = zenith::parser::Parser(content);
	
	auto [par1, node] = zenith::parser::next_node(parser);

	if (node.has_value())
		std::cout << "node has value\n" ;
	else 
		std::cout << "node does not have value\n";
	
	return 0;
}
