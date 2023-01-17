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

#include <elf.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

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

[[noreturn]] void Error(std::string_view error, std::string_view desc);

namespace Parse
{
	extern "C"
	{
#include "lex.h"
	};

	/**
	 * @brief class for terminating program "safely"
	 *
	 * size: 8 bytes
	 */
	class parse_exception : public std::exception
	{
	};

	//////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 * @enum NodeTypes
	 * @brief enum for identifying Node types
	 *
	 * size: 1 byte
	 */
	enum NodeTypes : unsigned char
	{
		Unused = 0,					//!< used on Node's default constructor
		Integer = TT_Int,			//!< Integer Nodes
		Double = TT_Double,			//!< Double Nodes
		String = TT_String,			//!< String Nodes
		Identifier = TT_Identifier, //!< Identifier Nodes
		Unary,						//!< Unary operation nodes
		Binary,						//!< Binary operation nodes
		Ternary,					//!< Ternary operation nodes
		Expression,					//!< Top-level expression nodes
		List,						//!< List nodes
		Call,						//!< variable call nodes
		Lambda,						//!< Lambda Function Nodes
		Switch,						//!< Switch Nodes
		Define,						//!< Type definintion nodes
		Index,						//!< Index nodes
		Include,					//!< Include nodes
		Scope,						//!< Scope nodes
		Task						//!< Task nodes
	};

	/**
	 * @struct Node
	 * @brief Base class for all node types
	 *
	 * For the compiler/interpreter to understand what the code actually means,
	 * the parser creates nodes, which are recursive tree-like structures that
	 * carry context inside themselves
	 *
	 * Size: 2 bytes
	 */
	struct Node
	{
		const NodeTypes type;		//!< current node @ref NodeTypes "type"
		const bool isConst = false; //!< defines if a node is const at parse-time
		/**
		 * @brief constructs a node
		 *
		 * Complexity: Constant
		 */
		explicit Node(const NodeTypes type_) noexcept : type(type_) {}

		/**
		 * @brief constructs a node with a non-default constness (probably isConst=true)
		 *
		 * A node constness is defined by the ability of the compiler to reduce it at compile-time,
		 * while it is little, it helps a lot the interpreter as there are less nodes to interpret.
		 * but that doesn't stop the parser to kick in more implementations
		 *
		 * @param type_ current node type
		 * @param constness current node constness
		 *
		 * Complexity: Constant
		 */
		Node(const NodeTypes type_, const bool constness) noexcept : type(type_), isConst(constness) {}

		/**
		 * @brief cast base Node pointer to a derived class `T`
		 *
		 * forces `T`, `T *`, `const T` and `const T *` to become `const T *`
		 *
		 * @tparam T a type that derives to Node
		 * @return const T*
		 *
		 * Complexity: Constant
		 *
		 */
		template <typename T>
		[[nodiscard]] auto cget() noexcept
		{
			static_assert(std::is_base_of<Node, T>::value, "type is not <Node> derived");
			return static_cast<T *>(this);
		}
	};

	/**
	 * @typedef node_pointer
	 * @brief a unique pointer to Node
	 */
	using node_pointer = std::unique_ptr<Node>;

	/**
	 * @struct NumberNode
	 * @brief defines a struct to carry numbers
	 *
	 * it is one of the two nodes that actually implement the constness behavior, as numbers are, well,
	 * constant.
	 *
	 * Size: 24 bytes
	 * @todo implement @a NumberNode as an union or similar, as a number is either an integer or a float
	 * (in computers, of course)
	 */
	struct NumberNode final : public Node
	{
		uint64_t number{}; //!< integer number value
		double value{};	   //!< decimal number value

		/**
		 * @brief Constructs a number node, from a double
		 *
		 * @param value_ node value
		 *
		 * Complexity: Constant
		 */
		explicit NumberNode(const double value_) noexcept : Node(Double, true), value(value_) {}

		/**
		 * @brief Constructs a number node, from an integer
		 *
		 * @param value_ node value
		 *
		 * Complexity: Constant
		 */
		explicit NumberNode(const uint64_t value_) noexcept : Node(Integer, true), number(value_) {}
	};

