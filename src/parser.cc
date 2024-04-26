#include "parser.hh"
#include "lexer.hh"
#include <fstream>
#include <functional>
#include <span>
#include <sstream>

#include <iostream>

namespace
{
	using Token = zenith::lexer::Token;
	using Lexer = zenith::lexer::Lexer;
	using Tokens = zenith::lexer::Tokens;
	using lexer_return = zenith::lexer::lexer_return;
	using TVar = zenith::lexer::TokenVariants;
	using StringToken = zenith::lexer::StringToken;

	using namespace zenith::parser;

	using Errors = zenith::errors::Errors;

	using node_expect = std::expected<nodeptr, Errors>;
	using parser_return = std::pair<Parser, node_expect>;
	using parser_error = std::unexpected<Errors>;

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return top_level_expr(Parser) noexcept;

	ZENITH_CONST_NODISCARD_CONSTEXPR Parser nextt(Parser parser) noexcept
	{
		lexer_return val;
		Lexer lexer{parser.lexer()};
		while (true)
		{
			auto [lex, value] = next_token(lexer, parser.content());
			lexer = lex;
			val = value;
			if (val || val.error() != Errors::CommentFinish)
			{
				break;
			}
		}
#if defined(ZENITH_PARSER_DEBUG) && ZENITH_PARSER_DEBUG != 0
		std::clog << "Parse log: got ";
		std::array<std::string_view, 5> types = {"tokens", "string", "int", "uint", "double"};
		if (val.has_value()) {
			std::clog << types[(*val).index()] << " ";
			switch ((*val).index())
			{
			case TVar::VarTokens: {
				std::array<std::string_view, 40> tokens =
				 {"+", "-", "*", "/", "%", "<", "<=", "<<", ">>", "==", "!=",
				  ">", ">=", "?", ":", "=", "!", "(", ")", "[", "]", "{", "}",
				  ".", ",", "|", "||", "&", "&&", "^", "~", "->", ">>=", "do",
				  "match", "else", "end", "import", "struct", "union"};

				std::clog << "value: '" << tokens[std::get<TVar::VarTokens>(*val)] << '\'';
				break;
			}
			case TVar::VarString:
			{
				auto str = std::get<TVar::VarString>(*val);
				std::clog << "value: " << str.first << ", identifer: " << std::boolalpha << str.second;
				break;
			}
			case TVar::VarInt:
				std::clog << "value: " << std::get<TVar::VarInt>(*val);
				break;
			default:
				break;
			}

			std::clog << "\n";
		} else {
			
			std::clog << "error: id " << static_cast<int>(val.error()) << '\n';
		}
#endif
		return Parser(parser.content(), val, lexer);
	}

	ZENITH_CONST_NODISCARD_CONSTEXPR node_expect make_error(Errors error) noexcept
	{
		return node_expect(std::unexpect, error);
	}

	template <class T, typename... Args>
	ZENITH_CONST_NODISCARD_CONSTEXPR node_expect make_node(Args &&...args) noexcept
	{
#if defined(ZENITH_PARSER_DEBUG) && ZENITH_PARSER_DEBUG != 0
		std::clog << "Parse log: making " << typeid(T).name() << '\n';
#endif
		auto &&val = make_unique_nothrow<T>(std::forward<Args>(args)...);
		if (val != nullptr)
		{
			return val;
		}
		return make_error(Errors::OutOfMemory);
	}

