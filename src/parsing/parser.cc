#include "parser.hpp"
#include <fstream>
#include <limits>

#include <iostream>
using namespace Parsing;
using namespace Typing;

Parser::Parser(const char *filename) : _lex("")
{
    ///TODO: remake this mess
    FILE *fp = fopen(filename, "r");
    if (!fp) return;
    this->_fname = filename;
    
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    auto buf = new char[size + 2];
    
    if (!buf) {
        fclose(fp);
        return;
    }

    fread(buf, size, 1, fp);

    buf[size] = '\n';
    buf[size + 1] = '\0';

    this->_buf = std::string(buf);
    delete[] buf;
    fclose(fp);

    this->_lex = Lexer(std::string_view(this->_buf));

    this->next();
    this->_generate_types();

    init_status = true;
}

void Parser::_generate_types()
{
    // this->types.insert({
    // {"bool", make_unique_variant<Type, RangeType>(std::numeric_limits<bool>::max())},
    // {"char", make_unique_variant<Type, RangeType>(std::numeric_limits<char>::min(), std::numeric_limits<char>::max())},
    // {"s8",  make_unique_variant<Type, RangeType>(std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())},
    // {"u8",  make_unique_variant<Type, RangeType>(std::numeric_limits<uint8_t>::max())},
    // {"s16", make_unique_variant<Type, RangeType>(std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())},
    // {"s32", make_unique_variant<Type, RangeType>(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max())},
    // {"s64", make_unique_variant<Type, RangeType>(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max())},
    // {"u16", make_unique_variant<Type, RangeType>(std::numeric_limits<uint16_t>::max())},
    // {"u32", make_unique_variant<Type, RangeType>(std::numeric_limits<uint32_t>::max())},
    // {"u64", make_unique_variant<Type, RangeType>(std::numeric_limits<uint64_t>::max())},
    // {"flt", make_unique_variant<Type, RangeType>(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max())},
    // {"dbl", make_unique_variant<Type, RangeType>(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max())}

    // });
}

void Parser::next() noexcept
{
    this->_tok = this->_lex.next_token();
}

returns<nodep> Parser::_number() noexcept
{
    return this->_tok.transform([this](Token tok){
        if (tok.id == TT_Int) {
            auto val = tok.get<uint64_t>();

            this->next();

            return std::make_unique<NumberNode>(val);
        }

        auto val = this->_tok->get<double>();

        this->next();

        return std::make_unique<NumberNode>(val);
    });
}

returns<nodep> Parser::_string(const enum TokenID id) noexcept
{
    return this->_tok.transform([this, id](Token & tok){
        this->next();
        return std::make_unique<StringNode>(tok.get<std::string_view>(), id);
    });    
}

returns<nodep> Parser::_comma(const enum TokenID end_token, const enum TokenID delim = TT_Comma) noexcept
{
    std::vector<nodep> nodes;

    while (this->_tok && this->_tok->id != end_token)
    {
        auto expression = this->_top_level();

        if (!expression)
            return expression;

        nodes.emplace_back(std::move(expression.value()));

        if (!this->_tok || this->_tok->id != delim)
        {
            break;
        }

        if (end_token != TT_Unknown)
            this->next();
    }

    return make_expected_unique<returns<nodep>, ListNode>(std::move(nodes));
}

