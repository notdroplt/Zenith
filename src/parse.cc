/**
 * \file parse.cc
 * \author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * \brief functionality for the parser
 * \version
 * \date 2022-12-02
 *
 * \copyright Copyright (c) 2022
 *
 *
 * the parser, the lexer and the optimizer are a big mess joined together instead of separate
 * things, it might increase complexity, but at least it makes things a bit simpler and no need to
 * multiple passes
 */

#include "zenith.hpp"

Parse::Parser::Parser(std::string_view fname) : filename(fname)
{
	std::ifstream file(fname.data());
	std::stringstream buf;
	buf << file.rdbuf();
	this->lexer.content = strdup(buf.str().c_str()); 
	this->lexer.current_char = this->lexer.content[0];
	this->lexer.file_size = strlen(this->lexer.content);
	this->lexer.position.column = 1;
	this->lexer.position.index = 0;
	this->lexer.position.last_line_pos = 0;
	this->lexer.position.line = 1;
	this->next();
}


void Parse::Parser::Note(std::string_view note, std::string_view desc) const
{
	std::cerr << this->filename << ':' << this->current_position.line << ':'
			  << this->current_position.column << ": " << AnsiFormat::Cyan << note << ": "
			  << AnsiFormat::Reset << desc << '\n';
}

void Parse::Parser::Warning(std::string_view warn, std::string_view desc) const
{
	std::cerr << AnsiFormat::Bold << this->filename << ':' << this->current_position.line << ':'
			  << this->current_position.column << ": " << AnsiFormat::Yellow << warn << ": "
			  << AnsiFormat::Reset << desc << '\n';
}

void Parse::Parser::Error(std::string_view error, std::string_view desc) const
{
	std::cerr << this->filename << ':' << this->lexer.position.line << ":"
			  << this->lexer.position.column << ": " << AnsiFormat::Red <<"Parser (" << error
			  << "): error: " << AnsiFormat::Reset << desc << '\n';
	throw Parse::parse_exception();
}

/* node parsing */

/**
 * \brief parse multiple expressions in series
 *
 * \param end_token stop token
 *
 * \returns a vector of parsed nodes
 *
 */
std::vector<Parse::node_pointer> Parse::Parser::List(Parse::TokenTypes end_token) 
{
	auto list = std::vector<Parse::node_pointer>();

	while (this->current_token.type != end_token) {
		auto value = this->Ternary();
		if (!value) return {};
		
		list.emplace_back(std::move(value));

		if (this->current_token.type != Parse::TT_Comma || this->current_token.type == end_token) break;
		
		this->next();
	}

	return list;
}

/**
 * \brief parse an array structure, as in
 *  [ ternary-expr , ...]
 *
 * \returns a node pointer to a ListNode
 */
auto Parse::Parser::Array() -> Parse::node_pointer
{
	auto list = this->List(Parse::TT_RightSquareBracket);
	if (list.empty()) return nullptr;
	else if (this->current_token.type != Parse::TT_RightSquareBracket) {
		this->Error("array", "expected ']'");
	}
	this->next();
	return std::make_unique<Parse::ListNode>(std::move(list));
}

/**
 * \brief parse a number
 *
 * \returns a node pointer to a NumberNode
 */
auto Parse::Parser::Number() -> Parse::node_pointer
{
	const auto tok = this->current_token;
	if (tok.type == Parse::TT_Unknown) return nullptr;
	
	this->next();

	if (tok.type == Parse::TT_Int) return std::make_unique<Parse::NumberNode>(tok.integer);
	
	return std::make_unique<Parse::NumberNode>(tok.number);
}

auto Parse::Parser::String(uint8_t type) -> Parse::node_pointer
{
	const auto tok = this->current_token;
	this->next();
	if (tok.type == Parse::TT_Unknown) return nullptr;
	return std::make_unique<Parse::StringNode>(tok.string, static_cast<Parse::NodeTypes>(type));
}