	/**
	 * @struct StringNode
	 * @brief defines a struct to carry strings
	 *
	 * the string node actualyl implements the constness=true when the node type is a string
	 *
	 * @b Size: 24 bytes + string size
	 *
	 * @todo implement @a StringNode as an offset of a table, to save memory space on bigger files as
	 * names are often the same or similar.
	 * @todo implement @a StringNode 's constness while a identifier
	 * also, as the identifiers @b might be compile-time resolvable
	 */
	struct StringNode final : public Node
	{
		std::string_view value; //!< string value

		/**
		 * @brief Construct a new String Node object, either for @a strings or @a identifiers
		 *
		 * @param value_ string value
		 * @param type_ node type, either @a string or @a identifier
		 *
		 * Complexity: Constant
		 * string_view's default copy constructor only does a shallow copy of the string
		 */
		StringNode(std::string_view value_, const NodeTypes type_) noexcept : Node(type_, type_ == String), value(value_) {}
	};

	/**
	 * @struct UnaryNode
	 * @brief defines a struct to carry unary operations
	 *
	 * Size: 24 bytes
	 *
	 * An unary node is represented by a prefix operation followed by some arbitrary node
	 * @note postfix operations are discouraged as they have the same semantic meaning as the prefix
	 * ones
	 *
	 * prefix operations happen @b before the main value, its syntax is as follows:
	 *
	 * @code
	 * [token] [value]
	 * @endcode
	 *
	 * and examples of those operations are:
	 * - increment operations : `++a`
	 * - decrement operations : `--b`
	 * - binary negation (one's complement) : `~c`
	 * - numeric negation (two's complement) : `-d`
	 * - boolean negation (equivalent to != 0) : `!e`
	 *
	 * @warning using the numeric positive operation `+i` is not deprecated but just discouraged, as it
	 * accomplishes nothing and is always just discarted @note this node benefits from the node
	 * constness, as it can be inlined and even discarted at compile time if the node value is constant
	 *
	 * @todo implement `--`, `++` and `~` unary operations
	 */
	struct UnaryNode final : public Node
	{
		const node_pointer value; //!< main node value
		const TokenTypes token;	  //!< unary operation token

		/**
		 * @brief Construct a new Unary Node object
		 *
		 * @param value_ unary node's value
		 * @param token_ unary operation token
		 *
		 * Complexity: Constant
		 */
		UnaryNode(node_pointer value_, const TokenTypes token_) noexcept : Node(Unary), value(std::move(value_)), token(token_) {}
	};

	/**
	 * @struct BinaryNode
	 * @brief defines a struct to carry binary operations
	 *
	 * Size: 32 bytes
	 *
	 * A Binary node is represented by a node, an binary operation, and another node.
	 *
	 * its syntax can be represented by:
	 * @code
	 * [left value]	[binary token] [right value]
	 * @endcode
	 *
	 * some examples of those operations are the:
	 * - artithimetic operations : `1 + 2`, `4 - 5`, `3 * 4`, ...
	 * - equality operations : `a > b`, `p != np`, `p == np`, ...
	 * - binary operations : `15 ^ 3`, `1 << 3`, `value & mask`, ...
	 *
	 * @todo implement `|`, `&`, `^`, `<<`, `>>`, and `<=>` (or any multi comparision operator)
	 */
	struct BinaryNode final : public Node
	{
		node_pointer left;		//!< left node value
		node_pointer right;		//!< right node value
		const TokenTypes token; //!< binary token

		/**
		 * @brief Construct a new Binary Node object
		 *
		 * @param left_ left node value
		 * @param token_ binary operation token
		 * @param right_ right node value
		 *
		 * Complexity: Constant
		 */
		BinaryNode(node_pointer left_, const TokenTypes token_, node_pointer right_) noexcept
			: Node(Binary), left(std::move(left_)), right(std::move(right_)), token(token_)
		{
		}
	};