returns<nodep> Parser::_array() noexcept
{
    this->next();

    return this->_comma(TT_RightSquareBracket)
        .and_then([this](nodep node) -> returns<nodep> {
        
        if (this->_tok->id != TT_RightSquareBracket) 
            return make_unexpected<returns<nodep>>("syntax", "expected ']' after a comma to initalizate an array");

        this->next();
        return node;
    });
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

    while (this->_tok->id != TT_Keyword && this->_tok->get<KeywordTypes>() != KW_end)
    {
        if (!this->_tok)
        {
            return make_unexpected<returns<nodep>>("syntax", "\"switch\" was incomplete before EOF");
        }

        if (this->_tok->id != TT_Colon || !(this->_tok->is<KeywordTypes>() && this->_tok->get<KeywordTypes>() == KW_default))
        {
            return make_unexpected<returns<nodep>>("syntax", "expected ':' or \"default\" as a case expression starter");
        }

        if (this->_tok->id == TT_Colon)
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

        if (this->_tok->id != TT_Equal)
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
        return returns<nodep>(std::unexpect, this->_tok.error());
    }

    auto start = this->_tok->start;

    switch (this->_tok->id)
    {
    case TT_Int:
    case TT_Double:
        return this->_number();
    case TT_String:
    case TT_Identifier:
    {
        auto value = this->_string(this->_tok->id);

        if (this->_tok->id != TT_Identifier || !value)
            return value;

        std::vector<nodep> arguments;
        arguments.push_back(std::move(*value));

        while (this->_tok && this->_tok->id == TT_Identifier)
        {
            auto arg = this->_string(this->_tok->id);
            if (!arg)
                return arg;
            arguments.push_back(nodep(const_cast<nodep *>(&*arg)->release()));
        }

        return make_expected_unique<returns<nodep>, ListNode>(std::move(arguments));
    }
    case TT_Keyword:
        if (this->_tok->get<KeywordTypes>() == KW_switch)
        {
            return this->_switch();
        }
        else if (this->_tok->get<KeywordTypes>() == KW_do)
        {
            return this->_task();
        }
        return make_unexpected<returns<nodep>>("syntax", "unexpected keyword", start, this->_tok->end);
    case TT_LeftParentesis:
        this->next();

        node = this->_top_level();
        if (!node)
            return node;
        start = this->_tok->start;
        if (this->_tok->id == TT_Comma)
        {
            this->next();
            std::vector<nodep> arglistv;
            auto args = this->_comma(TT_RightParentesis);
            if (!args)
                return args;
            if (!this->_tok || this->_tok->id != TT_RightParentesis)
                return make_unexpected<returns<nodep>>("syntax", "expected ')'", start, this->_tok->end);

            auto &arglist = (*args)->as<ListNode>().nodes();
            arglistv.push_back(std::move(*node));

            for (auto &e : arglist)
            {
                arglistv.push_back(nodep(const_cast<nodep *>(&e)->release()));
            }

            this->next();

            return make_expected_unique<returns<nodep>, ListNode>(std::move(arglistv));
        }

        if (!this->_tok || this->_tok->id != TT_RightParentesis)
        {
            return make_unexpected<returns<nodep>>("syntax", "expected ')'", start, this->_tok->end);
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

    auto type = this->_tok->id;
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
    auto start = this->_tok->start;
    auto node = this->_prefix();
    if (!node)
        return node;

    if (this->_tok->id == TT_Dot)
    {
        this->next();
        returns<nodep> child = this->_postfix();
        if (!child)
            return child;

        return make_expected_unique<returns<nodep>, ScopeNode>(*node, *child);
    }

    if (this->_tok->id == TT_LeftSquareBracket)
    {
        this->next();
        start = this->_tok->start;
        returns<nodep> index = this->_top_level();
        if (!index)
            return index;
        if (!this->_tok)
            return make_unexpected<returns<nodep>>("syntax", "expected indexable expression", start, this->_tok->end);
        if (this->_tok->id == TT_RightSquareBracket)
            return make_unexpected<returns<nodep>>("syntax", "expected a ']' to close an indexable expression", start, this->_tok->end);

        this->next();

        return make_expected_unique<returns<nodep>, CallNode>(std::move(*node), std::move(*index), Index);
    }

    if (this->_tok->id == TT_LeftParentesis)
    {
        this->next();

        start = this->_tok->start;

        auto arguments = this->_comma(TT_RightParentesis);
        if (!arguments)
            return arguments;

        if (!this->_tok || this->_tok->id != TT_RightParentesis)
        {
            return make_unexpected<returns<nodep>>("syntax", "expected a ')' after function call", start, this->_tok->end);
        }
        this->next();

        return make_expected_unique<returns<nodep>, CallNode>(std::move(*node), std::move(*arguments), Call);
    }

    return node;
}

returns<nodep> Parser::_term() noexcept
{
    auto lside = this->_postfix();
    
    if (!this->_tok)
        return returns<nodep>(std::unexpect, this->_tok.error());

    if (!lside)
        return lside;

    auto &type = this->_tok->id;
    while (this->_tok && (type == TT_Multiply || type == TT_Divide))
    {
        this->next();
        auto rside = this->_postfix();
        if (!rside)
            return rside;

        if ((*lside)->is(String) || (*rside)->is(String))
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

    auto &type = this->_tok->id;
    while (this->_tok && (type == TT_Plus || type == TT_Minus))
    {
        this->next();
        auto rside = this->_term();
        if (!rside)
            return rside;

        if ((*lside)->is(String) || (*rside)->is(String))
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

    auto &type = this->_tok->id;
    while (this->_tok && type >= TT_LessThan && type <= TT_GreaterThanEqual)
    {
        this->next();
        auto rside = this->_factor();
        if (!rside)
            return rside;

        if ((*lside)->is(String) || (*rside)->is(String))
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
    auto start = this->_tok->start;

    if (!left_side || !this->_tok)
        return left_side;

    if (this->_tok->id != TT_Arrow && this->_tok->id != TT_Question)
        return left_side;

    if (this->_tok->id == TT_Arrow)
    {
        this->next();

        auto arg_type = (*left_side)->type();

        if (arg_type != Identifier && arg_type != List)
            return make_unexpected<returns<nodep>>("syntax", "expected argument(s) to be either an identifier or a list of identifiers", start, this->_tok->end);

        if (!this->_tok)
            return make_unexpected<returns<nodep>>("syntax", "expected lambda body after '=>'", start, this->_tok->end);

        auto body = this->_comparisions();
        if (!body)
            return body;

        return make_expected_unique<returns<nodep>, LambdaNode>(std::move(*left_side), std::move(*body));
    }

    this->next();

    if (!this->_tok)
        return make_unexpected<returns<nodep>>("syntax", "expected expression after '?'", start, this->_tok->end);

    start = this->_tok->start;

    auto on_true = this->_comparisions().and_then([this, &start](nodep on_true) -> returns<nodep> {
        if (this->_tok->id != TT_Colon)
            return make_unexpected<returns<nodep>>("syntax", "expected ':' after expression", start, this->_tok->end);

        this->next();
        return on_true;
    });


    return monad_bind([](nodep condition, nodep on_true, nodep on_false) -> nodep {
        return std::make_unique<TernaryNode>(std::move(condition), std::move(on_true), std::move(on_false));
    }, std::move(left_side), std::move(on_true), this->_comparisions());
}

returns<nodep> Parser::_toplevel_type() noexcept
{
    auto start = this->_tok->start;

    if (this->_tok->id == TT_LeftSquareBracket || this->_tok->id == TT_RightSquareBracket)
    {
        bool closed_started = this->_tok->id == TT_LeftSquareBracket;
        this->next();
        start = this->_tok->start;

        if (this->_tok->id != TT_Int && this->_tok->id != TT_Double)
            return make_unexpected<returns<nodep>>("syntax", "expected integer literal for starting type range", start, this->_tok->end);

        auto range_start = this->_comparisions();

        if (!range_start) return range_start;

        if (this->_tok->id != TT_Comma)
            return make_unexpected<returns<nodep>>("syntax", "expected a comma before ending range type", this->_tok->start, this->_tok->end);

        this->next();

        auto range_end = this->_comparisions();

        if (!range_end) return range_end;

        if (this->_tok->id != TT_RightSquareBracket && this->_tok->id != TT_LeftSquareBracket)
            return make_unexpected<returns<nodep>>("syntax", "expected a ']' or a '[' to close a type range", this->_tok->start, this->_tok->end);

        bool close_ended = this->_tok->id == TT_RightSquareBracket;

        this->next();

        return make_expected_unique<returns<nodep>, TypeNode>(std::move(*range_start), std::move(*range_end), closed_started, close_ended);
    }

    return make_unexpected<returns<nodep>>("syntax error", "type signature not implemented", start, this->_tok->end);
}


returns<nodep> Parser::_statement() noexcept
{
    auto start = this->_tok->start;

    if (!this->_tok)
        return make_unexpected<returns<nodep>>("syntax", "expected either 'include' or name as a top level statement", start, this->_tok->end);

    if (!this->_tok || this->_tok->id != TT_Identifier)
        return make_unexpected<returns<nodep>>("syntax", "current token cannot be a name", start, this->_tok->end);

    auto name = this->_tok->get<std::string_view>();
    this->next();

    if (this->_tok && this->_tok->id == TT_Arrow)
    {
        this->next();
        return this->_comparisions().transform([](nodep body){
            return std::make_unique<LambdaNode>(nullptr, std::move(body));
        });
    }

    if (this->_tok && this->_tok->id == TT_TypeDefine)
    {
        this->next();

        auto value = this->_toplevel_type();
        
        return value;
    }

    if (!this->_tok || this->_tok->id != TT_Equal)
        return make_unexpected<returns<nodep>>("syntax", "expected '=' after name", start, this->_tok->end);

    this->next();

    return this->_top_level()
        .transform([&name](nodep value){
            return std::make_unique<ExpressionNode>(name, std::move(value));
        });
}

std::optional<std::vector<nodep>> Parser::parse()
{
    std::vector<nodep> nodes;

    while (this->_tok)
    {
        auto node = this->_statement();
        
        if (node) {
            nodes.push_back(std::move(*node));
        } else {
            std::cout << node.error().to_string(this->_fname) << std::endl;
            return std::nullopt;
        };


    }

    return nodes;
}