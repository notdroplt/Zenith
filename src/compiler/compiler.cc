#include "compiler.hpp"
#include <vector>

using namespace Compiling;
using namespace Parsing;

Compiler::Compiler(std::vector<nodep> nodes) : _nodes(std::move(nodes)), _registers(~(0b111U << 29))
{
}

returns<comp_status> Compiler::compile_number(const nodep &number)
{
    auto val_int = number->as<NumberNode>().value<uint64_t>();
    auto val_float = number->as<NumberNode>().value<double>();
    if (val_float)
        return make_unexpected<returns<comp_status>>("assembly", "floating point numbers are not yet supported");

    // numbers are always constant
    return make_expected<returns<comp_status>, uint64_t>(val_int.value());
}

returns<comp_status> Compiler::compile_unary(const nodep &node)
{
    auto &unary = node->as<UnaryNode>();
    auto compiled = this->compile(unary.node());
    if (!compiled)
        return compiled;

    if (std::holds_alternative<uint64_t>(*compiled))
    {
        auto constant = std::get<uint64_t>(*compiled);

        switch (unary.op())
        {
        case TT_Minus:
            constant = -constant;
            break;
        case TT_Not:
            constant = !constant;
            break;
        /* lets give them a chance although they aren't lvalues */
        case TT_Increment:
            ++constant;
            break;
        case TT_Decrement:
            --constant;
            break;
        default:
            return make_unexpected<returns<comp_status>>("syntax", "undefined operation on given token");
        }

        return make_expected<returns<comp_status>, uint64_t>(constant);
    }

    auto registers = std::get<reg_status>(*compiled);

    if (registers.status().all())
        return make_expected<returns<comp_status>, reg_status>(registers);

    auto index = registers.index();

    instruction_t inst;

    switch (unary.op())
    {
    case TT_Minus:
        inst = RInstruction(subi_instrc, index, 0, index);
        break;
    case TT_Not:
        inst = SInstruction(setgui_instrc, index, index, 0);
        break;
    case TT_Increment:
        inst = SInstruction(addi_instrc, index, index, 1);
        break;
    case TT_Decrement:
        inst = SInstruction(subi_instrc, index, index, 1);
        break;
    default:
        return make_unexpected<returns<comp_status>>("syntax", "undefined unary operation on given token");
    }

    this->append_instruction(inst);

    return make_expected<returns<comp_status>, reg_status>(registers);
}