auto Parse::Parser::Switch() -> Parse::node_pointer
{
	Parse::node_pointer switch_expr;
	Parse::node_pointer current_case;
	std::vector<std::pair<Parse::node_pointer, Parse::node_pointer>> cases;
	this->next();

	if (this->current_token.type == TT_Unknown) {
		this->Error("switch", "expected a switch expression");
		return nullptr;
	}

	switch_expr = this->Ternary();

	if (!switch_expr) return nullptr;

	while (this->current_token.keyword != Parse::KW_end) {
		Parse::node_pointer value;
		if (this->current_token.type != Parse::TT_Colon &&
			this->current_token.type == Parse::TT_Keyword && this->current_token.keyword != Parse::KW_default) {
			this->Error("case", "':' or \"default\" to initiate a case expression.");
			return nullptr;
		}

		if (this->current_token.type == Parse::TT_Colon) {
			this->next();
			value = this->Ternary();
			if (!value) this->Error("case", "expected a case expression.");
		} else {
			this->next();
			value = nullptr;
		}

		if (this->current_token.type != Parse::TT_Equal) {
			this->Error("case", "expected '=' after case expression");
		}

		this->next();
		cases.emplace_back(std::move(value), this->Ternary());
	}

	if (!this->current_token.type || (this->current_token.type == Parse::TT_Keyword && this->current_token.keyword != Parse::KW_end)) {
		this->Error("switch", "expected 'end' to close switch expression");
	}

	this->next();

	return std::make_unique<Parse::SwitchNode>(std::move(switch_expr), std::move(cases));
}

auto Parse::Parser::Task() -> Parse::node_pointer
{
	this->next();
	std::vector<Parse::node_pointer> task_list = {};
	while (this->current_token.type == Parse::TT_Keyword && this->current_token.keyword != Parse::KW_end) {
		this->next();
		task_list.push_back(this->Assign());
	}
	return std::make_unique<Parse::TaskNode>(std::move(task_list));
}

auto Parse::Parser::Atom() -> Parse::node_pointer
{
	Parse::node_pointer node;
	if (this->current_token.type == Parse::TT_Unknown) {
		return nullptr;
	}

	switch (this->current_token.type) {
		case Parse::TT_Int:
		case Parse::TT_Double:
			return this->Number();
		case Parse::TT_String:
		case Parse::TT_Identifier:
			return this->String(this->current_token.type);
		case Parse::TT_Keyword:
			if (this->current_token.keyword == Parse::KW_switch)
				return this->Switch();
			else if (this->current_token.keyword == Parse::KW_do)
				return this->Task();
			return nullptr;
		case Parse::TT_LeftParentesis:
			this->next();

			node = this->Ternary();
			if (!node)
				return nullptr;
			else if (this->current_token.type != Parse::TT_RightParentesis)
				this->Error("atom", "expected ')'");

			this->next();
			return node;
		case Parse::TT_LeftSquareBracket:
			this->next();
			return this->Array();
		default:
		this->Error("atom", "unknown case");
	}
}

auto Parse::Parser::Prefix() -> Parse::node_pointer
{
	const auto type = this->current_token.type;
	if (type == Parse::TT_Plus) {
		this->Warning("discouraged uses", "use of unary plus operator `+(value)` is discouraged as it accomplishes nothing, consider removing");
		this->next();
		return this->Atom();
	}

	if (type != Parse::TT_Minus && type != Parse::TT_Not) return this->Atom();

	this->next();
	auto value = this->Ternary();
	if (!value->isConst) {
		return std::make_unique<Parse::UnaryNode>(std::move(value), type);
	}

	if (value->type == Parse::String) {
		this->Error("Prefix", "cannot use '!' or '-' in a string constant");
	}

	if (value->type == Parse::Integer) {
		auto &val_ref = value->cget<Parse::NumberNode>()->number;
		val_ref = type == Parse::TT_Minus ? -val_ref : !val_ref;
	} else {
		auto &val_ref = value->cget<Parse::NumberNode>()->value;
		val_ref = type == Parse::TT_Minus ? -val_ref : !val_ref;
	}

	return value;
}

