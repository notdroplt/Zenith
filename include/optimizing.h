#pragma once
#ifndef ZENITH_OPTIMIZING_H
#define ZENITH_OPTIMIZING_H 1

#include <nodes.h>

/**
 * @brief generate an optimized unary node
 *
 * Complexity: Constant (but probably slow)
 * 
 * @param value unary node's value
 * @param token unary node's token
 *
 * @returns a valid pointer when the node was optimized
 * @returns NULL when the node could not be optimized
 */
node_pointer optimized_unarynode(node_pointer value, const enum TokenTypes token);

/**
 * @brief generate an optimized binary node
 * 
 * @param left left hand value
 * @param token operation token
 * @param right right hand value
 * @returns a valid pointer when the node was 
 */
node_pointer optimized_binarynode(node_pointer left, const enum TokenTypes token, node_pointer right);

#endif
