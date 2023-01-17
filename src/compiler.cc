#include "zenith.hpp"

using Assembler = Compiler::Assembler;
using return_t = Compiler::return_t;

int Assembler::request_register()
{
    for (int i = 0; i < 32; ++i)
        if (this->registers[i] == register_status::trashed || this->registers[i] == register_status::clear)
            return i;

    //! TODO: use stack (if target supports it)
    Error("compiler", "ran out of registers");
}

void Assembler::clear_register(int index)
{
    this->registers[index] = register_status::trashed;
}

void Assembler::append_instruction(const VirtMac::instruction_t instruction)
{
    this->instructions.push_back(instruction.value);
    dot += 8;
}

return_t Assembler::assemble_number(Parse::NumberNode *node)
{
    uint64_t reg_index = this->request_register();
    std::cerr << node->number << '\n';
    // goes only if number actually needs it
    if (node->number > 0x400000000000)
    {
        this->append_instruction(VirtMac::LInstruction(VirtMac::lui_instrc, reg_index, (node->number & 0xFFFFC00000000000) >> 18));
    }
    // set lower number as `lower_num + 0` -> rd
    this->append_instruction(VirtMac::SInstruction(VirtMac::addi_instrc, 0, reg_index, node->number & 0x3FFFFFFFFFFF));

    return {reg_index};
}

return_t Assembler::assemble_unary(Parse::UnaryNode *node)
{
    auto used_regs = this->assemble(node->value);

    switch (node->token)
    {
    case Parse::TT_Minus:
        // r1 = 0 - r1
        this->append_instruction(VirtMac::SInstruction(VirtMac::subi_instrc, used_regs[0], 0, used_regs[0]));
        break;
    case Parse::TT_Not:
        // xor is basically a "conditional not", so when all bits are set, the other ones are flipped
        this->append_instruction(VirtMac::SInstruction(VirtMac::xorr_instrc, used_regs[0], used_regs[0], 1));
        break;
    case Parse::TT_Increment:
        this->append_instruction(VirtMac::SInstruction(VirtMac::addi_instrc, used_regs[0], used_regs[0], 1));
        break;
    case Parse::TT_Decrement:
        this->append_instruction(VirtMac::SInstruction(VirtMac::subi_instrc, used_regs[0], used_regs[0], 1));
        break;
    default:
        break;
    }

    return used_regs;
}