auto Parse::Parser::Postfix() -> Parse::node_pointer
{
	auto value = this->Prefix();

	if (!value) return nullptr;

	if (this->current_token.type == Parse::TT_LeftParentesis) {
		this->next();
		auto argsnode = this->List(Parse::TT_RightParentesis);

		if (this->current_token.type == TT_Unknown || argsnode.empty()) {
			this->next();
			return value;
		} else if (this->current_token.type != Parse::TT_RightParentesis) {
			this->Error("function call", "expected a ')' after a function call");
		}

		this->next();

		return std::make_unique<Parse::CallNode>(std::move(value), std::move(argsnode));
		/// TODO: implement compile time call (this is gonna be awesome)
	}
	if (this->current_token.type == Parse::TT_LeftSquareBracket) {
		this->next();
		std::vector<Parse::node_pointer> index;
		index.push_back(this->Ternary());

		if (index.empty()) {
			this->Error("indexing", "expected an expression for an index");
		} else if (this->current_token.type != Parse::TT_RightSquareBracket) {
			this->Error("indexing", "expected a ']' to close index marker");
		}

		this->next();
		return std::make_unique<Parse::CallNode>(std::move(value), std::move(index), Parse::Index);
	}
	if (this->current_token.type == Parse::TT_Dot) {
		this->next();
		return std::make_unique<Parse::ScopeNode>(std::move(value), this->Postfix());
	}
	return value;
}

auto Parse::Parser::Term() -> Parse::node_pointer
{
	Parse::node_pointer lside = this->Postfix();
	Parse::node_pointer rside;
	if (this->current_token.type == Parse::TT_Unknown) return lside;
	
	while (this->current_token.type == Parse::TT_Multiply || this->current_token.type == Parse::TT_Divide) {
		const auto tok = this->current_token.type;
		this->next();
		rside = this->Postfix();
		if (!rside) {
			return nullptr;
		}

		if (!lside->isConst || !rside->isConst) {
			lside = std::make_unique<Parse::BinaryNode>(std::move(lside), tok, std::move(rside));
			if (this->current_token.type == Parse::TT_Unknown) {
				break;
			}
			continue;
		}

		if (lside->type == Parse::String || rside->type == Parse::String) {
			this->Error("binary", "cannot '*' or '/' a string");
		}

		long double left = 0.0L, right = 0.0L;
		switch (static_cast<uint>(lside->type) << 2U | rside->type) {
			case static_cast<uint>(Parse::Integer) << 2U | Parse::Integer: {
				left = lside->cget<NumberNode>()->number;
				right = rside->cget<NumberNode>()->number;
			} break;
			case static_cast<uint>(Parse::Integer) << 2U | Parse::Double: {
				left = lside->cget<Parse::NumberNode>()->number;
				right = rside->cget<Parse::NumberNode>()->value;
			} break;
			case static_cast<uint>(Parse::Double) << 2U | Parse::Integer: {
				left = lside->cget<Parse::NumberNode>()->value;
				right = rside->cget<Parse::NumberNode>()->number;
			} break;
			case static_cast<uint>(Parse::Double) << 2U | Parse::Double: {
				left = lside->cget<Parse::NumberNode>()->value;
				right = rside->cget<Parse::NumberNode>()->value;
			} break;
		}
		lside.reset();
		rside.reset();
		if (tok == TT_Multiply) {
			left *= right;
		} else {
			left /= right;
		}
		lside = std::make_unique<Parse::NumberNode>(static_cast<double>(left));
	}
	return lside;
}

