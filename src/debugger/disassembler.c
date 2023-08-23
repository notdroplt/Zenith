#include <virtualmachine/zenithvm.h>

#define get_instruction(instruction) instructions[instruction]
static const char * const instructions[] = {
    "and", "and", "xor", "xor", "or", "or", "count", "??", "lls",
    "lls", "lrs", "lrs", "als", "als", "ars", "ars", "add", "add",
    "sub", "sub", "umul", "umul", "smul", "smul", "udiv", "udiv", "sdiv",
    "sdiv", "??", "??", "??", "??", "ld [byte]", "ld [half]", "ld [word]", "ld [dword]",
    "st [byte]", "st [half]", "st [word]", "st [dword]", "jal", "jal", "je", "jne", "jlu",
    "jls", "jleu", "jles", "setleu", "setleu", "setles", "setles", "setlu", "setlu",
    "setls", "setls", "lui", "auipc", "ecall", "ebreak"};

void disassemble_instruction(union instruction_t inst)
{
    if (inst.ltype.opcode == jal_instrc || inst.ltype.opcode > setlsi_instrc)
    {
        printf("%s r%d, %lu\n", get_instruction(inst.ltype.opcode), inst.ltype.r1, (uint64_t)inst.ltype.immediate);
    }
    else if (inst.rtype.opcode % 2 == 0 && !(inst.rtype.opcode >= ld_byte_instrc && inst.rtype.opcode <= st_dwrd_instrc))
    {
        printf("%s r%d, r%d, r%u\n", get_instruction(inst.ltype.opcode), inst.rtype.rd, inst.rtype.r1, inst.rtype.r2);
    }
    else
    {
        printf("%s r%d, r%d, %lu\n", get_instruction(inst.ltype.opcode), inst.stype.rd, inst.stype.r1,
               (uint64_t)inst.stype.immediate);
    }
}

size_t fpread(void *buf, size_t size, size_t n, long off, FILE *fpointer)
{
    return fseek(fpointer, off, SEEK_SET) ? 0 : fread(buf, size, n, fpointer);
}

void disassemble_file(const char *filename)
{
    union instruction_t inst;
    long dot = 0;
    long maxsize = 0;
    FILE *fpointer = fopen(filename, "re");
    if (!fpointer)
    {
        return;
    }
    (void)fseek(fpointer, 0, SEEK_END);
    maxsize = ftell(fpointer);
    (void)fseek(fpointer, 0, SEEK_SET);

    if (!maxsize || maxsize % sizeof(uint64_t))
    {
        (void)fclose(fpointer);
        (void)fprintf(stderr, "file has an invalid size %ld (either size < 1 or size %% 8 > 0) \n", maxsize);
        return;
    }

    for (; dot < maxsize; dot += sizeof(uint64_t))
    {
        if (!fpread(&inst, sizeof(union instruction_t), 1, dot, fpointer) || inst.ltype.opcode > ebreak_instrc)
        {
            (void)fprintf(stderr, "invalid read at block 0x%04lX\n", dot);
            return;
        }
        printf("0x%08lx | %016lX | ", dot, inst.value);
        disassemble_instruction(inst);
    }

    (void)fclose(fpointer);
}