	/**
	 * @struct TernaryNode
	 * @brief defines a struct to hold Ternary expressions
	 *
	 * Size: 32 bytes
	 *
	 * A ternary node is to a binary node as 3 is to 2. It holds 3 different node values, but with the
	 * relations already defined.
	 *
	 * as it can have 3 different syntaxes, its representations are as follows:
	 *
	 * @code
	 * [condition node] ? [true node] : [false node]
	 * @endcode
	 * or
	 * @code
	 * [true node] if [condtion node] else [false node]
	 * @endcode
	 * or even
	 * @code
	 * if [condition] then [true node] else [false node]
	 * @endcode
	 *
	 * the ternary nodes are what the language mainly uses for conditional flow
	 *
	 * @todo implement `... if ... else ...` and `if ... then ... else` syntax
	 */
	struct TernaryNode final : public Node
	{
		node_pointer condition; //!< conditional node
		node_pointer trueop;	//!< true node, node value in case @ref condition evaluates to true
		node_pointer falseop;	//!< false node, node value in case @ref condition evaluates to fasle

		/**
		 * @brief Construct a new Ternary Node object
		 *
		 * @param condition_ the ternary's condition node
		 * @param true_op the ternary's true case value
		 * @param false_op the ternary's false case value
		 *
		 * Complexity: Constant
		 */
		TernaryNode(node_pointer condition_, node_pointer true_op, node_pointer false_op) noexcept
			: Node(Ternary), condition(std::move(condition_)), trueop(std::move(true_op)), falseop(std::move(false_op))
		{
		}
	};

	/**
	 * @struct ExpressionNode
	 * @brief defines a struct to hold a complete expression
	 *
	 * Size: 32 bytes
	 *
	 * Expressions are the way the language has to define variables (or constants, depends on usage)
	 *
	 * expressions use only one syntax:
	 * @code
	 * [name] = [value]
	 * @endcode
	 *
	 * @todo implement "volatile" or similar, as registers can be assigned and may or may not change
	 * values by themselves
	 */
	struct ExpressionNode final : public Node
	{
		std::string_view name; //!< expression name
		node_pointer value;	   //!< expression value

		/**
		 * @brief Construct a new Expression Node object
		 *
		 * @param name_ expression name
		 * @param value_ expression value
		 */
		ExpressionNode(std::string_view name_, node_pointer value_) noexcept : Node(Expression), name(name_), value(std::move(value_))
		{
		}
	};

	/**
	 * @struct ListNode
	 * @brief a struct to carry lists of values
	 *
	 * Size: 32 bytes
	 *
	 * Lists are collections of similar data, they @b should be "constant", as they cannot have their
	 * size changed after they are created,
	 *
	 * their syntax is as follows:
	 * @code
	 * [] (an open bracket and a closing bracket)
	 * [ ... ] (the same but with any number of nodes)
	 * @endcode
	 *
	 * @todo actually make it usable
	 */
	struct ListNode final : public Node
	{
		std::vector<node_pointer> args; //!< list of nodes

		/**
		 * @brief Construct a new List Node object
		 *
		 * @param args_ list of elements in the list
		 *
		 * Complexity: Constant
		 */
		explicit ListNode(std::vector<node_pointer> args_) : Node(List), args(std::move(args_)) {}
	};

	/**
	 * @struct CallNode
	 * @brief a struct that holds calling and indexing
	 *
	 * Size: 40 bytes
	 *
	 * calling functions are one of the core things in a language that is wrapped around functions. this
	 * node also handles indexing, as it is basically the same as calling with one argument, although
	 * they have different syntaxes: @code # calling [value] ( ... )
	 *
	 * # indexing
	 * [value] [ [value] ]
	 * @endcode
	 *
	 * @note Calling nodes with no arguments just turns it into an identifier, and indexing nodes with
	 * no indexer throws an error
	 */

	struct CallNode final : public Node
	{
		std::vector<node_pointer> args; //!< call arguments / indexer
		node_pointer expr;				//!< expression to call / index

		/**
		 * Call) @brief Construct a new Call Node object
		 *
		 * @param expr_ the expression to call or to index
		 * @param args_ arguments to caller or index to indexer
		 * @param type_ either @a Call or @a Index, defaults to @a Call
		 *
		 * Complexity: Linear
		 */
		CallNode(node_pointer expr_, std::vector<node_pointer> args_, const NodeTypes type_ = Call) noexcept
			: Node(type_), args(std::move(args_)), expr(std::move(expr_))
		{
		}
	};