auto Parse::Parser::Factor() -> Parse::node_pointer
{
	Parse::node_pointer lside = this->Term();
	Parse::node_pointer rside;
	if (this->current_token.type == Parse::TT_Unknown) {
		return lside;
	}
	while (this->current_token.type == Parse::TT_Plus || this->current_token.type == Parse::TT_Minus) {
		const auto tok = this->current_token.type;
		this->next();
		rside = this->Term();
		if (!rside) {
			return nullptr;
		}
		if (!lside->isConst || !rside->isConst) {
			lside = std::make_unique<Parse::BinaryNode>(std::move(lside), tok, std::move(rside));
			if (this->current_token.type == Parse::TT_Unknown) {
				break;
			}
			continue;
		}

		if (lside->type == Parse::String || rside->type == Parse::String) {
			this->Error("binary", "cannot '*' or '/' a string");
		}

		long double left = 0.0L, right = 0.0L;
		switch (static_cast<uint>(lside->type) << 2U | rside->type) {
			case static_cast<uint>(Parse::Integer) << 2U | Parse::Integer: {
				left = lside->cget<NumberNode>()->number;
				right = rside->cget<NumberNode>()->number;
			} break;
			case static_cast<uint>(Parse::Integer) << 2U | Parse::Double: {
				left = lside->cget<Parse::NumberNode>()->number;
				right = rside->cget<Parse::NumberNode>()->value;
			} break;
			case static_cast<uint>(Parse::Double) << 2U | Parse::Integer: {
				left = lside->cget<Parse::NumberNode>()->value;
				right = rside->cget<Parse::NumberNode>()->number;
			} break;
			case static_cast<uint>(Parse::Double) << 2U | Parse::Double: {
				left = lside->cget<Parse::NumberNode>()->value;
				right = rside->cget<Parse::NumberNode>()->value;
			} break;
		}
		lside.reset();
		rside.reset();
		if (tok == TT_Plus) {
			left += right;
		} else {
			left -= right;
		}
		lside = std::make_unique<Parse::NumberNode>(static_cast<double>(left));
	}
	return lside;
}

auto Parse::Parser::Comparisions() -> Parse::node_pointer
{
	Parse::node_pointer lside = this->Factor();
	Parse::node_pointer rside;
	if (this->current_token.type == Parse::TT_Unknown) {
		return lside;
	}

	while (this->current_token.type >= Parse::TT_LessThan &&
		   this->current_token.type <= Parse::TT_GreaterThanEqual) {
		const auto tok = this->current_token.type;
		this->next();
		rside = this->Factor();
		if (!rside) {
			return nullptr;
		}

		if (!lside->isConst || !rside->isConst) {
			lside = std::make_unique<Parse::BinaryNode>(std::move(lside), tok, std::move(rside));
			if (this->current_token.type == Parse::TT_Unknown) {
				break;
			}
			continue;
		}

		if (lside->type == Parse::String || rside->type == Parse::String) {
			this->Error("binary", "cannot '*' or '/' a string");
		}

		long double left = 0.0L, right = 0.0L;
		bool result = false;
		switch (static_cast<uint>(lside->type) << 2U | rside->type) {
			case static_cast<uint>(Parse::Integer) << 2U | Parse::Integer: {
				left = lside->cget<NumberNode>()->number;
				right = rside->cget<NumberNode>()->number;
			} break;
			case static_cast<uint>(Parse::Integer) << 2U | Parse::Double: {
				left = lside->cget<Parse::NumberNode>()->number;
				right = rside->cget<Parse::NumberNode>()->value;
			} break;
			case static_cast<uint>(Parse::Double) << 2U | Parse::Integer: {
				left = lside->cget<Parse::NumberNode>()->value;
				right = rside->cget<Parse::NumberNode>()->number;
			} break;
			case static_cast<uint>(Parse::Double) << 2U | Parse::Double: {
				left = lside->cget<Parse::NumberNode>()->value;
				right = rside->cget<Parse::NumberNode>()->value;
			} break;
		}
		lside.reset();
		rside.reset();
		switch (tok) {
			case Parse::TT_LessThan:
				result = left < right;
				break;
			case Parse::TT_LessThanEqual:
				result = left <= right;
				break;
			case Parse::TT_CompareEqual:
				result = left == right;
				break;
			case Parse::TT_NotEqual:
				result = left != right;
				break;
			case Parse::TT_GreaterThan:
				result = left > right;
				break;
			case Parse::TT_GreaterThanEqual:
				result = left >= right;
				break;
			default:
				this->Error("binary",
							"'impossible' error, as no token should get someone here :) ");
		}
		lside = std::make_unique<Parse::NumberNode>(static_cast<uint64_t>(result));
	}
	return lside;
}

