#pragma once
#include "lexer.hh"
#include "utils.hh"
#include <concepts>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace zenith::parser
{

    /**
     * @brief define possible node identificators
     */
    enum NodeID
    {
        NodeEmpty,
        /** An integer node */
        NodeInt,
        /** A double node */
        NodeDouble,
        /** A node with a string*/
        NodeString,
        /** A node with an identifier*/
        NodeIdentifier,
        /** nodes with 1 argument */
        NodeUnary,
        /** nodes with 2 arguments */
        NodeBinary,
        /** nodes with 3 arguments */
        NodeTernary,
        /** switch case nodes */
        NodeSwitch,
        /** the function but not yet assigned a name */
        NodeLambda,
        /** node with values that wont't change others, but can be changed*/
        NodeVariable,
        /** indexer node */
        NodeIndex,
        /** child and parent node */
        NodeParent,
        /** structured data nodes, an union is also assigned this*/
        NodeStruct,
        /** any general type*/
        NodeType,
        /** a single input that generates an output (without a name) */
        NodeFunctor,
        /** combines multiple functors into a typed piece (with possibly a name) */
        NodeFunction,
        /** named variable for type axioms */
        NodeForall,
        /** constraint inside a type axiom */
        NodeConstraint
    };

    /**
     * @brief declare possible attributes for nodes
     */
    enum NodeFlags
    {
        None = 0x00,
        /** the compiler cannot write to this byte (or memory range) */
        ReadOnly = 0x01,
        /** the compiler cannot read to this byte (or memory range) */
        WriteOnly = 0x02,
        /** this variable (or memory range) might change value after declared */
        Mutable = 0x04,
        /** this variable (or memory range) shadows a register */
        Register = 0x08,
        /** this variable is mapped to a specific place in memory */
        MemoryMapped = 0x10,
        /** this variable (or memory range) might receive values without notice */
        Input = 0x20,
        /** this variable changes the outside world when its value is changed */
        Output = 0x40,
    };

    /**
     * @brief restrict template types to only when U is derived from T
     */
    template <typename T, typename U>
    concept Derived = std::is_base_of_v<U, T>;

    /**
     * @brief Represent a node in a tree
     *
     * a basic node only has information about its node type(a binary operation),
     * its value type (an integer) and some flags
     *
     */
    struct Node
    {
    private:
        /** node type */
        void *m_type{nullptr};

        /** node id */
        NodeID m_nid{NodeID::NodeEmpty};

        /** node flags */
        NodeFlags m_flags{NodeFlags::None};

    public:
        /**
         * @brief Default Node constructor
         *
         * @param id node id
         * @param type node type
         * @param flags flags
         */
        constexpr explicit Node(
            const NodeID nid,
            void *type = nullptr,
            NodeFlags flags = None) : m_type{type}, m_nid{nid}, m_flags{flags} {}

        /**
         * @brief Default node constructor
         *
         * @note deleted because all nodes need
         * at least their types to be defined
         */
        constexpr Node() = delete;

        /**
         * @brief Default node copy constructor
         *
         * @param node node to copy from
         */
        constexpr Node(const Node &node) = default;

        /**
         * @brief Default node move constructor
         *
         * @param node node to move from
         */
        constexpr Node(Node &&node) = default;

        /**
         * @brief Default node copy assignment operator
         *
         * @param node node to copy from
         *
         * @return this reference
         */
        ZENITH_CONST_NODISCARD_CONSTEXPR Node &operator=(const Node &node) = default;

        /**
         * @brief Default node move assignment operator
         *
         * @param node node to move from
         * @return this reference
         */
        ZENITH_CONST_NODISCARD_CONSTEXPR Node &operator=(Node &&) = default;

        /**
         * @brief get node type
         *
         * @returns a node type
         */
        ZENITH_CONST_NODISCARD_CONSTEXPR auto type() const noexcept { return this->m_type; }

        /**
         * @brief get the node id
         *
         * @return a \ref NodeID "node id"
         */
        ZENITH_CONST_NODISCARD_CONSTEXPR NodeID nid() const noexcept { return this->m_nid; }

        /**
         * @brief get the node's flags
         *
         * @return one or more \ref NodeFlags "node flags"
         */
        ZENITH_CONST_NODISCARD_CONSTEXPR auto flags() { return this->m_flags; }

        /**
         * @brief compare two basic node
         *
         * @param left left hand side
         * @param right right hand side
         * @return std::strong_ordering
         */
        ZENITH_CONST_NODISCARD_CONSTEXPR friend auto operator<=>(const Node &left, const Node &right)
        {
            if (auto cmp = left.m_nid <=> right.m_nid; cmp != std::strong_ordering::equal)
            {
                return cmp;
            }
            if (auto cmp = left.m_flags <=> right.m_flags; cmp != std::strong_ordering::equal)
            {
                return cmp;
            }
            return left.m_type <=> right.m_type;
        }

        /**
         * @brief Get a node as any of its subtypes
         *
         * @tparam T Type derived from node
         * @return this pointer if type matches, or nullptr if it doesnt
         */
        template <Derived<Node> T>
        [[nodiscard]] constexpr const T *as() const noexcept
        {
            return reinterpret_cast<const T *>(this);
        }

        virtual ~Node() = default;
    };

    template <typename T>
    constexpr bool is_intval_v = std::is_same_v<T, uint64_t> ||
                                 std::is_same_v<T, int64_t> ||
                                 std::is_same_v<T, double>;

    using nodeptr = std::unique_ptr<Node>;
    template <typename T>
    concept intval_member = is_intval_v<T>;

    struct NumberNode final : public Node
    {
    public:
        /**
         * @brief define a variant for possible node values
         */
        using intval_t = std::variant<uint64_t, int64_t, double>;

    private:
        intval_t _value{0UL};

    public:
        constexpr NumberNode() : Node{NodeInt}, _value{0UL} {}
        constexpr NumberNode(const NumberNode &) = default;
        constexpr NumberNode(NumberNode &&) = default;
        constexpr NumberNode &operator=(const NumberNode &) = default;
        constexpr NumberNode &operator=(NumberNode &&) = default;

        explicit constexpr NumberNode(const uint64_t val) : Node{NodeInt}, _value{val} {}
        explicit constexpr NumberNode(const int64_t val) : Node{NodeInt}, _value{val} {}
        explicit constexpr NumberNode(const double val) : Node{NodeDouble}, _value{val} {}

        template <intval_member T>
        constexpr auto has() { return std::holds_alternative<T>(this->_value); }

        template <intval_member T>
        constexpr auto value() { return std::get<T>(this->_value); }

        ~NumberNode() final = default;
    };

    struct StringNode final : public Node
    {
    private:
        std::string_view _string{};

    public:
        constexpr explicit StringNode(
            const std::string_view view = {},
            const NodeID type = NodeString) : Node{type}, _string{view} {}

        constexpr StringNode() : Node{NodeString}, _string{} {}
        constexpr StringNode(const StringNode &) = default;
        constexpr StringNode(StringNode &&) = default;
        constexpr StringNode &operator=(const StringNode &) = default;
        constexpr StringNode &operator=(StringNode &&) = default;

        constexpr auto string() { return this->_string; }

        ~StringNode() final = default;
    };

    struct UnaryNode final : public Node
    {
    private:
        nodeptr _operand = nullptr;
        lexer::Tokens _operation = lexer::Tokens::TokenPlus;

    public:
        constexpr UnaryNode(
            lexer::Tokens operation,
            nodeptr operand) : Node{NodeUnary}, _operand{std::move(operand)}, _operation{operation} {}

        constexpr UnaryNode() : Node{NodeUnary} {}
        constexpr UnaryNode(const UnaryNode &) = delete;
        constexpr UnaryNode(UnaryNode &&) = default;
        constexpr UnaryNode &operator=(const UnaryNode &) = delete;
        constexpr UnaryNode &operator=(UnaryNode &&) = default;

        constexpr auto &operand() { return this->_operand; }
        constexpr auto operation() { return this->_operation; }

        ~UnaryNode() final = default;
    };

    struct BinaryNode final : public Node
    {
    private:
        nodeptr _left{nullptr};
        nodeptr _right{nullptr};
        lexer::Tokens _operation = lexer::Tokens::TokenPlus;

    public:
        constexpr BinaryNode(
            nodeptr left,
            const lexer::Tokens operation,
            nodeptr right) : Node{NodeBinary}, _left{std::move(left)}, _right{std::move(right)}, _operation{operation}
        {
        }

        constexpr BinaryNode() : Node{NodeBinary} {}
        constexpr BinaryNode(const BinaryNode &) = delete;
        constexpr BinaryNode(BinaryNode &&) = default;
        constexpr BinaryNode &operator=(const BinaryNode &) = delete;
        constexpr BinaryNode &operator=(BinaryNode &&) = default;

        constexpr auto &left() { return this->_left; }
        constexpr auto &right() { return this->_right; }
        constexpr auto &operation() { return this->_operation; }

        ~BinaryNode() final = default;
    };

    struct TernaryNode final : public Node
    {
    private:
        nodeptr _condition{nullptr};
        nodeptr _on_true{nullptr};
        nodeptr _on_false{nullptr};

    public:
        constexpr TernaryNode(
            nodeptr condition,
            nodeptr on_true,
            nodeptr on_false) : Node{NodeTernary}, _condition{std::move(condition)}, _on_true{std::move(on_true)}, _on_false{std::move(on_false)} {}

        constexpr TernaryNode() : Node{NodeTernary} {}
        constexpr TernaryNode(const TernaryNode &) = delete;
        constexpr TernaryNode(TernaryNode &&) = default;
        constexpr TernaryNode &operator=(const TernaryNode &) = delete;
        constexpr TernaryNode &operator=(TernaryNode &&) = default;

        ZENITH_CONST_NODISCARD_CONSTEXPR nodeptr &condition() noexcept
        {
            return this->_condition;
        }

        ZENITH_CONST_NODISCARD_CONSTEXPR nodeptr &on_true() noexcept
        {
            return this->_on_true;
        }

        ZENITH_CONST_NODISCARD_CONSTEXPR nodeptr &on_false() noexcept
        {
            return this->_on_false;
        }

        ~TernaryNode() final = default;
    };

    struct LambdaNode final : public Node
    {
    private:
        nodeptr _name{nullptr};
        std::vector<nodeptr> _args{};
        nodeptr _expr{nullptr};

    public:
        constexpr LambdaNode(nodeptr name, std::vector<nodeptr> args, nodeptr expr) : Node{NodeLambda}, _name{std::move(name)}, _args{std::move(args)}, _expr{std::move(expr)} {}

        constexpr LambdaNode() : Node{NodeLambda} {}
        constexpr LambdaNode(const LambdaNode &) = delete;
        constexpr LambdaNode(LambdaNode &&) = default;
        constexpr LambdaNode &operator=(const LambdaNode &) = delete;
        constexpr LambdaNode &operator=(LambdaNode &&) = default;

        ZENITH_CONST_NODISCARD_CONSTEXPR nodeptr &name() noexcept
        {
            return this->_name;
        }

        ZENITH_CONST_NODISCARD_CONSTEXPR std::vector<nodeptr> &args() noexcept
        {
            return this->_args;
        }

        ZENITH_CONST_NODISCARD_CONSTEXPR nodeptr &expr() noexcept
        {
            return this->_expr;
        }

        ~LambdaNode() final = default;
    };

    struct TypeNode final : public Node
    {
    private:
        nodeptr name;
        nodeptr superset;
        nodeptr clause;
    };

    using lexer_return = lexer::lexer_return;
    using Tokens = lexer::Tokens;
    using Lexer = lexer::Lexer;

    class Parser final
    {
    private:
        std::string_view m_content;
        lexer_return m_current;
        Lexer m_lexer;

    public:
        explicit Parser(std::string_view content) noexcept;
        Parser(std::string_view content, lexer_return current, Lexer lexer) noexcept : m_content{content}, m_current{current}, m_lexer{lexer} {}

        ZENITH_CONST_NODISCARD_CONSTEXPR auto content() const noexcept
        {
            return this->m_content;
        }

        ZENITH_CONST_NODISCARD_CONSTEXPR auto current() const noexcept
        {
            return this->m_current;
        }

        ZENITH_CONST_NODISCARD_CONSTEXPR auto lexer() const noexcept
        {
            return this->m_lexer;
        }
    };

    std::pair<Parser, std::expected<nodeptr, zenith::errors::Errors>> next_node(Parser) noexcept;

}; // namespace zenith::parser
