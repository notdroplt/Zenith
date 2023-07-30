#pragma once
#ifndef ZENITH_PARSER_HPP
#define ZENITH_PARSER_HPP 1

#include "platform.h"
#include <types/types.hpp>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

/**
 * @enum TokenTypes
 * @brief All possible token types
 *
 * size : 1 byte
 */
enum TokenTypes
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
    KW_include,  /*!< "include" */
    KW_type,     /*!< "type" */
    KW_define,   /*!< "define" */
    KW_is        /*!< "is" */
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

    enum TokenTypes type; /*!< specify token type */

    /**
     * @brief Construct a token as normal token
     *
     */
    constexpr Token(enum TokenTypes type) : type(type)
    {
    }

    /**
     * @brief Construct a token as an Integer
     * @param integer
     */
    constexpr Token(uint64_t integer) : Token(TT_Int)
    {
        this->val = integer;
    }

    constexpr Token(double decimal) : Token(TT_Double)
    {
        this->val = decimal;
    }

    constexpr Token(std::string_view string) : Token(TT_String)
    {
        this->val = string;
    }

    template <typename T>
    constexpr auto get() const
    {
        return std::get<T>(val);
    }

    template <typename T>
    constexpr bool is() const noexcept
    {
        return std::get_if<T>(&this->val) != nullptr;
    }
};

namespace Parsing
{

    class Lexer
    {
    private:
        struct pos_t _pos
        {
            0, 0, 0, 0
        };
        std::string_view _content;
        char _char;

        char next();
        std::optional<Token> string();
        std::optional<Token> number();
        std::optional<Token> token(const enum TokenTypes);
        std::optional<Token> identifier();
        std::optional<Token> compare(char c, TokenTypes type);
        std::optional<Token> equal();
        std::optional<Token> donot();

        std::optional<Token> repeat(char c, TokenTypes type);

    public:
        Lexer(std::string_view content) : _content(content), _char(content[0]){};
        static uint64_t strtoi(std::string_view str);
        static double strtod(std::string_view str);
        std::optional<Token> next_token();
    };

