/**
 * @file compiler.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief defines a compiler that should be able to scale to multiple architectures
 * @version
 * @date 2023-02-21
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#ifndef ZENITH_COMPILER_H
#include <stdbool.h>

#include "nodes.h"
#include "zenithvm.h"
#include "platform.h"
#include "types.h"

/**
 * @brief defines statuses for registers, which can either be:
 *
 * 0 -> trashed, means its not being used, but value is unknown
 * 1 -> used, somewhere in the code that register is being used
 *
 */
enum register_status
{
    reg_trashed = 0, /*!< trashed status */
    reg_used = 1     /*!< used status */
};

/**
 * @brief defines a struct which will be resposible for assembling the nodes
 *
 * assembling right now is just traversing a tree and returning the instructions
 * to perform those actions, there is still need to implement an optimizer and a
 * type checker, which will be done later
 */
struct Assembler
{
    const struct Array *parsed_nodes; /*!< array of nodes that have been parsed */
    struct HashMap *table;            /*!< table of elements */
    struct List *instructions;        /*!< compiled instructions */
    uint64_t dot;                     /*!< current offset in the file */
    uint64_t root_index;              /*!< how many root nodes have been parsed */
    uint64_t entry_point;             /*!< where should the code start */
    uint32_t registers;               /*!< register status */
};

/**
 * @brief generate the assembler struct
 *
 * @param [in] parsed_nodes nodes already parsed (and futurely also optimized)
 * @return assembler struct
 *
 */
struct Assembler *create_assembler(const struct Array *parsed_nodes);

/**
 * @brief translates a compiling unit (a file)
 *
 * @param [in, out] assembler assembler struct
 *
 * @return array of instructions
 *
 * Complexity: unpredictable (depends on every node traversing complexity)
 */
struct Array *translate_unit(struct Assembler *assembler);

#endif