auto Parse::Parser::Ternary() -> Parse::node_pointer
{
	Parse::node_pointer condition = this->Comparisions();
	Parse::node_pointer trueop;
	Parse::node_pointer falseop;

	if (this->current_token.type != Parse::TT_Question ||
		this->current_token.type == Parse::TT_Unknown || !condition) {
		return condition;
	}
	this->next();

	if (this->current_token.type == Parse::TT_Unknown) {
		this->Error("ternary", "expected expression after '?'");
	} else if (!(trueop = this->Comparisions())) {
		return nullptr;
	} else if (this->current_token.type == Parse::TT_Unknown ||
			   this->current_token.type != Parse::TT_Colon) {
		this->Error("ternary", "expected expression after ':'");
	}

	this->next();
	falseop = this->Comparisions();
	if (falseop && condition->isConst) {
			bool truthy  = (condition->type == Parse::Integer &&
					  condition->cget<Parse::NumberNode>()->number) ||
					 (condition->type == Parse::Double &&
					  condition->cget<Parse::NumberNode>()->value) ||
					 (condition->type == Parse::String &&
					  condition->cget<Parse::StringNode>()->value.empty());
			return truthy ? std::move(trueop) : std::move(falseop);
		}
	else if (falseop) 
		return std::make_unique<Parse::TernaryNode>(std::move(condition), std::move(trueop),
													std::move(falseop));
	return nullptr;
}

auto Parse::Parser::Name() -> std::string_view
{
	if (this->current_token.type != Parse::TT_Identifier) {
		this->Error("name", "token cannot be a name");
	}
	return this->current_token.string;
}

auto Parse::Parser::Lambda(std::string_view name, bool arrowed) -> Parse::node_pointer
{
	std::vector<Parse::node_pointer> args;
	if (arrowed) {
		return std::make_unique<Parse::ExpressionNode>(name, this->Ternary());
	}

	do {
		if (this->current_token.type == Parse::TT_Comma) {
			this->next();
			continue;
		} else if (this->current_token.type != Parse::TT_Identifier) {
			break;
		}
		auto arg = this->current_token.string;

		this->next();
		args.push_back(std::make_unique<Parse::StringNode>(arg, Identifier));
	} while (this->current_token.type);

	if (!arrowed && this->current_token.type != Parse::TT_RightParentesis) {
		this->Error("lambda", "expected ')' to close parameter list");
	} else if (!arrowed) {
		this->next();
	}
	if (this->current_token.type != Parse::TT_Arrow) {
		this->Error("lambda", "expected '=>' after parameter list");
	}

	this->next();
	return std::make_unique<Parse::LambdaNode>(name, this->Ternary(), std::move(args));
}

auto Parse::Parser::Define(std::string_view name) -> Parse::node_pointer
{
	auto cast = this->String(this->current_token.type);
	if (!cast) {
		this->Error("define", "expected a type");
	}

	return std::make_unique<Parse::DefineNode>(name, std::move(cast));
}

auto Parse::Parser::Assign() -> Parse::node_pointer
{
	if (this->current_token.keyword == Parse::KW_include) {
		this->next();
		auto str = this->current_token.string;
		this->next();
		return std::make_unique<Parse::IncludeNode>(str);
	}
	auto name = this->Name();
	this->next();

	if (name.empty()) {
		this->Error("assign", "a name is necessary for each expression assigned");
	}

	const auto tok = this->current_token;
	this->next();
	Parse::node_pointer ptr;
	if (tok.type == Parse::TT_LeftParentesis || tok.type == Parse::TT_Arrow) {
		ptr = this->Lambda(name, tok.type == Parse::TT_Arrow);
	} else if (tok.type == Parse::TT_Colon || (this->current_token.type == Parse::TT_Keyword && this->current_token.keyword == Parse::KW_as)) {
		this->Warning("discouraged use", "use of defines is not yet completed, consider removing");
		ptr = this->Define(name);
	} else if (tok.type == Parse::TT_Equal) {
		ptr = std::make_unique<ExpressionNode>(name, this->Ternary());
	} else {
		this->Error("assign", "expected either '(' or '=>', \"as\" or ':', or '='");
	}

	this->map[name] = std::make_pair(ptr.get(), 0);
	return ptr;
}

auto Parse::Parser::File() -> std::vector<Parse::node_pointer> try 
{
	std::vector<Parse::node_pointer> value;

	while (this->current_token.type) {
		auto node = this->Assign();
		if (!node) break;
		value.emplace_back(std::move(node));
	}
	return value;
} catch(Parse::parse_exception &e) {
	return {};
}
