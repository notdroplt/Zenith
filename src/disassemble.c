#include "virtualmachine.h"

const char *get_instruction(uint64_t instruction)
{
    switch (instruction)
    {
    case andr_instrc:
    case andi_instrc:
        return "and";

    case xorr_instrc:
    case xori_instrc:
        return "xor";

    case orr_instrc:
    case ori_instrc:
        return "or";

    case llsr_instrc:
    case llsi_instrc:
        return "lls";

    case lrsr_instrc:
    case lrsi_instrc:
        return "lrs";

    case alsr_instrc:
    case alsi_instrc:
        return "als";

    case arsr_instrc:
    case arsi_instrc:
        return "ars";

    case addr_instrc:
    case addi_instrc:
        return "add";

    case subr_instrc:
    case subi_instrc:
        return "sub";

    case umulr_instrc:
    case umuli_instrc:
        return "umul";

    case smulr_instrc:
    case smuli_instrc:
        return "smul";

    case udivr_instrc:
    case udivi_instrc:
        return "udiv";

    case sdivr_instrc:
    case sdivi_instrc:
        return "sdiv";

    case ld_byte_instrc:
        return "ld [byte]";
    case ld_half_instrc:
        return "ld [half]";
    case ld_word_instrc:
        return "ld [word]";
    case ld_dwrd_instrc:
        return "ld [dwrd]";

    case st_byte_instrc:
        return "st [byte]";
    case st_half_instrc:
        return "st [half]";
    case st_word_instrc:
        return "st [word]";
    case st_dwrd_instrc:
        return "st [dwrd]";

    case jal_instrc:
    case jalr_instrc:
        return "jal";

    case je_instrc:
        return "je";
    case jne_instrc:

    case jlu_instrc:
        return "jlu";
    case jls_instrc:
        return "jls";
    case jleu_instrc:
        return "jleu";
    case jles_instrc:
        return "jles";

    case setleur_instrc:
    case setleui_instrc:
        return "setleu";

    case setlesr_instrc:
    case setlesi_instrc:
        return "setles";

    case setlur_instrc:
    case setlui_instrc:
        return "setlu";

    case setlsr_instrc:
    case setlsi_instrc:
        return "setls";

    case lui_instrc:
        return "lui";

    case auipc_instrc:
        return "auipc";

    case ecall_instrc:
        return "ecall";

    case ebreak_instrc:
        return "ebreak";
    default:
        return "??";
    }
    
}

void disassemble(const char * filename) {
    uint64_t dot = 0, maxsize = 0;
    union instruction_t instruction;
    FILE * fp = fopen(filename, "r");
    if (!fp) return;
    
    fseek(fp, 0, SEEK_END);
    maxsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (!maxsize || maxsize % 8) {
        fprintf(stderr, "file has an invalid size %ld\n", maxsize);
        fclose(fp);
        return;
    }

    while(dot < maxsize) {
        if (!fread(&instruction, sizeof(union instruction_t), 1, fp)) {
            fprintf(stderr, "invalid read at block 0x%lx\n", dot);
            fclose(fp);
        }
        if (instruction.ltype.opcode == 0x28 || instruction.ltype.opcode > 0x37) 
            printf("0x%04lx: %s r%d, %ld\n", dot, get_instruction(instruction.ltype.opcode), instruction.ltype.r1, (uint64_t)instruction.ltype.immediate);
        else if (instruction.rtype.opcode % 2 == 0 && !(instruction.rtype.opcode >= 0x20 && instruction.rtype.opcode <= 0x2F))
            printf("0x%04lx: %s r%d, r%d, r%d\n", dot, get_instruction(instruction.ltype.opcode), instruction.rtype.rd, instruction.rtype.r1, instruction.rtype.r2);
        else 
            printf("0x%04lx: %s r%d, r%d, %ld\n", dot, get_instruction(instruction.ltype.opcode), instruction.stype.rd, instruction.stype.r1, (uint64_t)instruction.stype.immediate);
        dot += 8;
    }
    fclose(fp);
} 