	/**
	 * @struct SwitchNode
	 * @brief a struct that holds a switch condition
	 *
	 * Size: 40 bytes
	 *
	 * a switch statement is a (sometimes) better way to express multiple conditions
	 *
	 * syntax representation (quite big this time):
	 * @code
	 * switch [expression node]
	 *  [case value] : [expression]
	 *   ...
	 *  default : [expression]
	 * end
	 * @endcode
	 * @note the spacing isnt needed, but the default case is
	 * @todo make the compiler smart enough to detect whether or not the default case is needed (e.g.:
	 * enums)
	 */
	struct SwitchNode final : public Node
	{
		std::vector<std::pair<node_pointer, node_pointer>> cases; //!< vector of cases
		node_pointer expr;										  //!< switch expression

		/**
		 * @brief Construct a new Switch Node object
		 *
		 * @param expr_ switch expression
		 * @param cases_ vector of cases
		 */
		SwitchNode(node_pointer expr_, std::vector<std::pair<node_pointer, node_pointer>> cases_) noexcept
			: Node(Switch), cases(std::move(cases_)), expr(std::move(expr_))
		{
		}
	};

	/**
	 * @struct LambdaNode
	 * @brief a struct that is responsible for functions
	 *
	 * Size: 56 bytes
	 *
	 * functions, on a functional language, kinda expected. Those structures are pieces of code that do
	 * something.
	 *
	 * syntax:
	 * @code
	 * [name] (args?...) => expression
	 * @endcode
	 *
	 * arguments are not necessary, but if a function is declared without them, it becomes an @ref
	 * ExpressionNode "expression node, with same name and value"
	 *
	 * @warning calling inlined functions might throw errors, they should be referenced instead
	 * @todo implement case where calling a lambda derived expression doesn't throw any errors
	 *
	 */
	struct LambdaNode final : public Node
	{
		std::vector<node_pointer> args; //!< function arguments
		std::string_view name;			//!< function name
		node_pointer expression;		//!< function expression

		/**
		 * args_) @brief Construct a new Lambda Node object
		 *
		 * @param name_ function name
		 * @param expression_ function expression
		 * @param args_ function args
		 *
		 * Complexity: Constant
		 */
		LambdaNode(std::string_view name_, node_pointer expression_, std::vector<node_pointer> args_) noexcept
			: Node(Lambda), args(std::move(args_)), name(name_), expression(std::move(expression_))
		{
		}
	};

	/**
	 * @struct DefineNode
	 * @brief a struct that holds type definitions
	 *
	 * Size: 32 bytes
	 *
	 * `DN`, in short, is what is (hopefully going to be) the main standout point of the whole language,
	 * as of now, it is discarted as there is no way to use it as intended. In the future, it should:
	 * - strongly type functions and variables: `identity_function : (x) -> x`
	 * - accept math expressions to reduce types: `naturals_up_to_10 : {x in N | x < 10}`
	 * - clone types: `irrationals := complex`
	 * - template types: `int_list := list<int>`
	 *
	 * syntax:
	 * @code
	 * [type_name] : [type_definition]
	 * [type_name] := [other_type]
	 * @endcode
	 *
	 * the concept of how types will actually be defined is yet to be
	 *
	 * @warning types should not use it yet as it just slows down compilation time
	 * @todo implement strong typing, math casting, clone typing and templates
	 */
	struct DefineNode final : public Node
	{
		node_pointer cast;	   //!< node that type is casted to
		std::string_view name; //!< type name

		/**
		 * @brief Construct a new Define Node object
		 *
		 * @param name_ type name
		 * @param cast_ type cast
		 *
		 * Complexity: Constant
		 */
		DefineNode(std::string_view name_, node_pointer cast_) noexcept : Node(Define), cast(std::move(cast_)), name(name_) {}
	};