returns<comp_status> Compiler::compile_binary(const nodep &node, bool jumping)
{
    auto &binary = node->as<BinaryNode>();

    auto lchild = this->compile(binary.left());

    if (!lchild)
        return lchild;

    auto rchild = this->compile(binary.right());

    if (!rchild)
        return rchild;

    if (
        std::holds_alternative<uint64_t>(*lchild) &&
        std::holds_alternative<uint64_t>(*rchild))
    {
        auto lconst = std::get<uint64_t>(*lchild);
        auto rconst = std::get<uint64_t>(*rchild);

        switch (binary.op())
        {
        case TT_Plus:
            lconst += rconst;
            break;
        case TT_Minus:
            lconst -= rconst;
            break;
        case TT_Multiply:
            lconst *= rconst;
            break;
        case TT_Divide:
            if (!rconst)
                return make_unexpected<returns<comp_status>>("compiling", "division by a constant zero");
            lconst /= rconst;
            break;
        case TT_NotEqual:
            lconst = lconst != rconst;
            break;
        case TT_CompareEqual:
            lconst = lconst == rconst;
            break;
        case TT_GreaterThan:
            lconst = lconst > rconst;
            break;
        case TT_GreaterThanEqual:
            lconst = lconst >= rconst;
            break;
        case TT_LessThan:
            lconst = lconst < rconst;
            break;
        case TT_LessThanEqual:
            lconst = lconst <= rconst;
            break;
        default:
            /* in theory */
            __builtin_unreachable();
        }
        return make_expected<returns<comp_status>, uint64_t>(lconst);
    }

    instruction_t inst = instruction_t{.value = 0};

    if (
        !std::holds_alternative<uint64_t>(*lchild) &&
        std::holds_alternative<uint64_t>(*rchild) &&
        (!jumping || std::get<uint64_t>(*rchild) < 1LLU << 46))
    {
        auto lreg = std::get<reg_status>(*lchild).index();
        auto rconst = std::get<uint64_t>(*rchild);

        switch (binary.op())
        {
        case TT_Plus:
            inst = SInstruction(addi_instrc, lreg, lreg, rconst);
            break;
        case TT_Minus:
            inst = SInstruction(subi_instrc, lreg, lreg, rconst);
            break;
        case TT_Multiply:
            inst = SInstruction(umuli_instrc, lreg, lreg, rconst);
            break;
        case TT_Divide:
            if (!rconst)
                return make_unexpected<returns<comp_status>>("compiling", "division by a constant zero");
            inst = SInstruction(udivi_instrc, lreg, lreg, rconst);
            break;
        case TT_NotEqual:
            this->append_instruction(SInstruction(xori_instrc, lreg, lreg, rconst));
            inst = RInstruction(setgui_instrc, lreg, 0, lreg);
            break;
        case TT_CompareEqual:
            this->append_instruction(SInstruction(xori_instrc, lreg, lreg, rconst));
            inst = RInstruction(setleur_instrc, 0, lreg, lreg);
            break;
        case TT_GreaterThan:
            inst = SInstruction(setgui_instrc, lreg, lreg, rconst);
            break;
        case TT_LessThan:
            inst = SInstruction(setleui_instrc, lreg, lreg, rconst);
            break;
        default:
            __builtin_unreachable();
        }

        if (inst.value)
        {
            this->append_instruction(inst);
            return make_expected<returns<comp_status>, reg_status>(lreg);
        }
    }

    returns<uint8_t> scratch_register;
    if (std::holds_alternative<uint64_t>(*rchild) || std::holds_alternative<uint64_t>(*rchild))
    {
        uint64_t v;
        if (std::holds_alternative<uint64_t>(*rchild))
            v = std::get<uint64_t>(*rchild);
        else
            v = std::get<uint64_t>(*lchild);

        scratch_register = this->_registers.alloc();
        if (!scratch_register)
            return make_unexpected<returns<comp_status>>(scratch_register.error());

        if (v > 1LL << 46)
        {
            constexpr auto shift = 12;
            constexpr auto mask = ((1LLU << 52) - 1) << shift;
            constexpr auto remask = (1 << (shift + 1)) - 1;

            this->append_instruction(LInstruction(lui_instrc, *scratch_register, (v & mask) >> shift));
            this->append_instruction(SInstruction(ori_instrc, 0, *scratch_register, v & remask));
        }
    }

    auto lreg = std::holds_alternative<uint64_t>(*lchild) ? *scratch_register : std::get<reg_status>(*lchild).index();
    auto rreg = std::holds_alternative<uint64_t>(*rchild) ? *scratch_register : std::get<reg_status>(*rchild).index();

    if (jumping)
    {
        switch (binary.op())
        {
        case TT_Plus:
            this->append_instruction(RInstruction(addr_instrc, lreg, rreg, lreg));
            goto default_jump_case;
        case TT_Minus:
            this->append_instruction(RInstruction(subr_instrc, lreg, rreg, lreg));
            goto default_jump_case;
        case TT_Multiply:
            this->append_instruction(RInstruction(umulr_instrc, lreg, rreg, lreg));
            goto default_jump_case;
        case TT_Divide:
            this->append_instruction(RInstruction(udivr_instrc, lreg, rreg, lreg));
            goto default_jump_case;
        /* jumping ON FALSE (we don't have likely/unlikely yet)*/
        case TT_NotEqual:
            inst = SInstruction(je_instrc, lreg, rreg, 0);
            break;
        case TT_CompareEqual:
            inst = SInstruction(je_instrc, lreg, rreg, 0);
            break;
        case TT_GreaterThan:
            inst = SInstruction(jleu_instrc, lreg, rreg, 0);
            break;
        case TT_GreaterThanEqual:
            inst = SInstruction(jgu_instrc, rreg, lreg, 0);
            break;
        case TT_LessThan:
            inst = SInstruction(jleu_instrc, rreg, lreg, 0);
            break;
        case TT_LessThanEqual:
            inst = SInstruction(jleu_instrc, lreg, rreg, 0);
            break;
        default:
            __builtin_unreachable();
        }
        goto end;
    default_jump_case:
        inst = SInstruction(jne_instrc, lreg, 0, 0);
    end:
    }

    switch (binary.op())
    {
    case TT_Plus:
        inst = RInstruction(addr_instrc, lreg, rreg, 0);
        break;
    case TT_Minus:
        inst = RInstruction(subr_instrc, lreg, rreg, 0);
        break;
    case TT_Multiply:
        inst = RInstruction(subr_instrc, lreg, rreg, 0);
        break;
    case TT_Divide:
        inst = RInstruction(subr_instrc, lreg, rreg, 0);
        break;
    case TT_NotEqual:
        inst = SInstruction(je_instrc, lreg, rreg, 0);
        break;
    case TT_CompareEqual:
        inst = SInstruction(je_instrc, lreg, rreg, 0);
        break;
    case TT_GreaterThan:
        inst = SInstruction(jleu_instrc, lreg, rreg, 0);
        break;
    case TT_GreaterThanEqual:
        inst = SInstruction(jgu_instrc, rreg, lreg, 0);
        break;
    case TT_LessThan:
        inst = SInstruction(jleu_instrc, rreg, lreg, 0);
        break;
    case TT_LessThanEqual:
        inst = SInstruction(jleu_instrc, lreg, rreg, 0);
        break;
    default:
        __builtin_unreachable();
    }

    this->append_instruction(inst);
    this->_registers.free(rreg);

    return make_expected<returns<comp_status>, reg_status>(lreg);
}

