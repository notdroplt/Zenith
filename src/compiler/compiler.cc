#include <zenith.hpp>
using Assembler = Compiler::Assembler;
using return_t = Compiler::return_t;

Assembler::Assembler(std::vector<Parse::node_pointer> &nodes) : parsed_nodes(nodes),
dot(0), root_index(0), registers()
{
    registers[31] = 0;
}

void Assembler::append_instruction(const VirtMac::instruction_t instruction)
{
    this->instructions.push_back(instruction);
    dot += 8;
}

Parse::LambdaNode * Assembler::get_function(uint64_t index){
    if (index > this->parsed_nodes.size() || this->parsed_nodes.at(index)->type != Parse::Lambda)
        return nullptr;
    return this->parsed_nodes.at(index)->cget<Parse::LambdaNode>();
}

Parse::ExpressionNode * Assembler::get_constant(uint64_t index){
    if (index > this->parsed_nodes.size() || this->parsed_nodes.at(index)->type != Parse::Expression)
        return nullptr;
    return this->parsed_nodes.at(index)->cget<Parse::ExpressionNode>();
}

uint64_t Assembler::assemble_number(Parse::NumberNode *node)
{
    if (node->value == 0.0 && node->number == 0) {
        return 0; // uses registers, becomes faster
    }
    uint64_t reg_index = this->request_register();

    if (reg_index == -1U) {
        return -1;
    }
    // goes only if number actually needs it
    if (node->number > 0x400000000000)
    {
        this->append_instruction(VirtMac::LInstruction(VirtMac::lui_instrc, reg_index, (node->number & (uint64_t)-1LL) >> 18));
    }
    // set lower number as `lower_num + 0` -> rd
    this->append_instruction(VirtMac::SInstruction(VirtMac::addi_instrc, 0, reg_index, node->number & ((1LLU << 46) - 1)));

    return reg_index;
}

