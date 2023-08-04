#include "parser.hpp"
#include <fstream>
#include <iostream>
using namespace Parsing;


Parser::Parser(const char *filename) : _lex("")
{
    assert(filename != NULL);

    this->_fname = filename;

    FILE * fp = fopen(filename, "r");

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    auto buf = new char[size + 1];
    fread(buf, size, 1, fp);

    buf[size] = '\0';

    this->_buf = std::string(buf);
    delete[] buf;
    fclose(fp);

    this->_lex = Lexer(std::string_view(this->_buf));

    this->next();
}

void Parser::next() noexcept
{
    this->_tok = this->_lex.next_token();
}

returns<nodep> Parser::_number() noexcept
{
    if (!this->_tok)
    {
        return make_unexpected<returns<nodep>>("syntax", "invalid token");
    }

    if (this->_tok->type == TT_Int)
    {
        return make_expected_unique<returns<nodep>, NumberNode>(this->_tok->get<uint64_t>());
    }

    return make_expected_unique<returns<nodep>, NumberNode>(this->_tok->get<double>());
}

returns<nodep> Parser::_string(const enum TokenTypes id) noexcept
{
    if (!this->_tok)
    {
        return make_unexpected<returns<nodep>>("syntax", "invalid token");
    }
    auto token = *this->_tok;

    this->next();

    return make_expected_unique<returns<nodep>, StringNode>(token.get<std::string_view>(), id);
}

returns<nodep> Parser::_comma(const enum TokenTypes end_token) noexcept
{
    std::vector<nodep> nodes;

    while (this->_tok && this->_tok->type != end_token)
    {
        auto expression = this->_top_level();
        if (!expression)
            return expression;
        nodes.emplace_back(std::move(expression.value()));

        if (!this->_tok || this->_tok->type != TT_Comma)
        {
            break;
        }

        this->next();
    }

    return make_expected_unique<returns<nodep>, ListNode>(std::move(nodes));
}

returns<nodep> Parser::_array() noexcept
{
    this->next();

    auto array = this->_comma(TT_RightSquareBracket);

    if (!array)
        return array;

    if (this->_tok->type != TT_RightSquareBracket)
    {
        return make_unexpected<returns<nodep>>("syntax", "expected ']' after a comma to initalizate an array");
    }

    this->next();

    return array;
}

returns<nodep> Parser::_switch() noexcept
{
    std::vector<std::pair<nodep, nodep>> cases;
    returns<nodep> case_expr;
    returns<nodep> case_return;

    this->next();

    if (!this->_tok)
    {
        return make_unexpected<returns<nodep>>("syntax", "expected a switch expresion");
    }

    auto switch_expr = this->_top_level();
    if (!switch_expr)
        return switch_expr;

    while (this->_tok->type != TT_Keyword && this->_tok->get<KeywordTypes>() != KW_end)
    {
        if (!this->_tok)
        {
            return make_unexpected<returns<nodep>>("syntax", "\"switch\" was incomplete before EOF");
        }

        if (this->_tok->type != TT_Colon || !(this->_tok->is<KeywordTypes>() && this->_tok->get<KeywordTypes>() == KW_default))
        {
            return make_unexpected<returns<nodep>>("syntax", "expected ':' or \"default\" as a case expression starter");
        }

        if (this->_tok->type == TT_Colon)
        {
            this->next();

            case_expr = this->_top_level();
            if (!case_expr)
                return case_expr;

            this->next();
        }
        else
        {
            this->next();
            case_expr.value() = NULL;
        }

        if (this->_tok->type != TT_Equal)
        {
            return make_unexpected<returns<nodep>>("syntax", "expected '=' after case expression");
        }

        this->next();

        case_return = this->_top_level();
        if (!case_return)
            return case_return;

        cases.push_back(std::make_pair(std::move(*case_expr), std::move(*case_return)));
    }

    if (!this->_tok || this->_tok->get<KeywordTypes>() != KW_end)
    {
        return make_unexpected<returns<nodep>>("syntax", "expected 'end' to close switch expression");
    }

    return make_expected_unique<returns<nodep>, SwitchNode>(std::move(*switch_expr), std::move(cases));
}

returns<nodep> Parser::_task() noexcept
{
    return make_unexpected<returns<nodep>>("syntax", "tasks are yet to be implemented");
}

