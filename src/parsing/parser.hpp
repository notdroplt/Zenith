/**
 * @file parser.hpp
 * @author droplt
 * @brief parse files
 * @version 0.1
 * @date 2023-09-29
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once
#ifndef ZENITH_PARSER_HPP
#define ZENITH_PARSER_HPP 1

#include "../platform.hpp"
#include "../typing/typing.hpp"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>
#include <stack>

/**
 * @enum TokenID
 * @brief All possible token types
 *
 * size : 1 byte
 */
enum TokenID
{
    TT_Unknown,            /*!< used as the default token in case of errors */
    TT_Int,                /*!< integer numbers */
    TT_Double,             /*!< decimal numbers */
    TT_String,             /*!< character strings */
    TT_Identifier,         /*!< unmanaged names */
    TT_Keyword,            /*!< language-specific names */
    TT_Domain,             /*!< language domains */
    TT_Plus,               /*!< "+" */
    TT_Increment,          /*!< "++" */
    TT_Minus,              /*!< "-" */
    TT_Decrement,          /*!< "--" */
    TT_Multiply,           /*!< "*" */
    TT_Divide,             /*!< "/" */
    TT_LessThan,           /*!< "<" */
    TT_LessThanEqual,      /*!< "<=" */
    TT_LeftShift,          /*!< "<<" */
    TT_RightShift,         /*!< ">>" */
    TT_CompareEqual,       /*!< "==" */
    TT_NotEqual,           /*!< "!=" */
    TT_GreaterThan,        /*!< ">" */
    TT_GreaterThanEqual,   /*!< ">=" */
    TT_Question,           /*!< "?" */
    TT_Colon,              /*!< ":" */
    TT_TypeDefine,         /*!< "::" */
    TT_Equal,              /*!< "=" */
    TT_Not,                /*!< "!" */
    TT_LeftParentesis,     /*!< "(" */
    TT_RightParentesis,    /*!< ")" */
    TT_LeftSquareBracket,  /*!< "[" */
    TT_RightSquareBracket, /*!< "]" */
    TT_LeftCurlyBracket,   /*!< "{" */
    TT_RightCurlyBracket,  /*!< "}" */
    TT_Arrow,              /*!< "=>" */
    TT_Dot,                /*!< "." */
    TT_Comma,              /*!< "," */
    TT_Semicolon,          /*!< ";" */
    TT_At,                 /*!< "@" */
    TT_BitwiseOr,          /*!< "|" */
    TT_BinaryOr,           /*!< "||" */
    TT_BitwiseAnd,         /*!< "&" */
    TT_BinaryAnd,          /*!< "&&" */
    TT_BitWiseXor,         /*!< "^" */
    TT_Tilda               /*!< "~" */

};

/**
 * @enum KeywordTypes
 * @brief Saves space by deallocating space for keywords
 *
 *
 * size: 1 byte
 */
enum KeywordTypes
{
    KW_Unknown, /*!< error code for keywords */

    KW_var,      /*!< "var" */
    KW_function, /*!< "function" */
    KW_as,       /*!< "as" */
    KW_do,       /*!< "do" */
    KW_switch,   /*!< "switch" */
    KW_default,  /*!< "default" */
    KW_if,       /*!< "if" */
    KW_then,     /*!< "if" */
    KW_else,     /*!< "else" */
    KW_end,      /*!< "end" */
    KW_return,   /*!< "return" */
    KW_import,   /*!< "import" */
    KW_type,     /*!< "type" */
    KW_define,   /*!< "define" */
    KW_is,       /*!< "is" */
    KW_struct
};

/**
 * @enum DomainTypes
 * @brief type for domain definitions
 *
 * size: 1 byte
 */
enum DomainTypes
{
    DM_unknown, /*!< error code for domains*/

    DM_byte,  /*!< 1 byte:  N ∩ [0, 2⁸)  */
    DM_hword, /*!< 2 bytes: N ∩ [0, 2¹⁶) */
    DM_word,  /*!< 4 bytes: N ∩ [0, 2³²) */
    DM_dword, /*!< 8 bytes: N ∩ [0, 2⁶⁴) */

