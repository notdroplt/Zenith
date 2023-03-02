#include <nodes.h>
#include <types.h>	

/**
 * @defgroup optmizfunc optimization functions
 * @brief define functions for optimizing nodes
 * 
 * @{
 */

/**
 * It tries to optimize the unary operation by doing it at compile time, and if it can't, it returns
 * NULL, which means that the unary operation should be done at runtime
 * 
 * @param value the value to be optimized
 * @param token the token type of the operator
 * 
 * @return A node pointer.
 */

node_pointer optimized_unarynode(node_pointer value, const enum TokenTypes token) {
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
/* @} */ /* end of group optmizfunc*/
