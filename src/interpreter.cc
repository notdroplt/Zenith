#include "zenith.hpp"	

auto Interpreter::interpret_UnaryNode(const Parse::UnaryNode *node, Interpreter::Context &context)
-> Interpreter::type_pointer
{
	auto *operand = interpret_Node(node->value, context);

	if (operand->type() > Interpreter::vNumber) {
		Error("runtime error", "cannot unary operate a non-number");
	}

	auto &value = operand->get<Number>()->value();
	value = node->token == Parse::TT_Minus ? -value : static_cast<long double>(value == 0.0);
	return operand;
}

auto Interpreter::interpret_BinaryNode(const Parse::BinaryNode *node, Interpreter::Context &context)
-> Interpreter::type_pointer
{
	auto *left = interpret_Node(node->left, context);
	auto *right = interpret_Node(node->right, context);

	if (left->type() > Interpreter::vNumber || right->type() > Interpreter::vNumber) {
		Error("runtime", "cannot binary operate over non-numbers");
	}

	auto leftv = left->get<Number>()->value();
	auto rightv = right->get<Number>()->value();

	switch (node->token) {
		case Parse::TT_Plus:
			leftv += rightv;
			break;
		case Parse::TT_Minus:
			leftv -= rightv;
			break;
		case Parse::TT_Multiply:
			leftv *= rightv;
			break;
		case Parse::TT_Divide:
			leftv /= rightv;
			break;
		case Parse::TT_CompareEqual:
			leftv = static_cast<long double>(leftv == rightv);
			break;
		case Parse::TT_NotEqual:
			leftv = static_cast<long double>(leftv != rightv);
			break;
		case Parse::TT_GreaterThan:
			leftv = static_cast<long double>(leftv > rightv);
			break;
		case Parse::TT_GreaterThanEqual:
			leftv = static_cast<long double>(leftv >= rightv);
			break;
		case Parse::TT_LessThan:
			leftv = static_cast<long double>(leftv < rightv);
			break;
		case Parse::TT_LessThanEqual:
			leftv = static_cast<long double>(leftv <= rightv);
			break;

		default:
			break;
	}

	auto res = leftv;

	return new Number(res);
}

auto Interpreter::interpret_CallNode(const Parse::CallNode *node, Interpreter::Context &context)
-> Interpreter::type_pointer
{
	auto *caller = interpret_Node(node->expr, context);
	if ((caller == nullptr) ||
		!(caller->type() == Interpreter::vLambda || caller->type() == Interpreter::vBuiltin)) {
		Error("runtime", "expected caller to be a function");
	}

	if (caller->type() == Interpreter::vBuiltin) {
		return caller->get<BuiltinFunction>()->run(context, node->args);
	}
	return caller->get<Lambda>()->run(context, node->args);
}

auto Interpreter::interpret_Node(const Parse::node_pointer &ptr, Interpreter::Context &context)
-> Interpreter::type_pointer
{
	switch (ptr->type) {
		case Parse::Unused:
			Error("runtime", "found unused node, this should be impossible actually");
		case Parse::Integer:
		case Parse::Double:
			if (ptr->cget<Parse::NumberNode>()->type == Parse::Integer) {
				return new Interpreter::Number(ptr->cget<Parse::NumberNode>()->number);
			}
			return new Interpreter::Number(ptr->cget<Parse::NumberNode>()->value);
		case Parse::Identifier:
			return context.lookup(ptr->cget<Parse::StringNode>()->value);
		case Parse::String:
			return new Interpreter::String(ptr->cget<Parse::StringNode>()->value);
		case Parse::Unary:
			return interpret_UnaryNode(ptr->cget<Parse::UnaryNode>(), context);
		case Parse::Binary:
			return interpret_BinaryNode(ptr->cget<Parse::BinaryNode>(), context);
		case Parse::Ternary:
			return interpret_Node(
			interpret_Node(ptr->cget<Parse::TernaryNode>()->condition, context)->is_true()
			? ptr->cget<Parse::TernaryNode>()->trueop
			: ptr->cget<Parse::TernaryNode>()->falseop,
			context);
		case Parse::Expression:
			return context.insert(
			ptr->cget<Parse::ExpressionNode>()->name,
			interpret_Node(ptr->cget<Parse::ExpressionNode>()->value, context));
		case Parse::Lambda:
			return context.insert(ptr->cget<Parse::LambdaNode>()->name,
								  new Lambda(ptr->cget<Parse::LambdaNode>()));
		case Parse::Call:
			return interpret_CallNode(ptr->cget<Parse::CallNode>(), context);
		case Parse::Scope:
		default:
			Error("runtime", "unhandled node");
	}
}

void add_builtins_to_context(Interpreter::Context &ctx);

auto Interpreter::interpreter_init(int argc, std::vector<Parse::node_pointer> &vec) -> int
{
	Interpreter::Context context = Interpreter::Context();

	add_builtins_to_context(context);
	for (auto &&item : vec) {
		interpret_Node(item, context);
	}
	auto  main_func = context["main"];
	if (main_func == nullptr) {
		return 1;
	}

	if (main_func->type() != Interpreter::vLambda) {
		Error("runtime", "expected main to be a lambda");
	}

	std::vector<type_pointer> args;
	args.push_back(new Number(static_cast<uint64_t>(argc)));

	auto *return_val = main_func->get<Interpreter::Lambda>()->run(context, args);
	
	if (!return_val) {
		return 0;
	}

	if (return_val->type() != Interpreter::vNumber) {
		Error("runtime", "expected main to return <int>");
	}
	return static_cast<int>(return_val->get<Interpreter::Number>()->value());
}

void add_builtins_to_context(Interpreter::Context & ctx [[maybe_unused]])
{

}