returns<nodep> Parser::_atom() noexcept
{
    returns<nodep> node;
    if (!this->_tok)
    {
        return make_unexpected<returns<nodep>>("syntax", "premature end of expression");
    }

    switch (this->_tok->type)
    {
    case TT_Int:
    case TT_Double:
        return this->_number();
    case TT_String:
    case TT_Identifier:
        return this->_string(this->_tok->type);
    case TT_Keyword:
        if (this->_tok->get<KeywordTypes>() == KW_switch)
        {
            return this->_switch();
        }
        else if (this->_tok->get<KeywordTypes>() == KW_do)
        {
            return this->_task();
        }
        return make_unexpected<returns<nodep>>("syntax", "unexpected keyword");
    case TT_LeftParentesis:
        this->next();

        node = this->_top_level();
        if (!node)
            return node;

        if (this->_tok->type == TT_Comma)
        {
            this->next();
            std::vector<nodep> arglistv;
            auto args = this->_comma(TT_RightParentesis);
            if (!args)
                return args;
            if (!this->_tok || this->_tok->type != TT_RightParentesis)
                return make_unexpected<returns<nodep>>("syntax", "expected ')'");

            auto &arglist = (*args)->as<ListNode>().nodes();
            arglistv.push_back(std::move(*node));

            for (auto &e : arglist)
                arglistv.push_back(nodep(const_cast<nodep *>(&e)->release()));
            

            this->next();

            return make_expected_unique<returns<nodep>, ListNode>(std::move(arglistv));
        }

        if (!this->_tok || this->_tok->type != TT_RightParentesis)
        {
            return make_unexpected<returns<nodep>>("syntax", "expected ')'");
        }

        this->next();

        return node;
    case TT_LeftSquareBracket:
        this->next();
        return this->_array();
    default:
        return make_unexpected<returns<nodep>>("syntax", "unknown atom case");
    }
}

returns<nodep> Parser::_prefix() noexcept
{
    if (!this->_tok)
        return this->_atom();

    auto type = this->_tok->type;
    returns<nodep> node;

    if (type == TT_Plus)
    {
        this->next();
        return this->_atom();
    }

    if (type != TT_Minus || type != TT_Not)
        return this->_atom();

    node = this->_top_level();

    if (!node)
        return node;

    return make_expected_unique<returns<nodep>, UnaryNode>(type, std::move(*node));
}

returns<nodep> Parser::_postfix() noexcept
{
    auto node = this->_prefix();
    if (!node)
        return node;

    if (this->_tok->type == TT_Dot)
    {
        this->next();
        returns<nodep> child = this->_postfix();
        if (!child)
            return child;

        return make_expected_unique<returns<nodep>, ScopeNode>(*node, *child);
    }

    if (this->_tok->type == TT_LeftSquareBracket)
    {
        this->next();
        returns<nodep> index = this->_top_level();
        if (!index)
            return index;
        if (!this->_tok)
            return make_unexpected<returns<nodep>>("syntax", "expected indexable expression");
        if (this->_tok->type == TT_RightSquareBracket)
            return make_unexpected<returns<nodep>>("syntax", "expected a ']' to close an indexable expression");

        this->next();

        return make_expected_unique<returns<nodep>, CallNode>(std::move(*node), std::move(*index), Index);
    }

    if (this->_tok->type == TT_LeftParentesis)
    {
        this->next();

        auto arguments = this->_comma(TT_RightParentesis);
        if (!arguments)
            return arguments;

        if (!this->_tok || this->_tok->type != TT_RightParentesis)
        {
            return make_unexpected<returns<nodep>>("syntax", "expected a ')' after function call");
        }
        this->next();

        return make_expected_unique<returns<nodep>, CallNode>(std::move(*node), std::move(*arguments), Call);
    }

    return node;
}

returns<nodep> Parser::_term() noexcept
{
    auto lside = this->_postfix();
    if (!this->_tok || !lside)
        return lside;

    auto &type = this->_tok->type;
    while (this->_tok && (type == TT_Multiply || type == TT_Divide))
    {
        this->next();
        auto rside = this->_postfix();
        if (!rside)
            return rside;

        if ((*lside)->type() == String || (*rside)->type() == String)
        {
            return make_unexpected<returns<nodep>>("syntax", "cannot '*' or '/' a string literal");
        }

        lside = std::make_unique<BinaryNode>(std::move(*lside), type, std::move(*rside));
    }

    return lside;
}