return_t Assembler::assemble_unary(Parse::UnaryNode *node)
{
    auto used_regs = this->assemble(node->value);

    if (used_regs[0] == -1U) {
        return used_regs;
    }

    switch (node->token)
    {
    case Parse::TT_Minus:
        // r1 = 0 - r1
        this->append_instruction(VirtMac::SInstruction(VirtMac::subi_instrc, used_regs[0], used_regs[0], 0));
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
    auto used_rchild = this->assemble(node->right);

    auto & lfirst = used_lchild[0];
    auto & rfirst = used_rchild[0];

    if (rfirst == -1U || lfirst == -1U) {
        return used_lchild;
    }

    if (!isJumping) {
        switch (node->token)
        {
        case Parse::TT_Plus:
            /*
             * r1 = r1 + r2
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::addr_instrc, lfirst, rfirst, lfirst));
            break;
        case Parse::TT_Minus:
            /* 
             * r1 = r1 - r2 
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::subr_instrc, lfirst, rfirst, lfirst));
            break;
        case Parse::TT_Multiply:
            /*
             * r1 = r1 * r2 (signed)
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::smulr_instrc, lfirst, rfirst, lfirst));
            break;
        case Parse::TT_Divide:
            /*
             * r1 = r1 / r2 (signed)
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::udivr_instrc, lfirst, rfirst, lfirst));
            break;
        case Parse::TT_NotEqual:
            /*
             * r1 = r1 ^ r2
             * r1 = 0 < r1 (unsigned)
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::xorr_instrc, lfirst, rfirst, lfirst));
            this->append_instruction(VirtMac::RInstruction(VirtMac::setlur_instrc, 0, lfirst, lfirst));
            break;
        case Parse::TT_CompareEqual:
            /*
             * r1 = r1 ^ r2
             * r1 = r1 <= 0 (unsigned) 
             */
            this->append_instruction(VirtMac::RInstruction(VirtMac::xorr_instrc, lfirst, rfirst, lfirst));
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
             * r1 = r2 <= r1 (signed)
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
            return {-1LU};
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
            return {-1LU};
        }
    }
    this->clear_register(rfirst);
    used_rchild.erase(used_rchild.begin());
    used_lchild.insert(used_lchild.end(), used_rchild.begin(), used_rchild.end());
    return used_lchild;
}

uint64_t Assembler::assemble_identifier(Parse::StringNode * node) {

    return_t registers;
    if (!this->table.count(node->value) && !this->table[this->get_function(this->root_index)->name].entries.count(node->value))
        Error("compiler (identifier)", "variable not found");
    else if (this->table.count(node->value) && this->table[this->get_function(this->root_index)->name].entries.count(node->value))
        Error("compiler (identifier)", "multiple *invalid* definitions of same variable");
    else if (this->table[this->get_function(this->root_index)->name].entries.count(node->value))
        return this->table[this->get_function(this->root_index)->name].entries[node->value];
    else if (this->table.count(node->value))
        return this->table[node->value].dot;
    else 
        Error("compiler (identifier)", "unimplemented path: global identifier"); 
    return -1; 
}

return_t Assembler::assemble_lambda(Parse::LambdaNode *node)
{
    return_t regs = {register_status::used, register_status::used};
    for (uint32_t i = 0; i < node->arg_count; ++i) {
        auto &arg = node->args[i];
        if (arg->type != Parse::Identifier) {
            Error("Compiler (function arguments)", "expected argument to be a name");
            return {-1LU};
        }
        auto reg_index = this->request_register(true);
        if (reg_index == -1) {
            return {-1LU};
        }
        this->table[node->name].entries[arg->cget<Parse::StringNode>()->value] = reg_index;
        regs.push_back(reg_index);
    }
    
    this->table[node->name].dot = this->dot;
    this->table[node->name].arg_count = node->arg_count;
    this->entry_point = this->dot;
    auto used = this->assemble(node->expression);
    if (used[0] == -1U) {
        return used;
    }
    if (node->name == "main") {
        this->append_instruction(VirtMac::LInstruction(VirtMac::ecall_instrc, 0, 1));
    } else {
        this->append_instruction(VirtMac::SInstruction(VirtMac::jalr_instrc, 0, 31, 0));
    } 

    for (auto && entry : this->table[node->name].entries) {
        this->clear_register(entry.second);
    }

    return regs;
}

return_t Assembler::assemble_call(Parse::CallNode * node) {
    auto expr = this->assemble(node->expr);

    symbol_table_entry sym_entry;

    sym_entry.dot = -1;
    for (auto && pair : this->table) 
        if (pair.second.dot == expr[0]) { // key searching so there isnt need for another table
            sym_entry = pair.second;
            break;
        }

    if (sym_entry.dot == (uint64_t)-1) 
        Error("compiler (call node)", "function not found");
    
    if (sym_entry.arg_count != node->args_size) 
        Error("compiler (call node)", "argument size mismatch");
    
    for (uint8_t arg_counter = 30; arg_counter < 30 - node->args_size; --arg_counter) {

        auto reg = this->request_register(false, arg_counter);
        if (reg == -1) {
            Error("compiler (call node)", "could not request register for argument");
            return {-1LU};
        }
        this->assemble(node->args[30 - arg_counter]);
    }

    auto return_reg = this->request_register();
    
    int64_t offset = this->dot - sym_entry.dot;

    if (offset >= 1 << 18)    
        this->append_instruction(VirtMac::LInstruction(VirtMac::auipc_instrc, return_reg, (offset >> 18)));
    this->append_instruction(VirtMac::SInstruction(VirtMac::jalr_instrc, return_reg, return_reg, (offset & ((1 << 18) - 1))));
    
    return {};
}

return_t Assembler::assemble_ternary(Parse::TernaryNode * node){
    return_t used_conditions;

    if (node->condition->type == Parse::Binary) 
        this->assemble_binary(node->condition->cget<Parse::BinaryNode>(), true);
    else 
        // checks `cond != 0`, which is what c-like languages do
        this->append_instruction(VirtMac::SInstruction(VirtMac::je_instrc, this->assemble(node->condition)[0], 0, 0));
   
    auto idx = this->instructions.size() - 1;
    uint64_t dot_v = this->dot;  

    auto status_cond = this->registers;

    auto used_true = this->assemble(node->trueop);

    if (dot_v == this->dot) {
        // if the node is just a variable
        this->append_instruction(VirtMac::RInstruction(VirtMac::addr_instrc, used_true[0], 0, used_true[0]));
    }
    auto status_true = this->registers;

    this->registers = status_cond;

    this->append_instruction(VirtMac::SInstruction(VirtMac::jal_instrc, 0, 0, 0));
    this->instructions[idx].stype.immediate = this->dot - dot_v;

    idx = this->instructions.size() - 1;
    dot_v = this->dot;

    auto used_false = this->assemble(node->falseop);
    if (dot_v == this->dot) {
        // if the node is just a variable
        this->append_instruction(VirtMac::RInstruction(VirtMac::addr_instrc, used_false[0], 0, used_false[0]));
    }
    this->registers |= status_true; // because they are set on *used*, `or`ing registers will always set more
                                    // also, because most functions are only one-liners, they most of the time will use the same
                                    // amount of registers on either paths, so this part of the code will not have effects most of the times

    this->instructions[idx].ltype.immediate = this->dot - dot_v;

    return used_true;

    
}
return_t Assembler::assemble(const Parse::node_pointer &node)
{
    switch (node->type)
    {
    case Parse::Integer:
        return {this->assemble_number(node->cget<Parse::NumberNode>())};
    case Parse::Identifier:
        return {this->assemble_identifier(node->cget<Parse::StringNode>())};
    case Parse::Unary:
        return this->assemble_unary(node->cget<Parse::UnaryNode>());
    case Parse::Binary:
        return this->assemble_binary(node->cget<Parse::BinaryNode>(), false);
    case Parse::Ternary:
        return this->assemble_ternary(node->cget<Parse::TernaryNode>());
    case Parse::Lambda:
        return this->assemble_lambda(node->cget<Parse::LambdaNode>());
    case Parse::Call:
        return this->assemble_call(node->cget<Parse::CallNode>());
    default:
        Error("compiler", "unhandled node");
        return {-1LU};
    }
}

Compiler::byte_container Assembler::compile()
{
    for (auto &&node : this->parsed_nodes)
    {
        if (this->assemble(node)[0] == -1U) break;
        
        ++this->root_index;
    }
    
    return this->instructions;
}