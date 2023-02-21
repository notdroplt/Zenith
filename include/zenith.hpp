/**
 * @file zenith.hpp
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief define everything
 * @version
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

/**
 * \mainpage Zenith language
 * \section Introduction
 *
 * Zenith is a functional-like language, developed by me alone as a project using [scientific methods](https://knowyourmeme.com/photos/2110821-fuck-around-and-find-out),
 * to achieve some math with programming, to make some research over it, and also use only a single programming language instead of many.
 *
 * Even with the code as robust as it is, as cited in the code's license, there is no warranty that it will work as intended for now,
 * but I hope in the long term it is at least stable.
 *
 * The development of the language, even though it is from an entirely different paradigm, is
 * heavily inspired by [this BASIC interpreter](https://github.com/davidcallanan/py-myopl-code) [series](https://www.youtube.com/watch?v=Eythq9848Fg&list=PLZQftyCk7_SdoVexSmwy_tBgs7P0b97yD)
 * by [Code pulse](https://www.youtube.com/channel/UCUVahoidFA7F3Asfvamrm7w)
 *
 * Remember, this is a "vacation" project (as these last 3 months kinda were vacation), I have no idea if i will continue developing it while on high school specially
 *
 * \section reason Why did I make this
 * In short, no reason at all. but it was either this or descent into madness during 3 months where i was not caring much about grades.
 *
 *
 *
 * \section Syntax Zenith Syntax
 * Zenith will look like, most of the time, as your normal functional language, with functions being the center of everything
 *
 *
 */
#ifndef ZENITH_HEADER_H
#define ZENITH_HEADER_H 1

#include <string_view>
#include <unordered_map>
#include <vector>
#include <bitset>
extern "C"
{
#include <types.h>
}
/**
 * @namespace AnsiFormat
 * @brief provides [ANSI](https://en.wikipedia.org/wiki/ANSI_escape_code) color encoding
 *
 */
namespace AnsiFormat
{
	static constexpr const char *const Red = "\x1b[31m";	 //!< outputs red
	static constexpr const char *const Green = "\x1b[32m";	 //!< outputs green
	static constexpr const char *const Yellow = "\x1b[33m";	 //!< outputs yellow
	static constexpr const char *const Blue = "\x1b[34m";	 //!< outputs blue
	static constexpr const char *const Magenta = "\x1b[35m"; //!< outputs magenta
	static constexpr const char *const Cyan = "\x1b[36m";	 //!< outputs cyan
	static constexpr const char *const Bold = "\x1b[1m";	 //!< <strong> outputs bold text </strong>
	static constexpr const char *const Reset = "\x1b[0m";	 //!< clears styled output
} // namespace AnsiFormat

/**
 * @brief prints an error message to output (stderr)
 *
 * @param error error name
 * @param desc error description
 *
 * @note errors are not fatal
 */
void Error(std::string_view error, std::string_view desc);

namespace Parse
{
	extern "C"
	{
#include "lex.h"
#include "nodes.h"
	};
} // namespace Parse

namespace VirtMac
{
	extern "C"
	{
#include "zenithvm.h"
	};
} // namespace VirtMac

namespace Compiler
{
	/**
	 * @brief type for instruction prefixes
	 *
	 */
	using prefixes = VirtMac::instruction_prefixes;
	
	/**
	 * @brief defines a type for all `assemble_(node)` functions
	 *
	 */
	using return_t = std::vector<uint64_t>;

	/**
	 * @brief defines a type for a symbol table entry
	 *
	 */
	struct symbol_table_entry
	{
		bool is_expression = false;								//!< set if current entry is an expression rather than a function
		uint64_t dot = 0;										//!< current place at file
		uint64_t value = -1;									//!< value, if immediate
		uint16_t reg_idx = -1;									//!< value, if register
		uint8_t arg_count;										//!< amount of arguments, if function
		std::unordered_map<std::string_view, uint64_t> entries; //!< registers used in arguments, if function
	};

	/**
	 * @brief assemble nodes to form running code (does not format)
	 * not the best name because it goes from nodes -> bytes, but anyway
	 *
	 * the compiler will basically go through the node tree and compile node by node in the least optimal way,
	 * but generates code
	 */
	class Assembler
	{
	public:
		/**
		 * @brief enum for probable status of registers
		 *
		 */
		enum register_status : bool
		{
			trashed = 0, //!< bogus values on register, need to do a cleanup
			used = 1,	 //!< the compiler is using this register
		};

