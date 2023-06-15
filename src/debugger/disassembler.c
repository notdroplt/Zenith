#include "zenithvm.h"

#define get_instruction(instruction) instructions[instruction]
static const char *instructions[] = {
    "and",       "and",       "xor",       "xor",        "or",     "or",        "count",     "??",        "lls",
    "lls",       "lrs",       "lrs",       "als",        "als",    "ars",       "ars",       "add",       "add",
    "sub",       "sub",       "umul",      "umul",       "smul",   "smul",      "udiv",      "udiv",      "sdiv",
    "sdiv",      "??",        "??",        "??",         "??",     "ld [byte]", "ld [half]", "ld [word]", "ld [dword]",
    "st [byte]", "st [half]", "st [word]", "st [dword]", "jal",    "jal",       "je",        "jne",       "jlu",
    "jls",       "jleu",      "jles",      "setleu",     "setleu", "setles",    "setles",    "setlu",     "setlu",
    "setls",     "setls",     "lui",       "auipc",      "ecall",  "ebreak"};

void disassemble_instruction(union instruction_t inst) {
    if (inst.ltype.opcode == 0x28 || inst.ltype.opcode > 0x37)
        printf("%s r%d, %ld\n", get_instruction(inst.ltype.opcode), inst.ltype.r1, (uint64_t)inst.ltype.immediate);
    else if (inst.rtype.opcode % 2 == 0 && !(inst.rtype.opcode >= 0x20 && inst.rtype.opcode <= 0x2F))
        printf("%s r%d, r%d, r%d\n", get_instruction(inst.ltype.opcode), inst.rtype.rd, inst.rtype.r1, inst.rtype.r2);
    else
        printf("%s r%d, r%d, %ld\n", get_instruction(inst.ltype.opcode), inst.stype.rd, inst.stype.r1,
               (uint64_t)inst.stype.immediate);
}

void disassemble_file(const char *filename) {
    union instruction_t inst;
    uint64_t dot = 0, maxsize = 0;
    FILE *fp = fopen(filename, "r");
    if (!fp) return;
    fseek(fp, 0, SEEK_END);
    maxsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (!maxsize || maxsize % 8) {
        fprintf(stderr, "file has an invalid size %ld (either < 1 or %% 8 > 0) \n", maxsize);
        fclose(fp);
        return;
    }

    for (; dot < maxsize; dot += 8)
        if (!fread(&inst, sizeof(union instruction_t), 1, fp) || inst.ltype.opcode > 0x3B)
            fprintf(stderr, "invalid read at block 0x%lX\n", dot);
        else {
            printf("0x%08lx | %016lX | ", dot, inst.value);
            disassemble_instruction(inst);
        }

    fclose(fp);
}
