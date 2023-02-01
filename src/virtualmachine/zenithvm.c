#include "zenithvm.h"

union instruction_t RInstruction(const uint8_t opcode, const uint8_t r1, const uint8_t r2, const uint8_t rd)
{
    union instruction_t instrc;
    instrc.rtype.opcode = opcode;
    instrc.rtype.r1 = r1;
    instrc.rtype.r2 = r2;
    instrc.rtype.rd = rd;
    instrc.rtype.pad = 0;
    return instrc;
}

union instruction_t SInstruction(const uint8_t opcode, const uint8_t r1, const uint8_t rd, const uint64_t immediate)
{
    union instruction_t instrc;
    instrc.stype.opcode = opcode;
    instrc.stype.r1 = r1;
    instrc.stype.rd = rd;
    instrc.stype.immediate = immediate;
    return instrc;
}

union instruction_t LInstruction(const uint8_t opcode, const uint8_t r1, const uint64_t immediate)
{
    union instruction_t instrc;
    instrc.ltype.opcode = opcode;
    instrc.ltype.r1 = r1;
    instrc.ltype.immediate = immediate;
    return instrc;
}

void exec_instruction(register struct thread_t *thread)
{
    register union instruction_t instruction;
    if (thread->halt_sig)
        return;
    instruction.value = fetch64(thread, thread->program_counter);
    thread->program_counter += 8;
    switch (instruction.rtype.opcode)
    {
    case andr_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] & thread->registers[instruction.rtype.r2];
        break;
    case andi_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] & instruction.stype.immediate;
        break;
    case xorr_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] ^ thread->registers[instruction.rtype.r2];
        break;
    case xori_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] ^ instruction.stype.immediate;
        break;
    /**/
    case orr_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] | thread->registers[instruction.rtype.r2];
        break;
    case ori_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] | instruction.stype.immediate;
        break;
    case 0x06:
    case 0x07:
        break;
    /**/
    case llsr_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] >> thread->registers[instruction.rtype.r2];
        break;
    case llsi_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] >> instruction.stype.immediate;
        break;
    case lrsr_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] << thread->registers[instruction.rtype.r2];
        break;
    case lrsi_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] << instruction.stype.immediate;
        break;
    /**/
    case alsr_instrc:
        thread->registers[instruction.rtype.rd] = (int64_t)thread->registers[instruction.rtype.r1] >> (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case alsi_instrc:
        thread->registers[instruction.stype.rd] = (int64_t)thread->registers[instruction.stype.r1] >> (int64_t)instruction.stype.immediate;
        break;
    case arsr_instrc:
        thread->registers[instruction.rtype.rd] = (int64_t)thread->registers[instruction.rtype.r1] << (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case arsi_instrc:
        thread->registers[instruction.stype.rd] = (int64_t)thread->registers[instruction.stype.r1] << (int64_t)instruction.stype.immediate;
        break;
    /**/
    case addr_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] + thread->registers[instruction.rtype.r2];
        break;
    case addi_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] + instruction.stype.immediate;
        break;
    case subr_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] - thread->registers[instruction.rtype.r2];
        break;
    case subi_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] - instruction.stype.immediate;
        break;
    /**/
    case umulr_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] * thread->registers[instruction.rtype.r2];
        break;
    case umuli_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] * instruction.stype.immediate;
        break;
    case smulr_instrc: 
        thread->registers[instruction.rtype.rd] = (int64_t)thread->registers[instruction.rtype.r1] * (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case smuli_instrc:
        thread->registers[instruction.stype.rd] = (int64_t)thread->registers[instruction.stype.r1] * (int64_t)instruction.stype.immediate;
        break;
    /* TODO: implement divide by zero exception*/
    case udivr_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] / thread->registers[instruction.rtype.r2];
        break;
    case udivi_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] / instruction.stype.immediate;
        break;
    case sdivr_instrc:
        thread->registers[instruction.rtype.rd] = (int64_t)thread->registers[instruction.rtype.r1] / (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case sdivi_instrc:
        thread->registers[instruction.stype.rd] = (int64_t)thread->registers[instruction.stype.r1] / (int64_t)instruction.stype.immediate;
        break;
    /**/
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
        break;
    /**/
    case ld_byte_instrc:
        thread->registers[instruction.stype.rd] = fetch8(thread, thread->registers[instruction.stype.r1] + instruction.stype.immediate);
        break;
    case ld_half_instrc:
        thread->registers[instruction.stype.rd] = fetch16(thread, thread->registers[instruction.stype.r1] + instruction.stype.immediate);
        break;
    case ld_word_instrc:
        thread->registers[instruction.stype.rd] = fetch32(thread, thread->registers[instruction.stype.r1] + instruction.stype.immediate);
        break;
    case ld_dwrd_instrc:
        thread->registers[instruction.stype.rd] = fetch64(thread, thread->registers[instruction.stype.r1] + instruction.stype.immediate);
        break;
    /**/
    case st_byte_instrc:
        set_memory_8(thread, thread->registers[instruction.stype.rd] + instruction.stype.immediate, thread->registers[thread->registers[instruction.stype.r1]]);
        break;
    case st_half_instrc:
        set_memory_16(thread, thread->registers[instruction.stype.rd] + instruction.stype.immediate, thread->registers[thread->registers[instruction.stype.r1]]);
        break;
    case st_word_instrc:
        set_memory_32(thread, thread->registers[instruction.stype.rd] + instruction.stype.immediate, thread->registers[thread->registers[instruction.stype.r1]]);
        break;
    case st_dwrd_instrc:
        set_memory_64(thread, thread->registers[instruction.stype.rd] + instruction.stype.immediate, thread->registers[thread->registers[instruction.stype.r1]]);
        break;
    /**/
    case jal_instrc:
        thread->registers[instruction.ltype.r1] = thread->program_counter + 8;
        thread->program_counter += instruction.ltype.immediate;
        break;
    case jalr_instrc:
        thread->registers[instruction.stype.rd] = thread->program_counter + 8;
        thread->program_counter += thread->registers[instruction.stype.rd] + instruction.stype.immediate;
        break;
    case je_instrc:
        if (thread->registers[instruction.stype.r1] == thread->registers[instruction.stype.rd])
            thread->program_counter += instruction.stype.immediate;
        break;
    case jne_instrc:
        if (thread->registers[instruction.stype.r1] != thread->registers[instruction.stype.rd])
            thread->program_counter += instruction.stype.immediate;
        break;
    /**/
    case jlu_instrc:
        if (thread->registers[instruction.stype.r1] < thread->registers[instruction.stype.rd])
            thread->program_counter += instruction.stype.immediate;
        break;
    case jls_instrc:
        if ((int64_t)thread->registers[instruction.stype.r1] < (int64_t)thread->registers[instruction.stype.rd])
            thread->program_counter += instruction.stype.immediate;
        break;
    case jleu_instrc:
        if (thread->registers[instruction.stype.r1] <= thread->registers[instruction.stype.rd])
            thread->program_counter += instruction.stype.immediate;
        break;
    case jles_instrc:
        if ((int64_t)thread->registers[instruction.stype.r1] <= (int64_t)thread->registers[instruction.stype.rd])
            thread->program_counter += instruction.stype.immediate;
        break;
    /**/
    case setlur_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] < thread->registers[instruction.rtype.r2];
        break;
    case setlui_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] < instruction.stype.immediate;
        break;
    case setlsr_instrc:
        thread->registers[instruction.rtype.rd] = (int64_t)thread->registers[instruction.rtype.r1] < (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case setlsi_instrc:
        thread->registers[instruction.stype.rd] = (int64_t)thread->registers[instruction.stype.r1] < (int64_t)instruction.stype.immediate;
        break;
    /**/
    case setleur_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] <= thread->registers[instruction.rtype.r2];
        break;
    case setleui_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] <= instruction.stype.immediate;
        break;
    case setlesr_instrc:
        thread->registers[instruction.rtype.rd] = (int64_t)thread->registers[instruction.rtype.r1] <= (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case setlesi_instrc:
        thread->registers[instruction.stype.rd] = (int64_t)thread->registers[instruction.stype.r1] <= (int64_t)instruction.stype.immediate;
        break;
    /**/
    case lui_instrc:
        thread->registers[instruction.ltype.r1] |= instruction.ltype.immediate << 18;
        break;
    case auipc_instrc:
        thread->registers[instruction.ltype.r1] = thread->program_counter + (instruction.ltype.immediate << 18);
        break;
    case ecall_instrc:
        switch (instruction.ltype.immediate)
        {
        case 1:
            thread->halt_sig = 1;
            break;
        default:
            break;
        }
        break;
    case ebreak_instrc:
        break;
    default:
        break;
    }
    thread->registers[0] = 0;
}

int run(const char *filename, int argc, char **argv, void (*debugger)(struct thread_t *thread))
{
    struct thread_t thread;
    register long size;
    FILE *fp;

    (void)argv;
    
    fp = fopen(filename, "r");
    if (!fp)
        return EXIT_FAILURE;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    fseek(fp, 0, SEEK_SET);

    thread.program_counter = 0;
    thread.memory = malloc(3 << 8); // give some working space (and align it too)
    thread.memory_size = 3 << 8;
    thread.registers[0] = 0;
    thread.registers[31] = argc;
    thread.halt_sig = 0;

    if (!thread.memory)
    {
        fprintf(stderr, "could not allocate %ld bytes of memory\n", thread.memory_size);
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (!fread(thread.memory, size, 1, fp))
    {
        fclose(fp);
        free(thread.memory);
        return EXIT_FAILURE;
    }

    // registers should NOT be initialized because code shouldn't rely on that

    if (debugger)
        debugger(&thread);
    else while (!thread.halt_sig)
        exec_instruction(&thread);

    return (int)thread.registers[1];
}