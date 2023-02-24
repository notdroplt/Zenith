/**
 * @file nodes.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief define compiler nodes
 * @version
 * @date 2023-02-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once
#ifndef ZENITH_NODES_H
#define ZENITH_NODES_H 1
#include "lex.h"
#include "types.h"
#include <stdbool.h>

/**
 * @enum NodeTypes
 * @brief enum for identifying Node types
 *
 * size: 1 byte
 */
enum NodeTypes
{
	Unused = 0,					/*!< used on Node's default constructor */
	Integer = TT_Int,			/*!< Integer Nodes */
	Double = TT_Double,			/*!< Double Nodes */
	String = TT_String,			/*!< String Nodes */
	Identifier = TT_Identifier, /*!< Identifier Nodes */
	Unary,						/*!< Unary operation nodes */
	Binary,						/*!< Binary operation nodes */
	Ternary,					/*!< Ternary operation nodes */
	Expression,					/*!< Top-level expression nodes */
	List,						/*!< List nodes */
	Call,						/*!< variable call nodes */
	Lambda,						/*!< Lambda Function Nodes */
	Switch,						/*!< Switch Nodes */
	Define,						/*!< Type definintion nodes */
	Index,						/*!< Index nodes */
	Include,					/*!< Include nodes */
	Scope,						/*!< Scope nodes */
	Task						/*!< Task nodes */
};

struct Node
{
	enum NodeTypes type; /*!< current node @ref NodeTypes "type" */
	bool isconst;		 /*!< defines if a node is const at parse-time */
};
/**
 * @brief constructs a default node
 *
 * A node constness is defined by the ability of the compiler to reduce it at compile-time,
 * while it is little, it helps a lot the interpreter as there are less nodes to interpret.
 * but that doesn't stop the parser to kick in more implementations
 *
 * @param type current node type
 * @param isconst current node constness
 *
 * Complexity: depends on malloc
 */
struct Node create_node(const enum NodeTypes type, const bool isconst);

/**
 * @typedef node_pointer
 * @brief a unique pointer to Node
 */
typedef struct Node *node_pointer;

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
struct NumberNode
{
	struct Node base; /*!< base node struct */
	uint64_t number;  /*!< integer number value */
	double value;	  /*!< decimal number value */
};

/**
 * @brief Constructs a number node, from an integer
 *
 * @param value node value
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_intnode(const uint64_t value);

/**
 * @brief Constructs a number node, from a double
 *
 * @param value_ node value
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_doublenode(const double value);

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
struct StringNode
{
	struct Node base;	   /*!< base node */
	struct string_t value; //!< string value
};

/**
 * @brief Construct a new String Node object, either for @a strings or @a identifiers
 *
 * @param value string value
 * @param type node type, either @a string or @a identifier
 *
 * Complexity: depends on `malloc`
 * string_view's default copy constructor only does a shallow copy of the string
 */
node_pointer create_stringnode(struct string_t value, const enum NodeTypes type);

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
 * accomplishes nothing and is always just discarted
 *
 * @note this node benefits from the node
 * constness, as it can be inlined and even discarted at compile time if the node value is constant
 *
 * @todo implement `--`, `++` and `~` unary operations
 */
struct UnaryNode
{
	struct Node base;	   //!< base node
	node_pointer value;	   //!< main node value
	enum TokenTypes token; //!< unary operation token
};

/**
 * @brief Construct a new Unary Node object
 *
 * @param value unary node's value
 * @param token unary operation token
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_unarynode(const node_pointer value, const enum TokenTypes token);

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
struct BinaryNode
{
	struct Node base;	   //!< base node
	node_pointer left;	   //!< left node value
	node_pointer right;	   //!< right node value
	enum TokenTypes token; //!< binary token
};

/**
 * @brief Construct a new Binary Node object
 *
 * @param left left node value
 * @param token binary operation token
 * @param right right node value
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_binarynode(node_pointer left, const enum TokenTypes token, node_pointer right);

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
struct TernaryNode
{
	struct Node base;
	node_pointer condition; //!< conditional node
	node_pointer trueop;	//!< true node, node value in case @ref condition evaluates to true
	node_pointer falseop;	//!< false node, node value in case @ref condition evaluates to fasle
};

/**
 * @brief Construct a new Ternary Node object
 *
 * @param condition the ternary's condition node
 * @param true_op the ternary's true case value
 * @param false_op the ternary's false case value
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_ternarynode(node_pointer condition, node_pointer true_op, node_pointer false_op);

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
struct ExpressionNode
{
	struct Node base;
	struct string_t name; //!< expression name
	node_pointer value;	  //!< expression value
};

/**
 * @brief Construct a new Expression Node object
 *
 * @param name expression name
 * @param value expression value
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_expressionnode(struct string_t name, node_pointer value);

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
 * [ ... ] (the same but with any number of nodes between them)
 * @endcode
 *
 * @todo actually make it usable
 */