	/**
	 * @struct IncludeNode
	 * @brief a struct that handles including other files
	 *
	 * Size: 32 bytes
	 *
	 * Includes are what let either other compilable files, or binary libraries (such as
	 * [glibc](https://www.gnu.org/software/libc/)) to be bundled together in your code
	 *
	 * syntax is, quite easy:
	 * @code
	 * include "file name"
	 * @endcode
	 *
	 * the compiler is going to be smart enough so that if any "shared library type" is used it is
	 * actually going to load them instead of trying to compile them
	 *
	 * @todo implement loading libraries and including files
	 */
	struct IncludeNode final : public Node
	{
		const std::string_view include_file; //!< the file name
		const bool isBinaryLibrary;			 //!< test to see if it is a binary or a normal file

		/**
		 * @brief Construct a new Include Node object
		 *
		 * @param fname file name
		 * @param isLib check if it is a binary library
		 *
		 * Complexity: Constant
		 */
		explicit IncludeNode(std::string_view fname, bool isLib = false) : Node(Include), include_file(fname), isBinaryLibrary(isLib)
		{
		}
	};

	/**
	 * @struct ScopeNode
	 * @brief a struct that holds nesting values
	 *
	 * Size: 24 bytes
	 *
	 * a good way to organize code is to put it in scopes, so that is what it is done for
	 *
	 * Syntax is very basic, just:
	 * @code
	 * [parent node].[child node]
	 * @endcode
	 *
	 * @todo implement scoping with higher precedence
	 */
	struct ScopeNode final : public Node
	{
		node_pointer parent; //!< node that holds the scope
		node_pointer child;	 //!< node that is going to be get from the scope

		/**
		 * @brief Construct a new Scope Node object
		 *
		 * @param parent_ parent node
		 * @param child_ child node
		 *
		 * Complexity: Constant
		 */
		ScopeNode(node_pointer parent_, node_pointer child_) : Node(Scope), parent(std::move(parent_)), child(std::move(child_)) {}
	};

	/**
	 * @struct TaskNode
	 * @brief Do multiple things inside a function
	 *
	 * Size: 32 bytes
	 *
	 */
	struct TaskNode final : public Node
	{
		//!< TODO: actually implement as a unordered multimap, so tasks don't
		//!< need to be synchronized and follow what the programmer wrote
		std::vector<node_pointer> tasks; //!< list of tasks

		/**
		 * @brief Construct a new Task Node object
		 *
		 * @param task_list list of nodes
		 *
		 * Complexity: Constant
		 */
		explicit TaskNode(std::vector<node_pointer> task_list) : Node(Task), tasks(std::move(task_list)) {}
	};

	////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 * @class Parser
	 * @brief The core of the AST generation
	 *
	 * The parser works amalgamated with the lexer/tokenizer, so instead of
	 * there being a pass for tokenization and another one for node creation,
	 * the parser requests a token with Parser::next, so at every
	 * point in time, there is only one token being parsed, which saves memory.
	 *
	 */
	class Parser final
	{
		/**
		 * @brief calls the lexer to get the next token
		 *
		 * all functions, when finished parsing, return at the @b following token, so for example
		 * @code
		 * ... abs(a + b) - 3
		 * #            ^------ current token
		 * @endcode
		 * suppose that the parser is parsing the function call, before returning, the function will call `next()` and stop at the
		 * next token, which is the `-` token, so:
		 * @code
		 * ... abs(a + b) - 3
		 * #              ^---- current token
		 * @endcode
		 */
		inline void next()
		{
			this->current_token = getNextToken(&this->lexer);
		}

		/**
		 * @brief parses a list of tokens in ortder
		 *
		 * @param end_token token to end the parser list
		 * @return std::vector<node_pointer> vector of nodes
		 */
		std::vector<node_pointer> List(TokenTypes end_token);

		/**
		 * @brief parse an array
		 *
		 * @return node_pointer
		 */
		node_pointer Array();

		/**
		 * @brief parse a number (all formats)
		 *
		 * @return node_pointer
		 */
		node_pointer Number();

		/**
		 * @brief parse a string/identifier, depends on @ref type
		 *
		 * @param type selector for either string or identifier
		 * @return node_pointer
		 */
		node_pointer String(uint8_t type);

		/**
		 * @brief parse a switch statement
		 *
		 * @return node_pointer
		 */
		node_pointer Switch();

		/**
		 * @brief parse a task statement
		 *
		 * @return node_pointer
		 */
		node_pointer Task();