    /**
     * @enum NodeTypes
     * @brief enum for identifying Node types
     *
     * size: 1 byte
     */
    enum NodeTypes
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
        Type,       /*!< type nodes */
    };

    struct Node
    {
    protected:
        const enum NodeTypes _type;
        const bool _const = false;

    public:
        using basetype = Node;
        constexpr Node(const enum NodeTypes type = Unused, const bool is_const = false) : _type(type), _const(is_const) {}

        virtual constexpr bool operator==(const Node &other) const
        {
            return this->_type == other._type && this->_const == other._const;
        };

        auto type() const { return this->_type; }
        auto constant() const { return this->_const; }

        template <typename T>
        constexpr auto &as() const
        {
            static_assert(std::is_base_of_v<Node, T>, "Expected <T> to be derived from node");
            return *static_cast<const T *>(this);
        }

        virtual ~Node() = default;
    };

    using nodep = std::unique_ptr<Node>;

    struct NumberNode final : public Node
    {
    protected:
        static_assert(sizeof(uint64_t) == sizeof(double), "expected <int64_t> to be same size as <dobule>");
        std::variant<uint64_t, double> _val;

    public:
        constexpr NumberNode(const uint64_t value) : Node(Integer, true), _val(value) {}
        constexpr NumberNode(const double value) : Node(Double, true), _val(value) {}

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

    //!< TODO: add identifier
    struct StringNode final : public Node
    {
    protected:
        std::string_view _value;
        bool _is_identifier;

    public:
        constexpr StringNode(std::string_view str, const TokenTypes id) : Node(id == TT_Identifier ? Identifier : String, true), _value(str), _is_identifier(id == TT_Identifier) {}

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
        const enum TokenTypes _op;

    public:
        constexpr UnaryNode(const enum TokenTypes token, nodep value) : Node(Unary, value->constant()),
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
        const enum TokenTypes _op;

    public:
        constexpr BinaryNode(nodep left, const enum TokenTypes op, nodep right) : Node(Binary, left->constant() && right->constant()),
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
        constexpr TernaryNode(nodep cond, nodep ontrue, nodep onfalse) : Node(Ternary, cond->constant()),
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
        constexpr ExpressionNode(const std::string_view name, nodep expression) : Node(Expression, expression->constant()),
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
        ListNode(std::vector<nodep> array) : Node(List, false),
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
        constexpr CallNode(nodep expr, nodep args, NodeTypes index) : Node(Call, false),
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
        constexpr SwitchNode(nodep expr, std::vector<std::pair<nodep, nodep>> cases) : Node(Switch, expr->constant()),
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
        constexpr LambdaNode(nodep args, nodep expr) : Node(Lambda, false), _args(std::move(args)), _expr(std::move(expr)) {}

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
        constexpr IncludeNode(std::string_view name) : Node(Include, false), _name(name) {}

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
        constexpr ScopeNode(nodep &parent, nodep &child) : Node(Scope, parent->constant()),
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
        constexpr TaskNode(std::vector<nodep> &tasks) : Node(Task, false),
                                                        _tasks(std::move(tasks)) {}

        virtual constexpr bool operator==(const Node &other) const noexcept override
        {
            return this->Node::operator==(other) && std::equal(this->_tasks.begin(), this->_tasks.end(), other.as<TaskNode>()._tasks.begin());
        }

        constexpr auto &tasks() const { return this->_tasks; }

        virtual ~TaskNode() = default;
    };

    // struct TypeNode final : public Node
    // {
    // public:
    //     enum class Alternatives
    //     {
    //         unknown,
    //         domain,
    //         array,
    //         pointer,
    //         structure,
    //         function,
    //         interval
    //     };
    // protected:
    //     const std::string_view _name;
    //     const enum Alternatives _meta = Alternatives::unknown;
    //     const enum DomainTypes _domain = DM_unknown;
    //     const struct array_tType
    //     {
    //         std::unique_ptr<TypeNode> _type = nullptr;
    //         uint64_t _size = 0;
    //         constexpr ArrayType() = default;
    //         constexpr ArrayType(std::unique_ptr<TypeNode> &type, uint64_t size) :
    //             _type(std::move(type)), _size(size) {}
    //     } _array;
    //     const std::vector<std::unique_ptr<TypeNode>> _struct = {};
    //     const struct FunctionType
    //     {
    //         std::vector<std::unique_ptr<TypeNode>> arguments = {};
    //         std::unique_ptr<TypeNode> return_type = nullptr;
    //     } _function;
    //     const std::unique_ptr<TypeNode> _ptr = nullptr;
    //     const struct IntervalTypes
    //     {
    //         double start = 0;
    //         double end = 0;
    //     } _interval;

    // public:
    //     constexpr TypeNode(const std::string_view name, const enum DomainTypes domain) :
    //         Node(Type, true), _name(name), _meta(Alternatives::domain), _domain(domain) {}

    //     constexpr TypeNode(const std::string_view name, std::unique_ptr<TypeNode> &type, uint64_t size) :
    //         Node(Type, true), _name(name), _meta(Alternatives::array), _array(type, size) {}

    //     constexpr TypeNode(const std::string_view name, std::vector<std::unique_ptr<TypeNode>> &structure) :
    //         Node(Type, true), _name(name), _meta(Alternatives::structure), _struct(std::move(structure)) {};

    //     constexpr auto &domain() { return this->_domain; }
    //     constexpr auto &array() { return this->_array; }
    //     constexpr auto &structure() { return this->_struct; }
    //     constexpr auto &interval() { return this->_interval; }
    //     virtual ~TypeNode() = default;
    // };

    class Parser
    {
        std::optional<Token> _tok; //!< current parsed token
        struct pos_t _pos;         //!< current cursor position
        struct Lexer _lex;         //!< tokenizer instance

        /// TODO: make this buffer not destroy after parser is complete
        ///       or make it so strings are actually references to other tables
        std::string _buf;   //!< file buffer
        std::string _fname; //!< name of file to translate

        inline void next() noexcept;
        returns<nodep> _comma(const enum TokenTypes end_token) noexcept;
        returns<nodep> _number() noexcept;
        returns<nodep> _string(const enum TokenTypes id) noexcept;
        returns<nodep> _array() noexcept;
        returns<nodep> _switch() noexcept;
        returns<nodep> _task() noexcept;
        returns<nodep> _atom() noexcept;
        returns<nodep> _prefix() noexcept;
        returns<nodep> _postfix() noexcept;
        returns<nodep> _term() noexcept;
        returns<nodep> _factor() noexcept;
        returns<nodep> _comparisions() noexcept;
        returns<nodep> _top_level() noexcept;
        returns<nodep> _statement() noexcept;

    public:
        Parser(const char *filename);

        /**
         * @brief translate an entire unit at once,
         *
         * @return a vector of (unoptimized) nodes if no error was found
         * @return std::nullopt in case an error was thrown
         */
        std::optional<std::vector<nodep>> parse();

        std::optional<nodep> parse_threaded();
    };
} // namespace Parsing

#endif