    DM_char,  /*!< 1 byte: N ∩ [-2⁷, 2⁷) */
    DM_short, /*!< 2 bytes: N ∩ [-2¹⁵, 2¹⁵) */
    DM_int,   /*!< 4 bytes: N ∩ [-2³¹, 2³¹) */
    DM_long   /*!< 8 bytes: N ∩ [-2⁶³, 2⁶³) */

};

/**
 * @enum ActionKeywords
 * @brief commands that run on the compiler and not on the compiled code
 *
 * size: 1 byte
 */
enum ActionKeywords
{
    Act_define,     /*!< defines an expression into the compiler map*/
    Act_if,         /*!< conditionally parse next section if the condition is true */
    Act_defined,    /*!< checks if a name is defined */
    Act_elif,       /*!< conditionally parse if a conditional action above fails */
    Act_else,       /*!< conditionally parse if all other actions before fail */
    Act_endif,      /*!< ends a conditional parse block */
    Act_deprecated, /*!< mark a block as deprecated */
    Act_note,       /*!< send a note to a frontend on the next block's use */
    Act_warn,       /*!< send a warn to a frontend on the next block's use */
    Act_error,      /*!< send an error to a frontend on the next block's use, possibly terminating the program */
};

struct Token
{
    //!< @brief implement a variant for all possible tokens
    //!< - an integer
    //!< - a decimal
    //!< - a keyword
    //!< - a string
    std::variant<uint64_t, double, enum KeywordTypes, std::string_view> val;

    //!< start of token
    pos_t start;

    //!< end of token
    pos_t end;

    //!< token id
    enum TokenID id;

    /**
     * @brief Construct a token as normal token
     * @param type token id
     */
    constexpr Token(enum TokenID id, pos_t start = pos_t{}, pos_t end = pos_t{}) : start(start), end(end), id(id) {}

    /**
     * @brief Construct a token as an Integer
     * @param integer integer value
     */
    constexpr Token(uint64_t integer, pos_t start = pos_t{}, pos_t end = pos_t{}) : Token(TT_Int, start, end) { this->val = integer; }

    /**
     * @brief Construct a token as a Double
     * @param decimal decimal value
     */
    constexpr Token(double decimal, pos_t start = pos_t{}, pos_t end = pos_t{}) : Token(TT_Double, start, end) { this->val = decimal; }

    /**
     * @brief Construct a token as a String
     * @param string string value
     */
    constexpr Token(std::string_view string, pos_t start = pos_t{}, pos_t end = pos_t{}) : Token(TT_String, start, end) { this->val = string; }

    /**
     * @brief Get a token value as a desired type
     * @tparam T type included on the tuple
     * @returns value of type in the tuple
     */
    template <typename T>
    constexpr auto get() const { return std::get<T>(val); }

    constexpr std::partial_ordering operator<=>(const Token &other) const
    {
        if (auto cmp = this->id <=> other.id; cmp != 0)
            return cmp;
        return this->val <=> other.val;
    }

    /**
     * @brief Check if token is a type
     *
     * @returns true if types are equal
     * @returns false if types are different
     */
    template <typename T>
    constexpr bool is() const noexcept { return std::holds_alternative<T>(this->val); }
};

namespace Parsing
{
    /**
     * @brief Generate tokens on a file
     */
    class Lexer
    {
    private:
        //!< position on the current file
        struct pos_t _pos
        {
            0, 0, 1, 1
        };

        //!< full content on file
        std::string_view _content;

        //!< current char on file
        char _char;

        //!< tokenize a new token
        char next();

        /**
         * @brief try to tokenize a string on the file
         *
         * @returns Token(TT_String) in case of a successful token
         * @returns a nullopt in case of an error while tokenizing
         */
        returns<Token> string();