returns<nodep> Parser::_factor() noexcept
{
    auto lside = this->_term();
    if (!this->_tok || !lside)
        return lside;

    auto &type = this->_tok->type;
    while (this->_tok && (type == TT_Plus || type == TT_Minus))
    {
        this->next();
        auto rside = this->_term();
        if (!rside)
            return rside;

        if ((*lside)->type() == String || (*rside)->type() == String)
        {
            return make_unexpected<returns<nodep>>("syntax", "cannot '+' or '-' a string literal");
        }

        lside = std::make_unique<BinaryNode>(std::move(*lside), type, std::move(*rside));
    }

    return lside;
}

returns<nodep> Parser::_comparisions() noexcept
{
    auto lside = this->_factor();
    if (!this->_tok || !lside)
        return lside;

    auto &type = this->_tok->type;
    while (this->_tok && type >= TT_LessThan && type <= TT_GreaterThanEqual)
    {
        this->next();
        auto rside = this->_factor();
        if (!rside)
            return rside;

        if ((*lside)->type() == String || (*rside)->type() == String)
        {
            return make_unexpected<returns<nodep>>("syntax", "cannot '==', '!=', '>', '>=', '<', or '<=' a string literal (yet)");
        }

        lside = std::make_unique<BinaryNode>(std::move(*lside), type, std::move(*rside));
    }

    return lside;
}

returns<nodep> Parser::_top_level() noexcept
{
    auto left_side = this->_comparisions();

    if (!left_side || !this->_tok)
        return left_side;

    if (this->_tok->type != TT_Arrow && this->_tok->type != TT_Question)
        return left_side;

    if (this->_tok->type == TT_Arrow)
    {
        this->next();

        auto arg_type = (*left_side)->type();

        if (arg_type != Identifier && arg_type != List)
            return make_unexpected<returns<nodep>>("syntax", "expected argument(s) to be either an identifier or a list of identifiers");

        if (!this->_tok)
            return make_unexpected<returns<nodep>>("syntax", "expected lambda body after '=>'");

        auto body = this->_comparisions();
        if (!body)
            return body;

        return make_expected_unique<returns<nodep>, LambdaNode>(std::move(*left_side), std::move(*body));
    }

    this->next();

    if (!this->_tok)
        return make_unexpected<returns<nodep>>("syntax", "expected expression after '?'");

    auto on_true = this->_comparisions();
    if (!on_true)
        return on_true;

    if (this->_tok->type != TT_Colon)
        return make_unexpected<returns<nodep>>("syntax", "expected ':' after expression");

    this->next();

    auto on_false = this->_comparisions();
    if (!on_false)
        return on_false;

    return make_expected_unique<returns<nodep>, TernaryNode>(std::move(*left_side), std::move(*on_true), std::move(*on_false));
}

returns<nodep> Parser::_statement() noexcept
{
    if (!this->_tok)
        return make_unexpected<returns<nodep>>("syntax", "expected either 'include' or name as a top level statement");

    if (this->_tok->is<KeywordTypes>() && this->_tok->get<KeywordTypes>() == KW_include)
    {
        auto file = this->_string(TT_String);
        if (!file)
            return file;
        return make_expected_unique<returns<nodep>, IncludeNode>((*file)->as<StringNode>().value());
    }

    if (!this->_tok || this->_tok->type != TT_Identifier)
        return make_unexpected<returns<nodep>>("syntax", "current token cannot be a name");

    auto name = this->_tok->get<std::string_view>();
    this->next();

    if (this->_tok || this->_tok->type == TT_Arrow) {
        auto body = this->_comparisions();

        return make_expected_unique<returns<nodep>, LambdaNode>(nullptr, std::move(*body));
    }

    if (!this->_tok || this->_tok->type != TT_Equal)
        return make_unexpected<returns<nodep>>("syntax", "expected '=' after name");

    this->next();

    auto value = this->_top_level();

    if (!value)
        return value;

    return make_expected_unique<returns<nodep>, ExpressionNode>(name, std::move(*value));
}

std::optional<std::vector<nodep>> Parser::parse() {
    std::vector<nodep> nodes;

    while (this->_tok){
        auto node = this->_statement();
        if (!node) {
            std::cout << node.error().to_string(this->_fname) << std::endl;
            return std::nullopt;
        }

        nodes.push_back(std::move(*node));
    }

    return nodes;
}