		/**
		 * @brief parse an atom (the "smaller" nodes)
		 *
		 * @return node_pointer
		 */
		node_pointer Atom();

		/**
		 * @brief parse a prefix expression
		 *
		 * @return node_pointer
		 */
		node_pointer Prefix();

		/**
		 * @brief parse a postfix expression
		 *
		 * @return node_pointer
		 */
		node_pointer Postfix();

		/**
		 * @brief parse a math term expression
		 *
		 * @return node_pointer
		 */
		node_pointer Term();

		/**
		 * @brief parse a math factor
		 *
		 * @return node_pointer
		 */
		node_pointer Factor();

		/**
		 * @brief parse some comparisions
		 *
		 * @return node_pointer
		 */
		node_pointer Comparisions();

		/**
		 * @brief parse a ternary expression (basically if statements but smaller)
		 *
		 * @return node_pointer
		 */
		node_pointer Ternary();

		/**
		 * @brief parse names
		 *
		 * @return std::string_view
		 */
		std::string_view Name();

		/**
		 * @brief parse a lambda
		 *
		 * @param name lambda name
		 * @param arrowed check if lambda came from an arrowed expression
		 * @return node_pointer
		 */
		node_pointer Lambda(std::string_view name, bool arrowed);

		/**
		 * @brief parse a define statement
		 *
		 * @param name define name
		 * @return node_pointer
		 */
		node_pointer Define(std::string_view name);

		/**
		 * @brief parse all top level statements
		 *
		 * @return node_pointer
		 */
		node_pointer Assign();

	public:
		std::unordered_map<std::string_view, std::pair<Node *, int>>
			map{};				   //!< map of parsed top level nodes names, with reference count
		token_t current_token;	   //!< last token returned by the lexer
		pos_t current_position;	   //!< file position
		std::string_view filename; //!< current translating unit name
		lex_t lexer;			   //!< tokenizer object
		uint32_t file_size{0};	   //!< index limit

		/**
		 * @brief Construct a new Parser object
		 *
		 * @param name file name
		 */
		explicit Parser(std::string_view name);

		/**
		 * @brief parse a file
		 *
		 * @return std::vector<node_pointer>
		 */
		std::vector<node_pointer> File();

		/**
		 * @brief makes the compiler print a note to the output stream (which is probably stderr)
		 *
		 * @param note note title
		 * @param desc note description
		 */
		void Note(std::string_view note, std::string_view desc) const;

		/**
		 * @brief makes the compiler print a warning to the output stream (stderr)
		 *
		 * @param warn warn title
		 * @param desc warn description
		 */
		void Warning(std::string_view warn, std::string_view desc) const;

		/**
		 * @brief makes the compiler print an error message to the output stream (stderr) and throw
		 * a parse exception, which is catched by the \ref File "file function", and makes ends parsing
		 *
		 * @param error error title
		 * @param desc error description
		 */
		[[noreturn]] void Error(std::string_view error, std::string_view desc) const;
	};

	/**
	 * @brief returns a string representation of a token
	 *
	 * @param tok token value
	 * @return const char* the c-string representing that token
	 */
	const char *print_token(int tok);

} // namespace Parse