		std::vector<Parse::node_pointer> &parsed_nodes;					//!< array of nodes to compile
		std::unordered_map<std::string_view, symbol_table_entry> table; //!< symbols
		byte_container instructions;									//!< container for all instructions
		uint64_t dot;													//!< current instruction place in memory
		uint64_t root_index;											//!< used to get info about a function context
		uint64_t entry_point;											//!< value of `dot` on main
		std::bitset<32> registers;										//!< save status for all registers (set if used)
		/**
		 * @brief request a register to be used, kind of a `malloc()` function
		 *
		 * @param ascending request register from r31 -> r1 instead of r1 -> r31
		 * @param set_next tries to set the next register index
		 *
		 * Complexity: Linear, depends on which registers are used or not
		 *
		 * @return index of register
		 */
		int request_register(bool ascending = false, int set_next = -1);

		/**
		 * @brief says that its already done using the function
		 *
		 * Complexity: Constant (but may be slower if value is not in cache)
		 * @param index
		 */
		void clear_register(int index);

		/**
		 * @brief adds another instruction to the instruction vector
		 *
		 * Complexity: Constant (when there is enough space)
		 *
		 * @param instruction
		 */
		void append_instruction(const VirtMac::instruction_t instruction);

		/**
		 * @brief checks if there is a function in the root node
		 *
		 * @param index index to check defined function
		 * @return the pointer to the function
		 */
		Parse::LambdaNode *get_function(uint64_t index);

		/**
		 * @brief chekcs if there is a node in
		 *
		 * @param index
		 * @return
		 */
		Parse::ExpressionNode *get_constant(uint64_t index);

		/**
		 * @brief put a immediate into a register
		 *
		 * @param node number node
		 *
		 *
		 * Complexity: Linear (depends on how much registers are used)
		 *
		 * @return register used while compiling
		 */
		uint64_t assemble_number(Parse::NumberNode *node);

		/**
		 * @brief get a value (possibly a pointer or a register) from an identifier
		 *
		 *
		 * @param node
		 * @return register / pointer
		 */
		uint64_t assemble_identifier(Parse::StringNode *node);

		/**
		 * @brief assemble an unary node
		 *
		 * @param node current unary node
		 *
		 * Complexity: Constant + child node complexity
		 *
		 * @return registers used while compiling
		 */
		return_t assemble_unary(Parse::UnaryNode *node);

		/**
		 * @brief assemble a binary node
		 *
		 * @param node current binary node
		 *
		 * @param isJumping set to `true` when the current node is a child of a
		 *
		 * Complexity: Constant + child nodes complexities
		 *
		 * @return registers used while compiling
		 */
		return_t assemble_binary(Parse::BinaryNode *node, bool isJumping);

		/**
		 * @brief assemble a ternary node
		 *
		 * @param node current ternary node
		 *
		 * Comlpexity: Constant + child node complexities
		 *
		 * @return registers used while compiling
		 */
		return_t assemble_ternary(Parse::TernaryNode *node);

		/**
		 * @brief assemble a function
		 *
		 * @param node current lambda node
		 *
		 * Complexity: Linear
		 *
		 * @return registers used when compiling
		 */
		return_t assemble_lambda(Parse::LambdaNode *node);

		/**
		 * @brief assemble a function call
		 *
		 * @param node current call node
		 *
		 * Complexity: Linear (argument evaluation)
		 *
		 * @return registers used when compiling
		 *
		 *
		 */
		return_t assemble_call(Parse::CallNode *node);

		/**
		 * @brief assembly a node
		 *
		 * @param node current node to compile
		 *
		 * Complexity: Varies
		 *
		 * @return registers used
		 */
		return_t assemble(const Parse::node_pointer &node);

		/**
		 * @brief construct the assembler
		 *
		 * @param nodes node tree
		 *
		 * Complexity: Constant
		 */
		Assembler(std::vector<Parse::node_pointer> &nodes);

		/**
		 * @brief compiles all nodes
		 *
		 * Complexity: Linear (depends on `this->parsed_nodes`' size)
		 * @return vector of machine instructions
		 */
		byte_container compile();
	};
} // namespace Compiler

#endif