return_t Assembler::assemble_binary(Parse::BinaryNode *node, bool isJumping){
    auto used_lchild = this->assemble(node->left);
    auto used_rchild = this->assemble(node->left);

    auto & lfirst = used_lchild[0];
    auto & rfirst = used_rchild[0];
    if (!isJumping) {
        switch (node->token)
        {
        case Parse::TT_Plus:
            /*
             * r1 = r1 + r2
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::addr_instrc, lfirst, lfirst, rfirst));
            break;
        case Parse::TT_Minus:
            /* 
             * r1 = r1 - r2 
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::subr_instrc, lfirst, lfirst, rfirst));
            break;
        case Parse::TT_Multiply:
            /*
             * r1 = r1 * r2 (signed)
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::smulr_instrc, lfirst, lfirst, rfirst));
            break;
        case Parse::TT_Divide:
            /*
             * r1 = r1 / r2 (signed)
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::udivr_instrc, lfirst, lfirst, rfirst));
            break;
        case Parse::TT_NotEqual:
            /*
             * r1 = r1 ^ r2
             * r1 = 0 < r1 (unsigned)
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::xorr_instrc, lfirst, lfirst, rfirst));
            this->append_instruction(VirtMac::RInstruction(VirtMac::setlur_instrc, 0, lfirst, lfirst));
        case Parse::TT_CompareEqual:
            /*
             * r1 = r1 ^ r2
             * r1 = r1 <= 0 (unsigned) 
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::xorr_instrc, lfirst, lfirst, rfirst));
            this->append_instruction(VirtMac::SInstruction(VirtMac::setleur_instrc, lfirst, 0, lfirst));
            break;
        case Parse::TT_GreaterThan:
            /*
             * r1 = r2 < r1 (signed)
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::setlsr_instrc, rfirst, lfirst, lfirst));
            break;
        case Parse::TT_GreaterThanEqual:
            /*
             * r1 = r2 < r1 (signed)
             * 
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::setlesr_instrc, rfirst, lfirst, lfirst));
            break;
        case Parse::TT_LessThan:
            /*
             * r1 = r1 < r2 (signed)
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::setlsr_instrc, lfirst, rfirst, lfirst));
            break;
        default:
            Error("compiler (binary node)", "unhandled node");
            break;
        }
    } else {
        /**
         * 1. all instructions will be modified by the ternary instruction to the correct jump address
         *
         * 2. jumps do the \b reverse of what their token actually says because they jump on \b false and not on \b true
         *
         */
        switch (node->token)
        {
        case 0:
            case Parse::TT_NotEqual:
            /*
             * if (r1 == r2) -> pc += (offset, signed)
             */
            this->append_instruction(VirtMac::SInstruction(VirtMac::je_instrc, lfirst, rfirst, 0));
            break;
        case Parse::TT_CompareEqual:
            /*
             * if (r1 != r2) -> pc += (offset, signed)
             */
            this->append_instruction(VirtMac::SInstruction(VirtMac::jne_instrc, lfirst, rfirst, 0));
            break;
        case Parse::TT_GreaterThan:
            /*
             * if (r1 <= r2)(signed) -> pc += (offset, signed)
             */
            this->append_instruction(VirtMac::SInstruction(VirtMac::jles_instrc, lfirst, rfirst, 0));
            break;
        case Parse::TT_GreaterThanEqual:
            /*
             * if (r1 < r2)(signed) -> pc += (offset, signed)
             */
            this->append_instruction(VirtMac::SInstruction(VirtMac::jls_instrc, lfirst, rfirst, 0));
            break;
        case Parse::TT_LessThan:
            /*
             * if (r2 <= r1)(signed) -> pc += (offset, signed)
             */
            this->append_instruction(VirtMac::SInstruction(VirtMac::jles_instrc, rfirst, lfirst, 0));
            break;
        case Parse::TT_LessThanEqual:
            /*
             * if (r2 < r1)(signed) -> pc += (offset, signed)
             */
            this->append_instruction(VirtMac::SInstruction(VirtMac::jls_instrc, rfirst, lfirst, 0));
            break;
        default:
            Error("compiler (binary node, ternary parent)", "impossible error, as getting to here should not be possible (probably)");
            break;
        }
    }
    this->clear_register(rfirst);
    used_rchild.erase(used_rchild.begin());
    used_lchild.insert(used_lchild.end(), used_rchild.begin(), used_rchild.end());
    return used_lchild;
}

return_t Assembler::assemble_ternary(Parse::TernaryNode *node) {
    /**
     * ternary assembly:
     * 
     * j{!condition} false_op_label
     * true_op_label:
     * ...
     * jmp cond_end
     * false_op_label:
     * ...
     * cond_end:
     * code continues...
     */
    uint64_t jumps;
    auto registers = node->condition->type == Parse::Binary ? this->assemble_binary(node->condition->cget<Parse::BinaryNode>(), true) : this->assemble(node->condition);
    
    if (node->condition->type != Parse::Binary) {
        // makes a jump that tests for `{result} != 0`, which is basically what C like languages do
        this->append_instruction(VirtMac::SInstruction(VirtMac::jne_instrc, registers[0], 0, 0));
        
    }

    auto & jump_instr = this->instructions[this->instructions.size() - 1];
    auto true_used_registers = this->assemble(node->trueop);
    
}


return_t Assembler::assemble_lambda(Parse::LambdaNode *node)
{
    this->add_function(node->name);
    this->assemble(node->expression);
    return {};
}

return_t Assembler::assemble(const Parse::node_pointer &node)
{
    switch (node->type)
    {
    case Parse::Integer:
        this->assemble_number(node->cget<Parse::NumberNode>());
        break;
    case Parse::Unary:
        this->assemble_unary(node->cget<Parse::UnaryNode>());
        break;
    case Parse::Lambda:
        this->assemble_lambda(node->cget<Parse::LambdaNode>());
        break;
    default:
        Error("compiler", "unhandled node");
    }
    return {};
}

uint64_t Assembler::add_function(std::string_view name)
{
    if (this->symbols.count(name))
    {
        std::cerr << "symbol : " << name << '\n';
        Error("compiler", "multiple definitions of ^");
    }
    this->symbols[name] = this->dot;
    return this->dot;
}

Compiler::byte_container Assembler::compile()
{
    for (auto &&node : this->parsed_nodes)
    {
        this->assemble(node);
    }
    auto file = std::ofstream("out.bin", std::ios_base::binary);
    file.write(reinterpret_cast<char *>(&this->instructions[0]), this->instructions.size() * sizeof(uint64_t));
    file.close();
    return this->instructions;
}