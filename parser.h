#pragma once

#include "plugin_loader.h"
#include "allocator.h"
#include "view.h"
#include "lexer_plugin.h"

//! define when parser couldn't read a file
#define PARSER_READ_ERROR -1

enum NodeID {
	NodeNone,
	NodeInt,
	NodeDouble,
	NodeString,
	NodeIdentifier,	/*! */
	NodeUnary,		/*! nodes with 1 argument */
	NodeBinary,		/*! nodes with 2 arguments */
	NodeTernary,	/*! nodes with 3 arguments */
	NodeSwitch,		/*! switch case nodes */
	NodeLambda,		/*! the function but not yet assigned a name */
	NodeFunction,	/*! its a functional language so this is expected*/
	NodeVariable,	/*! node with values that wont't change others, but can be changed*/
	NodeStruct,		/*! structured data nodes, an union is also assigned this*/
};

/***
 * @brief declare possible attributes for nodes
 *
 *
 */
enum NodeFlags {
	ReadOnly	 = 0x01, /*! the compiler cannot write to this byte (or memory range) */
	WriteOnly	 = 0x02, /*! the compiler cannot read to this byte (or memory range) */
	Mutable		 = 0x04, /*! this variable (or memory range) might change value after declared */
	Register	 = 0x08, /*! this variable (or memory range) shadows a register */
	MemoryMapped = 0x10, /*! this variable is mapped to a specific place in memory */
	Input		 = 0x20, /*! this variable (or memory range) might receive values without notice */
	Output		 = 0x40, /*! this variable changes the outside world when its value is changed */
};

struct NodeType;

struct Node {
	struct NodeType * type;
	enum NodeID id;
	enum NodeFlags flags;
};

#define null_node		\
	(struct Node){		\
		.type = NULL,	\
		.id = NoneNone,	\
		.is_mut = 0		\
	}

/**
 * this struct can hold values for
 * - NodeInt
 * - NodeDouble
*/
struct NumberNode {
	struct Node base;		/*! base node */
	union {
		uint64_t uinteger;	/*! unsigned integer */
		int64_t integer;	/*! signed integer */
		double decimal;		/*! double values */
	};
	uint8_t is_signed;		/*! mark whether NodeInt has sign or not */
};

#define null_number_node	\
	(struct NumberNode) {	\
		.base = null_node,	\
		.integer = 0,		\
		.is_signed = 0		\
	}

struct StringNode {
	struct Node base;
	string_view string;
};

struct UnaryNode {
	struct Node base;
	struct Node * operand;
	enum DefaultTokens operator;
};

struct BinaryNode {
	struct Node base;
	struct Node * left;
	struct Node * right;
	enum DefaultTokens operator;
};

struct TernaryNode {
	struct Node base;
	struct Node * condition;
	struct Node * on_true;
	struct Node * on_false;
};

struct NameNode {
	struct Node base;
	string_view * names;
	uint8_t name_count;
};

struct LambdaNode {
	struct Node base;
	struct NameNode * name;
	struct Node * arguments;
	struct Node * expression;
	uint8_t argument_count;
};

struct Parser {
	string_view * content;
	struct plugin_t * lexer;
};

struct Parser * create_parser(allocator_param(pool_t pool, struct plugin_t * const lexer, char* filename));
struct Node * get_next_node(struct Parser * parser);
