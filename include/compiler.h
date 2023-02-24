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
#include

#include "platform.h"
#include "types.h"


enum register_status {
    reg_trashed = 0,
    reg_used = 1
};

struct Assembler {
    struct Array * parsed_nodes;
    struct HashMap * table;
    struct List * instructions;
    uint64_t dot;
    uint64_t root_index;
    uint64_t entry_point;
};

struct Assembler * create_assembler(struct Array * parsed_nodes)

#endif