namespace VirtMac
{
	extern "C" {
		#include "virtualmachine.h"
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
	 * @enum instruction_status
	 * @brief defines status for specific instruction groups
	 *
	 */
	enum instruction_status
	{
		/**
		 * @brief not implemented category, when it is hardware @b and software impossibe to do a
		 * certain task, will make the compiler to either change what to do or raise an error if it
		 * can't implement
		 *
		 */
		not_implemented = 0,
		/**
		 * @brief category worked around, when it is a hardware impossible task, but there is software
		 * that can implement it, the compiler will try to not use it and instead unoptimize or even
		 * change code settings in order to get a implemented instruction(s).
		 *
		 * an example of this is floating point arithimetic on mcu's, which most of the times lead to
		 * compiler generated functions, which are good but use some good amount of space
		 *
		 * @note the compiler is not that smart yet, so it might just use it for now
		 */
		work_around = 1,
		/**
		 * @brief category implemented, when there is a way to implement that intrinsic with no need to
		 * specific functions and with at most 3 instructions per call.
		 */
		implemented = 2,
		/**
		 * @brief bare only category, when code can is hardware implemented, but can only be executed by
		 * bare metal, such as flag changing instructions @note most micro controlers can change flags
		 * normally, so instead of using this they use the @a implemented type.
		 */
		bare_only = 3,
		/**
		 * @brief used when there is hardware implementations, but they are somehow restricted, such as
		 * I/O operations, x86 which uses only dx/ax for operations for example.
		 *
		 */
		restricted_use = 4
	};

	/**
	 * @enum CompilerTypes
	 * @brief possible types for a compiler variable
	 *
	 */
	enum CompilerTypes : unsigned char
	{
		Immediate,			  //!< interger (or integer represented) numbers
		Register,			  //!< register index
		BasePointerImmediate, //!< pointer, `[imm]` format
		BasePointerRegister,  //!< pointer, `[r#]` format
	};

	/**
	 * @brief typing for a register, immediate or a pointer.
	 *
	 */
	using CompilerValue = uint64_t;

	/**
	 * @brief uses a string as a container to instructions
	 *
	 */
	using asm_container = std::stringstream;

	/**
	 * @brief actual instruction bytes tht should be put into running code
	 *
	 */
	using byte_container = std::vector<uint64_t>;

	/**
	 * @enum TargetCallingConvention
	 * @brief defines calling convention indexes for x86_64 (because every other architecture seems to
	 * have only one)
	 *
	 */
	enum TargetCallingConvention
	{
		/* x86 conventions */
		conv_cdecl,
		conv_syscall,
		conv_stdcall,
		conv_msfastcall,
		conv_msvectorcall,
		/* x86_64 conventions */
		conv_ms64call,
		conv_sysvabi,
		/* architecture defined convention */
		conv_arch_defined
	};

	/**
	 * @struct memory_region_t
	 * @brief defines a region in memory
	 *
	 */
	struct memory_region_t
	{

		/**
		 * @enum memory_region_types
		 * @brief define types of memory, used only with bare-metal targets
		 */
		enum memory_region_types
		{
			usable,	   //!< memory able to be used normally
			bank,	   //!< address is inside a bank
			shadow,	   //!< rom mapped area
			mapped,	   //!< hardware mapped area
			registers, //!< register map area
			data_area, //!< reclaimable area that might have data with it
			reserved,  //!< default unusable memory area
		};

		const uint64_t start;			//!< start of memory region
		const uint64_t end;				//!< end of memory region
		const memory_region_types type; //!< type of region

		/**
		 * @brief Construct a new memory region object
		 *
		 * memory regions are pretty useful as they let the compiler map what is where when running a bare-metal code
		 * but they also provide a way to limit runtime usage on "big targets"
		 *
		 * @param reg_start start of memory region
		 * @param reg_end end of memory region
		 * @param reg_type type of memory region
		 */
		memory_region_t(const uint64_t reg_start, const uint64_t reg_end, const memory_region_types reg_type) noexcept : start(reg_start),
																														 end(reg_end),
																														 type(reg_type)
		{
		}
	};

	/**
	 * @brief defines a target for the compiler
	 *
	 * @tparam _hw_size define hardware size [8, 16, 32, 64, 128, ...]
	 * @tparam _register_count register count in target
	 * @tparam _endianess big/little endian
	 * @tparam _stack_align alignment needed on stack
	 * @tparam _stack_depth hardware limited stack size
	 * @tparam _multithreaded hardware threads
	 * @tparam _memory_size expected memory size
	 * @tparam _calling_convention target abi calling conventions
	 */
	template <int _hw_size = 64, int _register_count = 32, bool _endianess = false, int _stack_align = 0, int _stack_depth = 0, bool _multithreaded = false,
			  int _memory_size = 32768, TargetCallingConvention _calling_convention = conv_arch_defined>
	struct ConceptTarget
	{
		static_assert(_hw_size >= 4, "serial and 2 bit computers are not supported");
		/**
		 * @brief says how big, in bits, the biggest value a register can carry without extensions
		 *
		 */
		using hardware_size = std::integral_constant<int, _hw_size>;

		/**
		 * @brief how many registers can the compiler use
		 * registers are always `r#` no matter the architecture (x86[_64] fanyboys gonna scream too)
		 */
		using register_count = std::integral_constant<int, _register_count>;

		/**
		 * @brief set when the target works with big endian values
		 */
		using endianness = std::bool_constant<_endianess>; // set when target is big endian

		/**
		 * @brief usually set together with a custom calling convention, marks (and says) how much does
		 * the compiler need to align between each call
		 */
		using stack_alignment = std::integral_constant<int, _stack_align>; //!< marked if target needs alignment on stack

		/**
		 * @brief unused in most "big" targets and some mcu's, when set, marks the amount of stack the controller has
		 * @note as far as I am aware, only targets with less than 16kb ram use this
		 */
		using stack_depth =
			std::integral_constant<int, _stack_depth>;

		using multithreaded = std::bool_constant<_multithreaded>; //!< marked for multithreaded targets
		using memory_size =
			std::integral_constant<int, _memory_size>; //!< used mainly by MCU's or low-level systems to set memory size
		/**
		 * @brief defines what calling convention the compiler should use, on bare-metal microarchitectures,
		 * the compiler will just define whatever fits
		 */
		using calling_convention = std::integral_constant<TargetCallingConvention, _calling_convention>;
	};

	/**
	 * @brief defines a type for all `assemble_(node)` functions
	 * 
	 */
	using return_t = std::vector<uint64_t>;

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
		enum register_status
		{
			clear = 0,		  //!< register has **for sure** 0x0000 on its value
			trashed = 1,	  //!< bogus values on register, might need to do a cleanup
			used = 2,		  //!< the current function is using this register
			callee_saved = 3, //!< current function cannot use this register for any kind of reason
			caller_saved = 4  //!< current function cannot use this register because the caller is using it
		};

		register_status registers[32] = {
			register_status::used, register_status::used
		}; //!< save status for all registers

		std::vector<Parse::node_pointer> &parsed_nodes;
		std::unordered_map<std::string_view, uint64_t> symbols; //!< symbols defined
		byte_container instructions; //!< container for all instructions
		uint64_t dot; //!< current instruction place in memory

		/**
		 * @brief request a register to be used, kind of a `malloc()` function
		 *
		 * Complexity: Linear, depends on which registers are used or not
		 * 
		 * @return index of register
		 */
		int request_register();

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
		 * @brief put a immediate into a register
		 *
		 * @param node number node
		 *
		 * 
		 * Complexity: Linear (depends on how much registers are used)
		 * 
		 * @return registers used while compiling
		 */
		return_t assemble_number(Parse::NumberNode *node);

		/**
		 * @brief assemble an unary node 
		 *
		 * @param node current unary node
		 * 
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
		return_t assemble_binary(Parse::BinaryNode * node, bool isJumping);

		/**
		 * @brief assemble a ternary node
		 * 
		 * @param node current ternary node
		 * 
		 * Comlpexity: Constant + child node complexities
		 * 
		 * @return registers used while compiling
		 */
		return_t assemble_ternary(Parse::TernaryNode * node);

		/**
		 * @brief assemble a function 
		 *
		 * @param node current lambda node
		 * 
		 * Complexity: Constant
		 * 
		 * @return registers used when compiling
		 * 
		 * @todo add used registers to symbol table
		 */
		return_t assemble_lambda(Parse::LambdaNode *node);

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
		 * @brief adds a function to the symbol table
		 * 
		 * @param name function name
		 *
		 * Complexity: Constant
		 * 
		 * @return current pointer location
		 */
		uint64_t add_function(std::string_view name);


		/**
		 * @brief construct the assembler
		 * 
		 * @param nodes node tree
		 * 
		 * Complexity: Constant
		 */
		Assembler(std::vector<Parse::node_pointer> &nodes) : parsed_nodes(nodes)
		{
		}

		/**
		 * @brief compiles all nodes
		 * 
		 * Complexity: Linear (depends on `this->parsed_nodes`' size)
		 * @return vector of machine instructions
		 */
		byte_container compile();
	};
} // namespace Compiler
