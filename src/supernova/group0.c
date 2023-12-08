#include "group1.h"

sninstr_func(andr_instrc)
{
    thread->registers[instr.rtype.rd] = thread->registers[instr.rtype.r1] & thread->registers[instr.rtype.r2];
}

sninstr_func(andi_instrc)
{
    thread->registers[instr.stype.rd] = thread->registers[instr.stype.r1] & instr.stype.immediate;
}

sninstr_func(xorr_instrc)
{
    thread->registers[instr.rtype.rd] = thread->registers[instr.rtype.r1] ^ thread->registers[instr.rtype.r2];
}

sninstr_func(xori_instrc)
{
    thread->registers[instr.stype.rd] = thread->registers[instr.stype.r1] ^ instr.stype.immediate;
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
#if __has_builtin(__builtin_popcountl)
    thread->registers[instr.stype.rd] = __builtin_popcountl(thread->registers[instr.stype.r1]);
#else
  int count = 0;
  uint64_t val = thread->registers[instr.stype.r1];
  for (; val != 0; val &= val - 1)
    count++;
  thread->registers[instr.stype.rd] = count;
#endif
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