struct ListNode
{
	struct Node base;
	struct Array *nodes; // array of node pointers
};

/**
 * @brief Construct a new List Node object
 *
 * @param [in] args_ list of elements in the list
 * @param node_count amount of nodes pointed by `args_`
 * Complexity: depends on `malloc`
 */
node_pointer create_listnode(struct Array *node_array);

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

struct CallNode
{
	struct Node base;
	struct Array *arguments; //!< array of arguments
	node_pointer expr;		 //!< expression to call / index
};

/**
 * @brief Construct a new Call Node object
 *
 * @param [in] expr the expression to call or to index
 * @param [in] arg_array arguments to caller or index to indexer
 * @param type either @a Call or @a Index, defaults to @a Call
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_callnode(node_pointer expr, struct Array *arg_array, const enum NodeTypes type);

/**
 * @struct SwitchNode
 * @brief a struct that holds a switch condition
 *
 * Size: 40 bytes
 *
 * a switch expression is a (sometimes) better way to express multiple conditions
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
struct SwitchNode
{
	struct Node base;
	struct Array *cases; //!< array of cases
	node_pointer expr;	 //!< switch expression
};

/**
 * @brief Construct a new Switch Node object
 *
 * @param expr switch expression
 * @param cases vector of cases
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_switchnode(node_pointer expr, struct Array *cases);

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
struct LambdaNode
{
	struct Node base;
	struct Array *params;	 //!< function parameters
	struct string_t name;	 //!< function name
	node_pointer expression; //!< function expression
};

/**
 * @brief Construct a new Lambda Node object
 *
 * @param name function name
 * @param expression function expression
 * @param params_arr function args
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_lambdanode(struct string_t name, node_pointer expression, struct Array *params_arr);

/**
 * @struct DefineNode
 * @brief a struct that holds domain definitions
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
 * [type_name] : [domain definition]
 * [type_name] := [other domain]
 * @endcode
 *
 * the concept of how types will actually be defined is yet to be implemented
 *
 * @warning types should not use it yet as it just slows down compilation time
 * @todo implement strong typing, math casting, clone typing and templates
 */
struct DefineNode
{
	struct Node base;
	node_pointer cast;	  //!< node that type is casted to
	struct string_t name; //!< type name
};

/**
 * @brief Construct a new Define Node object
 *
 * @param name type name
 * @param cast type cast
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_definenode(struct string_t name, node_pointer cast);

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
struct IncludeNode
{
	struct Node base;
	struct string_t fname; //!< the file name
	bool binary;		   //!< test to see if it is a binary or a normal file
};

/**
 * @brief Construct a new Include Node object
 *
 * @param fname file name
 * @param isLib check if it is a binary library
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_includenode(struct string_t fname, bool binary);

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
struct ScopeNode
{
	struct Node base;
	node_pointer parent; //!< node that holds the scope
	node_pointer child;	 //!< node that is going to be get from the scope
};

/**
 * @brief Construct a new Scope Node object
 *
 * @param parent parent node
 * @param child child node
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_scopenode(node_pointer parent, node_pointer child);

/**
 * @struct TaskNode
 * @brief Do multiple things inside a function
 *
 * Size: 32 bytes
 *
 */
struct TaskNode
{
	struct Node base;
	//!< TODO: actually implement as a unordered multimap, so tasks don't
	//!< need to be synchronized and follow what the programmer wrote
	struct Array *tasks; //!< tasks to do
};

/**
 * @brief Construct a new Task Node object
 *
 * @param task_list list of nodes
 *
 * Complexity: depends on `malloc`
 */
node_pointer create_tasknode(struct Array *array);

/**
 * @brief deletes any kind of node
 *
 * @param node
 */
void delete_node(struct Node *node);

#endif