	template <typename T>
	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return number_node(Parser parser, T value) noexcept
	{
		return std::make_pair(nextt(parser), make_node<NumberNode>(value));
	}

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return string_node(Parser parser, std::string_view str, bool is_id) noexcept
	{
		return std::make_pair(nextt(parser), make_node<StringNode>(str, static_cast<NodeID>(NodeID::NodeString + static_cast<int>(is_id))));
	}

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return atomic_node(Parser parser) noexcept
	{
		if (!parser.current().has_value())
		{
			return std::make_pair(parser, parser_error(parser.current().error()));
		}

		Token token = parser.current().value();

		switch (token.index())
		{
		case TVar::VarTokens:
			if (std::get<Tokens>(token) == Tokens::TokenLeftParentesis) {
					auto [par, inner] = top_level_expr(nextt(parser));
					if (!inner)
					{
						return std::make_pair(par, make_error(inner.error()));
					}
					if (!parser.current())
					{
						return std::make_pair(par, make_error(parser.current().error()));
					}
					if (parser.current()->index() != TVar::VarString || std::get<Tokens>(*par.current()) != Tokens::TokenRightParentesis)
					{
						return std::make_pair(par, make_error(Errors::ClosingParentesisOnExpression));
					}
					return std::make_pair(nextt(par), std::move(inner));
				}
			break;
		case TVar::VarString:
		{
			auto [str, is_id] = std::get<StringToken>(token);
			return string_node(parser, str, is_id);
		}

		case TVar::VarInt:
			return number_node(parser, std::get<TVar::VarInt>(token));
		
		case TVar::VarUint:
			return number_node(parser, std::get<TVar::VarUint>(token));

		case TVar::VarDouble:
			return number_node(parser, std::get<TVar::VarDouble>(token));
		}
		__builtin_unreachable();
	}

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return suffix_node(Parser parser) noexcept
	{
		auto [par1, node] = atomic_node(parser);
		if (!node)
			return std::make_pair(par1, std::move(node));

		if (!parser.current().has_value())
		{
			return std::make_pair(parser, parser_error(parser.current().error()));
		}

		Token token = parser.current().value();

		if (token.index() != TVar::VarTokens)
		{
			return std::make_pair(par1, std::move(node));
		}

		switch (std::get<Tokens>(token))
		{
		case Tokens::TokenLeftSquareBracket:
		{
			const auto par2 = nextt(par1);
			auto [par3, index] = top_level_expr(par2);
			if (!index)
			{
				return std::make_pair(par3, std::move(index));
			}
			if (!par3.current().has_value() || par3.current().value().index() != TVar::VarTokens || std::get<TVar::VarTokens>(par3.current().value()) != Tokens::TokenRightSquareBracket)
			{
				return std::make_pair(par3, make_error(Errors::ClosingBracketOnIndex));
			}
			return std::make_pair(nextt(par3), make_node<BinaryNode>(std::move(*node), Tokens::ActionIndex, std::move(*index)));
		}
		case Tokens::TokenArrow:
		case Tokens::TokenDot:
		{
			const auto par2 = nextt(par1);
			auto [par3, child] = suffix_node(par2);
			if (!child)
			{
				return std::make_pair(par3, std::move(child));
			}
			return std::make_pair(par3, make_node<BinaryNode>(std::move(*node), std::get<Tokens>(token), std::move(*child)));
		}

		default:
			return std::make_pair(par1, std::move(node));
		}
	}

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return prefix_node(Parser parser) noexcept
	{
		if (!parser.current().has_value())
		{
			return std::make_pair(parser, parser_error(parser.current().error()));
		}

		const Token token = parser.current().value();

		if (token.index() != TVar::VarTokens)
			return suffix_node(parser);

		const Tokens value = std::get<Tokens>(token);

		auto [par1, suffix] = suffix_node(parser);

		if (!suffix)
		{
			return std::make_pair(parser, std::move(suffix));
		}

		nodeptr &node = suffix.value();
		switch (value)
		{
		case Tokens::TokenMinus:
		case Tokens::TokenQuestion:
		case Tokens::TokenNot:
		case Tokens::TokenBitwiseAnd:
		case Tokens::TokenTilda:
			return std::make_pair(nextt(par1), make_node<UnaryNode>(value, std::move(node)));
		default:
			return std::make_pair(par1, make_error(Errors::PrefixToken));
		}
	}

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return binary_node(Parser parser, std::span<const Token> tokens,
															   parser_return (*function)(Parser))
	{
		auto ret = function(parser);
		auto par1 = ret.first;
		auto left = std::move(ret.second);
		if (!left)
		{
			return std::make_pair(par1, std::move(left));
		}

		while (std::find(tokens.begin(), tokens.end(), par1.current()) != tokens.end())
		{
			const auto tok = par1.current();
			if (!tok.has_value())
				return std::make_pair(par1, make_error(tok.error()));

			if (tok.value().index() != TVar::VarTokens)
			{
				return std::make_pair(par1, make_error(Errors::InvalidTokenVariant));
			}

			const auto token = std::get<Tokens>(tok.value());

			auto [par2, right] = function(nextt(par1));

			if (!right)
			{
				return std::make_pair(par2, std::move(right));
			}

			left = make_node<BinaryNode>(std::move(*left), token, std::move(*right));
			par1 = nextt(par2);
		}

		return std::make_pair(par1, std::move(left));
	}

	constexpr inline std::array<Token, 3> prec0_tokens =
		{Tokens::TokenBitwiseAnd, Tokens::TokenBitwiseOr, Tokens::TokenBitwiseXor};

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return prec0(Parser parser)
	{
		return binary_node(parser, std::span<const Token, 3>(prec0_tokens), prefix_node);
	}

	constexpr inline std::array<Token, 2> prec1_tokens =
		{Tokens::TokenLeftShift, Tokens::TokenRightShift};

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return prec1(Parser parser)
	{
		return binary_node(parser, std::span<const Token, 2>(prec1_tokens), prec0);
	}

	constexpr inline std::array<Token, 3> prec2_tokens =
		{Tokens::TokenMultiply, Tokens::TokenDivide, Tokens::TokenModule};

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return prec2(Parser parser)
	{
		return binary_node(parser, std::span<const Token, 3>(prec2_tokens), prec1);
	}

	constexpr inline std::array<Token, 2> prec3_tokens =
		{Tokens::TokenPlus, Tokens::TokenMinus};

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return prec3(Parser parser)
	{
		return binary_node(parser, std::span<const Token, 2>(prec3_tokens), prec2);
	}

