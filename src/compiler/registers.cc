/**
 * @file registers.cc
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief takes care of register manipulation and usage
 * @version 
 * @date 2023-01-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <zenith.hpp>

using Assembler = Compiler::Assembler;

int Assembler::request_register(bool descending, int set_next)
{
    static int next_reg = -1;
    int i;

    if (set_next != -1) {
        next_reg = set_next;
        // compiler can then check if register is free
        return this->registers[set_next];
    }
    
    if (next_reg != -1 && this->registers[next_reg]) {
        // get pre-reserved register
        this->registers[set_next] = 1;
        return set_next;
    }

    if (!descending) {
        // normal register usage
        for (i = 1; i < 32; ++i)
            if (!this->registers[i])
                break;
    } else {
        // passing arguments around
        for (i = 31; i >= 0; --i)
            if (!this->registers[i])
                break;
    }
    
    if (i != -1) {
        this->registers[i] = 1;
        return i;
    } 
    
    //! TODO: use stack (if target supports it)
    Error("compiler", "ran out of registers");
}

void Assembler::clear_register(int index)
{
    this->registers[index] = 0;
}