returns<comp_status> Compiler::compile_identifier(const nodep &node)
{
    auto id = node->as<StringNode>();
    if (node->type() != Identifier)
        return make_unexpected<returns<comp_status>>("syntax", "expected identifier");

    auto val = this->_current_context[id.value()];

    if (!val)
        return make_unexpected<returns<comp_status>>(val.error());

    if (std::holds_alternative<region_entry>(*val))
        return std::get<region_entry>(*val).allocated_at;

    return std::get<lambda_entry>(*val).allocated_at;
}

returns<comp_status> Compiler::compile_lambda(const nodep &node)
{
    auto &lambda = node->as<LambdaNode>();

    context lambda_context = context(&this->_global_context);
    lambda_entry current_entry;
    if (lambda.args())
    {
        if (lambda.args()->type() == Identifier)
        {
            auto &arg = lambda.args()->as<StringNode>();

            /* we can be sure that requesting a register here is going to work */
            lambda_context.add_to_context(arg.value(), *this->_registers.alloc());
        }
        else
        {
            auto & args = lambda.args()->as<ListNode>().nodes();
            for (auto &&argument : args)
            {
                region_entry argument_entry;
                if (argument->type() != Identifier)
                    return make_unexpected<returns<comp_status>>("syntax", "expected an identifier on lambda argument list");

                auto reg = this->_registers.alloc();

                if (!reg) return make_unexpected<returns<comp_status>>(reg.error());

                argument_entry.allocated_at = *reg;
                argument_entry.constant = false;
                argument_entry.is_register = true;

                current_entry.arguments[argument->as<StringNode>().value()]
            }
        }
    }

argument_end:
}