#include "parser.h"

/**
 * @defgroup optmizfunc optimization functions
 * @brief define functions for optimizing nodes
 * 
 * @{
 */

/**
 * @brief tries to optimize an unary node.
 * if it can't, returns NULL
 * 
 * @param value the value to be optimized
 * @param token the token type of the operator
 * 
 * @return A node pointer.
 */

struct Node * optimized_unarynode(struct Node * value, const enum TokenTypes token) {
	if (token == TT_Plus) return value; /* this is what "discarted" means */

	
	if (value->isconst && value->type == Integer) {
		struct NumberNode * numnode = (struct NumberNode *)value;
		switch(token) {
			case TT_Increment:
				++numnode->number;
				break;
			case TT_Decrement:
				--numnode->number;
				break;
			case TT_Tilda:
				numnode->number = ~numnode->number;
				break;
			case TT_Not:
				numnode->number = !numnode->number;
				break;
			case TT_Minus:
				numnode->number = -numnode->number;
				break;
			default:
				delete_node(value);
				return NULL;
		} 
		return value;
	} else if (value->isconst && value->type == Double) {
		struct NumberNode * numnode = (struct NumberNode *)value;
		switch(token) {
			case TT_Increment:
				++numnode->number;
				break;
			case TT_Decrement:
				--numnode->number;
				break;
			case TT_Tilda:
				numnode->number = ~numnode->number;
				break;
			case TT_Not:
				numnode->number = !numnode->number;
				break;
			case TT_Minus:
				numnode->number = -numnode->number;
				break;
			default:
				delete_node(value);
				return NULL;
		} 
		return value;
	} 
	/* for now the only optimizations are compile time constant inlining */
	/* if no optimization was possible, simply return a null node an allocate memory (sadly) */
	return NULL;
}

struct Node * optimized_binarynode(struct Node * left, const enum TokenTypes token, struct Node * right){
	if (node_equals(left, right)) {
		delete_node(right);
		switch (token) {
			case TT_Minus:
				delete_node(left);
				return create_intnode(0);
			case TT_Divide:
				delete_node(left);
				return create_intnode(1);
			case TT_LessThan:
				delete_node(left);
				return create_intnode(0);
			case TT_LessThanEqual:
				delete_node(left);
				return create_intnode(1);
			case TT_CompareEqual:
				delete_node(left);
				return create_intnode(1);
			case TT_NotEqual:
				delete_node(left);
				return create_intnode(0);
			case TT_GreaterThan:
				delete_node(left);
				return create_intnode(0);
			case TT_GreaterThanEqual:
				delete_node(left);
				return create_intnode(1);
			case TT_BitwiseOr:
				return left;
			case TT_BitwiseAnd:
				return left;
			case TT_BitWiseXor:
				delete_node(left);
				return create_intnode(0);
			default: 
				break;
			}
	}
	if (left->isconst && right->isconst && left->type == Integer && right->type == Integer) {
		struct NumberNode * nleft = (struct NumberNode *)left;
		struct NumberNode * nright = (struct NumberNode *)right;
		switch (token) {				
			case TT_Plus:
				nleft->number += nright->number;
				break;
			case TT_Minus:
				nleft->number -= nright->number;
				break;
			case TT_Multiply:
				nleft->number *= nright->number;
				break;
			case TT_Divide:
				nleft->number *= nright->number;
				break;
			case TT_LessThan:
				nleft->number = nleft->number < nright->number;
				break;
			case TT_LessThanEqual:
				nleft->number = nleft->number <= nright->number;
				break;
			case TT_LeftShift:
				nleft->number <<= nright->number;
				break;			
			case TT_RightShift:
				nleft->number >>= nright->number;
				break;
			case TT_CompareEqual:
				nleft->number = nleft->number == nright->number;
				break;
			case TT_NotEqual:
				nleft->number = nleft->number != nright->number;
				break;
			case TT_GreaterThan:
				nleft->number = nleft->number > nright->number;
				break;
			case TT_GreaterThanEqual:
				nleft->number = nleft->number >= nright->number;
				break;
			case TT_BitwiseOr:
				nleft->number |= nright->number;
				break;
			case TT_BinaryOr:
				nleft->number = nleft->number || nright->number;
				break;
			case TT_BitwiseAnd:
				nleft->number &= nright->number;
				break;
			case TT_BinaryAnd:
				nleft->number = nleft->number && nright->number;
				break;
			case TT_BitWiseXor:
				nleft->number ^= nright->number;
				break;
			default: 
				goto opt_double_case;
			}
			delete_node(right);
			return left;
        }
opt_double_case:
	if (left->isconst && right->isconst && (left->type == Double || right->type == Double)) {
		struct NumberNode * nleft = (struct NumberNode *)left, * nright;
		if (nleft->value == 0.0 && nleft->number != 0)
			nleft->value = nleft->number;

		nright = (struct NumberNode *)right;
		if (nleft->value == 0.0 && nleft->number != 0)
			nleft->value = nleft->number;
		switch (token) {				
			case TT_Plus:
				nleft->value += nright->value;
				break;
			case TT_Minus:
				nleft->value -= nright->value;
				break;
			case TT_Multiply:
				nleft->value *= nright->value;
				break;
			case TT_Divide:
				nleft->value /= nright->value;
				break;
			case TT_LessThan:
				nleft->value = nleft->value < nright->value;
				break;
			case TT_LessThanEqual:
				nleft->value = nleft->value <= nright->value;
				break;
			case TT_CompareEqual:
				nleft->value = nleft->value == nright->value;
				break;
			case TT_NotEqual:
				nleft->value = nleft->value != nright->value;
				break;
			case TT_GreaterThan:
				nleft->value = nleft->value > nright->value;
				break;
			case TT_GreaterThanEqual:
				nleft->value = nleft->value >= nright->value;
				break;
			case TT_BinaryOr:
				nleft->value = nleft->value || nright->value;
				break;
			case TT_BinaryAnd:
				nleft->value = nleft->value && nright->value;
				break;
			default: 
				goto opt_double_case;
			}
			delete_node(right);
			return left;
        }
	return NULL;
}

struct Node * optimized_ternarynode(struct Node * condition, struct Node * true_op, struct Node * false_op) {
	if (node_equals(true_op, false_op)) {
		delete_node(condition);
		delete_node(false_op);
		return true_op;
	}

	if (condition->isconst) {
		uint64_t cond = condition->type == Integer 
						? ((struct NumberNode *)condition)->number 
						: ((struct NumberNode *)condition)->value;
		delete_node(condition);
		if (cond) {
			delete_node(false_op);
			return true_op;
		} 
		delete_node(true_op);
		return false_op;
	}

	return NULL;
}

/* @} */ /* end of group optmizfunc*/