        /**
         * @brief try to tokenize a number on the file
         *
         * @returns Token(TT_Integer) in case of a successful integer token
         * @returns Token(TT_Double) in case of a successful decmial token
         * @returns nullopt in case of any error while tokenizing
         */
        returns<Token> number();

        returns<Token> token(const enum TokenID);
        returns<Token> identifier();
        returns<Token> compare(char c, TokenID type);
        returns<Token> equal();
        returns<Token> donot();

        returns<Token> repeat(char c, TokenID type);

    public:
        Lexer(std::string_view content) : _content(content), _char(content[0]){};
        static uint64_t strtoi(std::string_view str);
        static double strtod(std::string_view str);
        returns<Token> next_token();
    };

    /**
     * @enum NodeID
     * @brief enum for identifying Nodes
     *
     * size: 1 byte
     */
    enum NodeID
    {
        Unused = 0, /*!< used on Node's default constructor */
        Integer,    /*!< Integer Nodes */
        Double,     /*!< Double Nodes */
        String,     /*!< String Nodes */
        Identifier, /*!< Identifier Nodes */
        Unary,      /*!< Unary operation nodes */
        Binary,     /*!< Binary operation nodes */
        Ternary,    /*!< Ternary operation nodes */
        Expression, /*!< Top-level expression nodes */
        List,       /*!< List nodes */
        Call,       /*!< variable call nodes */
        Lambda,     /*!< Lambda Function Nodes */
        Switch,     /*!< Switch Nodes */
        Index,      /*!< Index nodes */
        Include,    /*!< Include nodes */
        Scope,      /*!< Scope nodes */
        Task,       /*!< Task nodes */
        Domain,     /*!< Domain nodes */
    };

    /**
     * @brief A representation of any node in the syntax tree
     *
     * A node has the following components
     * - a node id to identify derived nodes without RTTI
     * - a node type for later optimizations and type checking
     * - a node flag to check when the current node is a constant
     * - a start position in the file, for error formating
     * - an end position in the file, for error formating
     */
    struct Node
    {
    protected:
        const enum NodeID _id;        //!< Node identificator
        const Typing::Type _nodetype; //!< Node type
        const bool _const = false;    //!< Node const flag
        const pos_t _start = pos_t{}; //!< Node start position
        const pos_t _end = pos_t{};   //< Node end position

    public:
        /**
         * @brief construct a base node
         *
         * @param id node identificator
         * @param node_type node type
         * @param is_const node const flag
         * @param start start position
         * @param end end position
         */
        Node(const enum NodeID id, Typing::Type node_type, const bool is_const = false, const pos_t start = pos_t{}, const pos_t end = pos_t{})
            : _id(id), _nodetype(std::move(node_type)), _const(is_const), _start(start), _end(end) {}

        /**
         * @brief compare two nodes
         *
         * nodes are the same independently of the position they have in the syntax tree
         *
         * @param other node to compare
         * @returns true when nodes are equal
         * @returns false otherwise
         */
        virtual constexpr bool operator==(const Node &other) const
        {
            return this->_id == other._id && this->_const == other._const && this->_nodetype == other._nodetype;
        };

        /**
         * @brief check if node against only its ID
         *
         *
         * @param id id to check
         * @returns true if IDs are equal
         * @returns false otherwise
         */
        constexpr bool operator==(const NodeID id) const
        {
            return this->_id == id;
        };

        /**
         * @brief type getter
         *
         * @return node id
         */
        constexpr auto type() const { return this->_id; }

        /**
         * @brief const flag getter
         *
         * @returns true if node is constant
         * @returns false if not
         */
        constexpr auto constant() const { return this->_const; }

        /**
         * @brief node type getter
         *
         * @returns node type
         */
        constexpr auto &node_type() const { return this->_nodetype; }

        /**
         * @brief start position getter
         *
         * @returns pos_t start position
         */
        constexpr auto &start() const { return this->_start; }

        /**
         * @brief
         */
        auto &end() const { return this->_end; }

