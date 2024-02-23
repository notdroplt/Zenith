#include "group1.h"
#pragma GCC diagnostic push
// reading from a bitfield does not go out of bounds
#pragma GCC diagnostic ignored "-Wanalyzer-out-of-bounds"

sninstr_func(addr_instrc)
{
    thread->registers[instr.rtype.rd] = thread->registers[instr.rtype.r1] + thread->registers[instr.rtype.r2];
}

sninstr_func(addi_instrc)
{
    thread->registers[instr.stype.rd] = thread->registers[instr.stype.r1] + instr.stype.immediate;
}

sninstr_func(subr_instrc)
{
    thread->registers[instr.rtype.rd] = thread->registers[instr.rtype.r1] - thread->registers[instr.rtype.r2];
}

sninstr_func(subi_instrc)
{
    thread->registers[instr.stype.rd] = thread->registers[instr.stype.r1] - instr.stype.immediate;
}

sninstr_func(orr_instrc)
{
    thread->registers[instr.rtype.rd] = thread->registers[instr.rtype.r1] | thread->registers[instr.rtype.r2];
}

sninstr_func(ori_instrc)
{
    thread->registers[instr.stype.rd] = thread->registers[instr.stype.r1] | instr.stype.immediate;
}

sninstr_func(not_instrc)
{
    thread->registers[instr.rtype.rd] = ~thread->registers[instr.rtype.r1];
}

sninstr_func(cnt_instrc)
{

    thread->registers[instr.stype.rd] = __builtin_popcount(thread->registers[instr.stype.r1]);
}

sninstr_func(llsr_instrc)
{
    thread->registers[instr.rtype.rd] = thread->registers[instr.rtype.r1] >> thread->registers[instr.rtype.r2];
}

sninstr_func(llsi_instrc)
{
    thread->registers[instr.stype.rd] = thread->registers[instr.stype.r1] >> instr.stype.immediate;
}

sninstr_func(lrsr_instrc)
{
    thread->registers[instr.rtype.rd] = thread->registers[instr.rtype.r1] << thread->registers[instr.rtype.r2];
}

sninstr_func(lrsi_instrc)
{
    thread->registers[instr.stype.rd] = thread->registers[instr.stype.r1] << instr.stype.immediate;
}

sninstr_func(alsr_instrc)
{
    thread->registers[instr.rtype.rd] = (int64_t)thread->registers[instr.rtype.r1] >> (int64_t)thread->registers[instr.rtype.r2];
}

sninstr_func(alsi_instrc)
{
    thread->registers[instr.stype.rd] = (int64_t)thread->registers[instr.stype.r1] >> (int64_t)instr.stype.immediate;
}

sninstr_func(arsr_instrc)
{
    thread->registers[instr.rtype.rd] = (int64_t)thread->registers[instr.rtype.r1] << (int64_t)thread->registers[instr.rtype.r2];
}

sninstr_func(arsi_instrc)
{
    thread->registers[instr.stype.rd] = (int64_t)thread->registers[instr.stype.r1] << (int64_t)instr.stype.immediate;
}

#pragma GCC diagnostic pop