	constexpr inline std::array<Token, 6> prec4_tokens =
		{Tokens::TokenLessThan, Tokens::TokenLessThanEqual,
		 Tokens::TokenGreaterThan, Tokens::TokenGreaterThanEqual,
		 Tokens::TokenCompareEqual, Tokens::TokenNotEqual};

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return prec4(Parser parser)
	{
		return binary_node(parser, std::span<const Token, 6>(prec4_tokens), prec3);
	}

	constexpr inline std::array<Token, 2> prec5_tokens =
		{Tokens::TokenBinaryAnd, Tokens::TokenBinaryOr};

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return prec5(Parser parser)
	{
		return binary_node(parser, std::span<const Token, 2>(prec5_tokens), prec4);
	}

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return ternary_node(Parser parser)
	{
		auto condition = prec5(parser);


		if (!condition.second)
		{
			return condition;
		}

		auto [par1, cond] = std::move(condition);

		if (!par1.current().has_value() ||
			par1.current().value().index() != TVar::VarTokens ||
			std::get<Tokens>(par1.current().value()) != Tokens::TokenQuestion)
		{
			return std::make_pair(par1, std::move(cond));
		}

		auto par2 = nextt(par1);

		auto on_true = prec5(par2);

		if (!on_true.second)
		{
			if (on_true.second.error() == Errors::ContentFinish)
			{
				return std::make_pair(on_true.first, make_error(Errors::ExpressionAfterQuestion));
			}

			return on_true;
		}

		auto [par3, true_val] = std::move(on_true);

		if (!par3.current().has_value() ||
			par3.current().value().index() != TVar::VarTokens ||
			std::get<Tokens>(par3.current().value()) != Tokens::TokenColon)
		{
			return std::make_pair(par3, make_error(Errors::ColonAfterExpression));
		}

		auto par4 = nextt(par3);

		auto on_false = prec5(par4);

		if (!on_false.second)
		{
			if (on_false.second.error() == Errors::ContentFinish)
			{
				return std::make_pair(on_false.first, make_error(Errors::ExpressionAfterColon));
			}

			return on_false;
		}

		auto [par5, false_val] = std::move(on_false);

		return std::make_pair(par5, make_node<TernaryNode>(std::move(*cond), std::move(*true_val), std::move(*false_val)));
	}

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return top_level_expr(Parser parser) noexcept
	{
		return ternary_node(parser);
	}

	ZENITH_CONST_NODISCARD_CONSTEXPR parser_return statement(Parser parser) noexcept
	{
		if (!parser.current().has_value() || parser.current().value().index() != TVar::VarString)
		{
			return std::make_pair(parser, make_error(parser.current().error()));
		}

		auto curstr = std::get<TVar::VarString>(parser.current().value());

		if (!curstr.second)
		{
			return std::make_pair(parser, make_error(Errors::NameOnTopLevel));
		}

		auto [par1, name] = string_node(parser, curstr.first, curstr.second);

		if (!name)
		{
			return std::make_pair(par1, std::move(name));
		}

		std::vector<nodeptr> args;

		// here we will assume that most people won't use functions with more
		// than 8 arguments *most of the time* so reserving here will allow
		// `args.push` to not be the main stopper
		constexpr auto argument_size = 8;
		args.reserve(argument_size);

		while (par1.current().has_value() && par1.current().value().index() == TVar::VarString)
		{
			auto argstr = std::get<TVar::VarString>(par1.current().value());
			auto &&[par2, arg] = string_node(par1, argstr.first, argstr.second);

			if (!arg)
			{
				return std::make_pair(par2, std::move(arg));
			}

			args.push_back(std::move(*arg));
			par1 = par2;
		}

		if (!par1.current().has_value() && par1.current().error() != Errors::ContentFinish)
		{
			return std::make_pair(par1, make_error(par1.current().error()));
		}

		if (!par1.current().has_value() || par1.current().value().index() != TVar::VarTokens || (std::get<Tokens>(*par1.current()) != Tokens::TokenEqual && std::get<Tokens>(*par1.current()) != Tokens::TokenColon))
		{
			return std::make_pair(par1, make_error(Errors::EqualsOrColon));
		}

		auto tok = std::get<Tokens>(*par1.current());

		auto par2 = nextt(par1);

		if (tok == Tokens::TokenEqual)
		{
			auto [par3, expr] = top_level_expr(par2);
			if (!expr)
			{
				return std::make_pair(par3, std::move(expr));
			}

			return std::make_pair(nextt(par3),
								  make_node<LambdaNode>(std::move(*name), std::move(args), std::move(*expr)));
		}

		return std::make_pair(par2, make_error(Errors::NoTypesYet));
	}
}; // namespace

namespace zenith::parser
{
	Parser::Parser(std::string_view content) noexcept : m_content{content} {
		auto [_, current, lexer] = nextt(Parser{content, 0, Lexer{content}});
		this->m_current = current;
		this->m_lexer = lexer;
	}

	ZENITH_CONST_NODISCARD std::pair<Parser, std::expected<nodeptr, zenith::errors::Errors>> next_node(Parser parser) noexcept
	{
		return statement(parser);
	}
} // namespace zenith::parser