        /**
         * @brief cast a node into any of its derived classes
         *
         * @tparam DerivedNode derived class to cast
         * @return current node as
         */
        template <typename DerivedNode>
            requires(std::is_base_of_v<Node, DerivedNode>)
        constexpr auto &as() const noexcept
        {
            return *static_cast<const DerivedNode *>(this);
        }

        constexpr bool is(NodeID id) { return this->_id == id; }

        virtual ~Node() = default;
    };

    using nodep = std::unique_ptr<Node>;

    struct NumberNode final : public Node
    {
    protected:
        std::variant<int64_t, uint64_t, double> _val;

    public:
        NumberNode(const uint64_t value) : Node(Integer, make_unique_variant<Typing::Type, Typing::RangeType>(value), true), _val(value) {}
        NumberNode(const int64_t value) : Node(Integer, make_unique_variant<Typing::Type, Typing::RangeType>(value), true), _val(value) {}
        NumberNode(const double value) : Node(Double, make_unique_variant<Typing::Type, Typing::RangeType>(value), true), _val(value) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) && this->_val == other.as<NumberNode>()._val;
        }

        template <typename T>
        constexpr std::optional<T> value() const noexcept
        {
            auto ptr = std::get_if<T>(&this->_val);
            return ptr ? std::optional(*ptr) : std::nullopt;
        }

        virtual ~NumberNode() = default;
    };

    struct StringNode final : public Node
    {
    protected:
        std::string_view _value;
        bool _is_identifier;

    public:
        StringNode(std::string_view str, const TokenID id) : Node(id == TT_Identifier ? Identifier : String, nullptr, true), _value(str), _is_identifier(id == TT_Identifier) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) && this->_is_identifier == other.as<StringNode>()._is_identifier && this->_value == other.as<StringNode>()._value;
        }

        constexpr auto &value() const noexcept { return this->_value; }
        constexpr auto &identifier() const noexcept { return this->_is_identifier; }

        virtual ~StringNode() = default;
    };

    struct UnaryNode final : public Node
    {
    protected:
        const nodep _node;
        const enum TokenID _op;

    public:
        UnaryNode(const enum TokenID token, nodep value) : Node(Unary, nullptr, value->constant()),
                                                           _node(std::move(value)), _op(token) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) &&
                   this->_op == other.as<UnaryNode>()._op &&
                   this->_node == other.as<UnaryNode>()._node;
        }

        constexpr auto &node() const { return this->_node; }
        constexpr auto &op() const { return this->_op; }

        virtual ~UnaryNode() = default;
    };

    struct BinaryNode final : public Node
    {
    protected:
        const nodep _left;
        const nodep _right;
        const enum TokenID _op;

    public:
        BinaryNode(nodep left, const enum TokenID op, nodep right) : Node(Binary, nullptr, left->constant() && right->constant()),
                                                                     _left(std::move(left)), _right(std::move(right)), _op(op) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) &&
                   this->_op == other.as<BinaryNode>()._op &&
                   this->_left == other.as<BinaryNode>()._left &&
                   this->_right == other.as<BinaryNode>()._right;
        }

        constexpr auto &left() const { return this->_left; }
        constexpr auto &right() const { return this->_right; }
        constexpr auto &op() const { return this->_op; }

        virtual ~BinaryNode() = default;
    };

    struct TernaryNode final : public Node
    {
    protected:
        const nodep _cond;
        const nodep _ontrue;
        const nodep _onfalse;

    public:
        TernaryNode(nodep cond, nodep ontrue, nodep onfalse) : Node(Ternary, nullptr, cond->constant()),
                                                               _cond(std::move(cond)), _ontrue(std::move(ontrue)), _onfalse(std::move(onfalse)) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) &&
                   this->_cond == other.as<TernaryNode>()._cond &&
                   this->_ontrue == other.as<TernaryNode>()._ontrue &&
                   this->_onfalse == other.as<TernaryNode>()._onfalse;
        }

        constexpr auto &cond() const { return this->_cond; }
        constexpr auto &ontrue() const { return this->_ontrue; }
        constexpr auto &onfalse() const { return this->_onfalse; }

        virtual ~TernaryNode() = default;
    };

    struct ExpressionNode final : public Node
    {
    protected:
        const nodep _expr;
        const std::string_view _name;

    public:
        ExpressionNode(const std::string_view name, nodep expression) : Node(Expression, nullptr, expression->constant()),
                                                                        _expr(std::move(expression)), _name(name) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) &&
                   this->_name == other.as<ExpressionNode>()._name &&
                   this->_expr == other.as<ExpressionNode>()._expr;
        }

        constexpr auto &name() const { return this->_name; }
        constexpr auto &expr() const { return this->_expr; }

        virtual ~ExpressionNode() = default;
    };

    struct ListNode final : public Node
    {
    protected:
        const std::vector<nodep> _nodes;

    public:
        ListNode(std::vector<nodep> array) : Node(List, nullptr, false),
                                             _nodes(std::move(array)) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) && std::equal(this->_nodes.begin(), this->_nodes.end(), other.as<ListNode>()._nodes.begin());
        }

        constexpr auto &nodes() const { return this->_nodes; }

        virtual ~ListNode() = default;
    };

    struct CallNode final : public Node
    {
    protected:
        const nodep _args;
        const nodep _expr;
        const bool _index;

    public:
        CallNode(nodep expr, nodep args, NodeID index) : Node(Call, nullptr, false),
                                                         _args(std::move(args)), _expr(std::move(expr)), _index(index == Index) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) &&
                   *this->_expr == *other.as<CallNode>()._expr &&
                   *this->_args == *other.as<CallNode>()._args;
        }

        constexpr auto &args() const { return this->_args; }
        constexpr auto &expr() const { return this->_expr; }
        constexpr auto is_index() const { return this->_index; }
        virtual ~CallNode() = default;
    };

    struct SwitchNode final : public Node
    {
    protected:
        const std::vector<std::pair<nodep, nodep>> _cases;
        const nodep _expr;

    public:
        SwitchNode(nodep expr, std::vector<std::pair<nodep, nodep>> cases) : Node(Switch, nullptr, expr->constant()),
                                                                             _cases(std::move(cases)), _expr(std::move(expr)) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) &&
                   *this->_expr == *other.as<SwitchNode>()._expr &&
                   std::equal(this->_cases.begin(), this->_cases.end(), other.as<SwitchNode>()._cases.begin());
        }

        constexpr auto &cases() const { return this->_cases; }
        constexpr auto &expr() const { return this->_expr; }

        virtual ~SwitchNode() = default;
    };

    struct LambdaNode final : public Node
    {
    protected:
        const nodep _args;
        const nodep _expr;

    public:
        LambdaNode(nodep args, nodep expr) : Node(Lambda, nullptr, false), _args(std::move(args)), _expr(std::move(expr)) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) &&
                   *this->_expr == *other.as<LambdaNode>()._expr &&
                   *this->_args == *other.as<LambdaNode>()._args;
        }

        constexpr auto &args() const { return this->_args; }
        constexpr auto &expr() const { return this->_expr; }

        virtual ~LambdaNode() = default;
    };

    struct IncludeNode final : public Node
    {
    protected:
        const std::string_view _name;

    public:
        IncludeNode(std::string_view name) : Node(Include, nullptr, false), _name(name) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) && this->_name == other.as<IncludeNode>()._name;
        }

        constexpr auto &name() const { return this->_name; }

        virtual ~IncludeNode() = default;
    };

    struct ScopeNode final : public Node
    {
    protected:
        const nodep _parent;
        const nodep _child;

    public:
        ScopeNode(nodep &parent, nodep &child) : Node(Scope, nullptr, parent->constant()),
                                                 _parent(std::move(parent)), _child(std::move(child)) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) &&
                   *this->_parent == *other.as<ScopeNode>()._parent &&
                   *this->_child == *other.as<ScopeNode>()._child;
        }

        constexpr auto &parent() const { return this->_parent; }

        virtual ~ScopeNode() = default;
    };

    struct TaskNode final : public Node
    {
    protected:
        const std::vector<nodep> _tasks;

    public:
        TaskNode(std::vector<nodep> &tasks) : Node(Task, nullptr, false),
                                              _tasks(std::move(tasks)) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) && std::equal(this->_tasks.begin(), this->_tasks.end(), other.as<TaskNode>()._tasks.begin());
        }

        constexpr auto &tasks() const { return this->_tasks; }

        virtual ~TaskNode() = default;
    };

    struct TypeNode final : public Node
    {
    public:
        enum class Alternatives
        {
            unknown,
            domain,
            array,
            pointer,
            structure,
            function,
            interval
        };

    protected:
        const std::string_view _name;
        const enum Alternatives _meta = Alternatives::unknown;
        struct ArrayType
        {
            std::unique_ptr<TypeNode> _type = nullptr;
            nodep _size = nullptr;
        };

        using StructType = std::vector<std::unique_ptr<TypeNode>>;

        const struct FunctionType
        {
            std::vector<std::unique_ptr<TypeNode>> arguments = {};
            std::unique_ptr<TypeNode> return_type = nullptr;
        } _function;

        using PointerType = std::unique_ptr<TypeNode>;

        struct IntervalType
        {
            using Delimiter = Typing::RangeType::Delimiter;
            nodep start = nullptr;
            nodep end = nullptr;
            bool closed_started = true;
            bool close_ended = false;
        };

        using Types = std::variant<ArrayType, StructType, FunctionType, PointerType, IntervalType>;

        Types _type;

    public:
        TypeNode(const std::string_view name, std::unique_ptr<TypeNode> &type, nodep &size)
            : Node(Domain, nullptr, true), _name(name), _meta(Alternatives::array), _type(ArrayType{std::move(type), std::move(size)})
        {
        }

        TypeNode(const std::string_view name, std::vector<std::unique_ptr<TypeNode>> &structure)
            : Node(Domain, nullptr, true), _name(name), _meta(Alternatives::structure), _type(std::move(structure))
        {
        }

        TypeNode(const std::string_view name, nodep range_start, nodep range_end, bool close_start, bool close_end)
            : Node(Domain, nullptr, true), _name(name), _type(IntervalType{std::move(range_start), std::move(range_end), close_start, close_end})
        {
        }

        virtual ~TypeNode() = default;
    };

    class Parser
    {
        returns<Token> _tok = TT_Unknown; //!< current parsed token
        std::unordered_map<std::string, Typing::Type &> types;
        std::stack<pos_t> start_positions;
        Lexer _lex; //!< tokenizer instance

        /// TODO: make this buffer not destroy itself after parsing is complete
        ///       or make it so strings are actually references on other tables
        std::string _buf;   //!< file buffer
        std::string _fname; //!< name of file to translate

        inline void next() noexcept;

        returns<nodep> _comma(const enum TokenID end_token, const enum TokenID delim) noexcept;
        returns<nodep> _number() noexcept;
        returns<nodep> _string(const enum TokenID id) noexcept;
        returns<nodep> _array() noexcept;
        returns<nodep> _switch() noexcept;
        returns<nodep> _task() noexcept;
        returns<nodep> _atom() noexcept;
        returns<nodep> _prefix() noexcept;
        returns<nodep> _postfix() noexcept;
        returns<nodep> _term() noexcept;
        returns<nodep> _factor() noexcept;
        returns<nodep> _comparisions() noexcept;
        returns<nodep> _type(const std::string_view name) noexcept;
        returns<nodep> _top_level() noexcept;
        returns<nodep> _statement() noexcept;
        void _generate_types();

    public:
        Parser(const char *filename);

        /**
         * @brief translate an entire unit at once,
         *
         * @return a vector of (unoptimized) nodes if no error was found
         * @return std::nullopt in case an error was thrown
         */
        std::optional<std::vector<nodep>> parse();
    };
} // namespace Parsing

#endif