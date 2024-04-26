#include "supernova.h"
#include "supernova_private.h"
#include <cstdint>
#include <cstdio>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void set_memory_8(register struct thread_t *thread, register uint64_t address, register uint8_t value)
{
    *(thread->memory + address) = value;
}

void set_memory_16(register struct thread_t *thread, register uint64_t address, register uint16_t value)
{
    *(uint16_t *)(thread->memory + address) = value;
}

void set_memory_32(register struct thread_t *thread, register uint64_t address, register uint32_t value)
{
    *(uint32_t *)(thread->memory + address) = value;
}

void set_memory_64(register struct thread_t *thread, register uint64_t address, register uint64_t value)
{
    *(uint64_t *)(thread->memory + address) = value;
}

supernova_api union instruction_t RInstruction(const uint8_t opcode, const uint8_t r1, const uint8_t r2, const uint8_t rd)
{
    union instruction_t instrc;
    instrc.rtype.opcode = opcode;
    instrc.rtype.r1 = r1;
    instrc.rtype.r2 = r2;
    instrc.rtype.rd = rd;
    instrc.rtype.pad = 0;
    return instrc;
}

supernova_api union instruction_t SInstruction(const uint8_t opcode, const uint8_t r1, const uint8_t rd, const uint64_t immediate)
{
    union instruction_t instrc;
    instrc.stype.opcode = opcode;
    instrc.stype.r1 = r1;
    instrc.stype.rd = rd;
    instrc.stype.immediate = immediate;
    return instrc;
}

supernova_api union instruction_t LInstruction(const uint8_t opcode, const uint8_t r1, const uint64_t immediate)
{
    union instruction_t instrc;
    instrc.ltype.opcode = opcode;
    instrc.ltype.r1 = r1;
    instrc.ltype.immediate = immediate;
    return instrc;
}

#define fetch(size, thread, address) (*(uint##size##_t *)((thread)->memory + (address)))

static void hwpush64(register struct thread_t *thread, uint64_t value)
{
    fetch(64, thread, thread->registers[1]) = value;
    thread->registers[1] -= 8;
}

static uint64_t hwpop64(register struct thread_t *thread)
{
    uint64_t val = fetch(64, thread, thread->registers[1]);
    thread->registers[1] += 8;
    return val;
}

static int64_t ssextend(uint64_t number)
{
    return (int64_t)number & -1LU << 46 ? number | 0xffff800000000000 : number;
}

static void pcall_minus_one(register struct thread_t *thread, uint64_t interrupt_space, uint64_t function_switch)
{
    switch (thread->registers[29])
    {
    case 0:
        if (thread->registers[28] == 0) {
            thread->registers[31] = 2;
            thread->registers[30] = thread->model->interrupt_count;
        } else if (thread->registers[28] == 1) {
            thread->int_vector = thread->registers[31];
        }
        break;
    case 1:
        thread->registers[31] = 0;
        break;
    case 2:

    
    default:
        break;
    }
}

static void dispatch_pcall(register struct thread_t *thread, uint64_t pcall_number)
{
    if (pcall_number == -1LLU)
    {
        pcall_minus_one(thread, thread->registers[29], thread->registers[28]);
        return;
    }

    for (int i = 0; i < 32; i++)
    {
        hwpush64(thread, thread->registers[i]);
    }

    hwpush64(thread, thread->program_counter);

    thread->program_counter = fetch(64, thread, thread->int_vector + pcall_number * 8);
}

void exec_instruction(register struct thread_t *thread)
{
    register union instruction_t instruction = {0};
    if (thread->halt_sig)
    {
        return;
    }
    instruction.value = fetch(64, thread, thread->program_counter);
    thread->program_counter += 8;

    switch (instruction.rtype.opcode)
    {
    case andr_instrc: sninstr_func_call(andr_instrc, thread, instruction); break;
    case andi_instrc: sninstr_func_call(andi_instrc, thread, instruction); break;
    case xorr_instrc: sninstr_func_call(xorr_instrc, thread, instruction); break;
    case xori_instrc: sninstr_func_call(xori_instrc, thread, instruction); break;
    /**/
    case orr_instrc: sninstr_func_call(orr_instrc, thread, instruction); break;
    case ori_instrc: sninstr_func_call(ori_instrc, thread, instruction); break;
    case not_instrc: sninstr_func_call(not_instrc, thread, instruction); break;
    case cnt_instrc: sninstr_func_call(cnt_instrc, thread, instruction); break;
    /**/
    case llsr_instrc: sninstr_func_call(llsr_instrc, thread, instruction); break; 
    case llsi_instrc: sninstr_func_call(llsi_instrc, thread, instruction); break; 
    case lrsr_instrc: sninstr_func_call(lrsr_instrc, thread, instruction); break; 
    case lrsi_instrc: sninstr_func_call(lrsi_instrc, thread, instruction); break; 
    /**/
    case alsr_instrc: sninstr_func_call(alsr_instrc, thread, instruction); break;
    case alsi_instrc: sninstr_func_call(alsi_instrc, thread, instruction); break;
    case arsr_instrc: sninstr_func_call(arsr_instrc, thread, instruction); break;
    case arsi_instrc: sninstr_func_call(arsi_instrc, thread, instruction); break;
    /**/
    case addr_instrc:
        thread->registers[instruction.rtype.rd] =
            thread->registers[instruction.rtype.r1] + thread->registers[instruction.rtype.r2];
        break;
    case addi_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] + instruction.stype.immediate;
        break;
    case subr_instrc:
        thread->registers[instruction.rtype.rd] =
            thread->registers[instruction.rtype.r1] - thread->registers[instruction.rtype.r2];
        break;
    case subi_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] - instruction.stype.immediate;
        break;
    /**/
    case umulr_instrc:
        thread->registers[instruction.rtype.rd] =
            thread->registers[instruction.rtype.r1] * thread->registers[instruction.rtype.r2];
        break;
    case umuli_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] * instruction.stype.immediate;
        break;
    case smulr_instrc:
        thread->registers[instruction.rtype.rd] =
            (int64_t)thread->registers[instruction.rtype.r1] * (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case smuli_instrc:
        thread->registers[instruction.stype.rd] =
            (int64_t)thread->registers[instruction.stype.r1] * (int64_t)instruction.stype.immediate;
        break;
    /* TODO: implement divide by zero exception*/
    case udivr_instrc:
        if (!thread->registers[instruction.rtype.r2])
        {
            goto division_by_zero_pcall;
        }
        thread->registers[instruction.rtype.rd] =
            thread->registers[instruction.rtype.r1] / thread->registers[instruction.rtype.r2];
        break;
    case udivi_instrc:
        if (!thread->registers[instruction.rtype.r2])
        {
            goto division_by_zero_pcall;
        }
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] / instruction.stype.immediate;
        break;
    case sdivr_instrc:
        if (!thread->registers[instruction.rtype.r2])
        {
            goto division_by_zero_pcall;
        }
        thread->registers[instruction.rtype.rd] =
            (int64_t)thread->registers[instruction.rtype.r1] / (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case sdivi_instrc:
        if (!thread->registers[instruction.rtype.r2])
        {
            goto division_by_zero_pcall;
        }
        thread->registers[instruction.stype.rd] =
            (int64_t)thread->registers[instruction.stype.r1] / (int64_t)instruction.stype.immediate;
        break;
    /**/
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
        break;
    /**/
    case ld_byte_instrc:
        thread->registers[instruction.stype.rd] =
            fetch(8, thread, thread->registers[instruction.stype.r1] + instruction.stype.immediate);
        break;
    case ld_half_instrc:
        thread->registers[instruction.stype.rd] =
            fetch(16, thread, thread->registers[instruction.stype.r1] + instruction.stype.immediate);
        break;
    case ld_word_instrc:
        thread->registers[instruction.stype.rd] =
            fetch(32, thread, thread->registers[instruction.stype.r1] + instruction.stype.immediate);
        break;
    case ld_dwrd_instrc:
        thread->registers[instruction.stype.rd] =
            fetch(64, thread, thread->registers[instruction.stype.r1] + instruction.stype.immediate);
        break;
    /**/
    case st_byte_instrc:
        fetch(8, thread, thread->registers[instruction.stype.rd] + instruction.stype.immediate) =
            thread->registers[thread->registers[instruction.stype.r1]];
        break;
    case st_half_instrc:
        fetch(16, thread, thread->registers[instruction.stype.rd] + instruction.stype.immediate) =
            thread->registers[thread->registers[instruction.stype.r1]];
        break;
    case st_word_instrc:
        fetch(32, thread, thread->registers[instruction.stype.rd] + instruction.stype.immediate) =
            thread->registers[thread->registers[instruction.stype.r1]];
        break;
    case st_dwrd_instrc:
        fetch(64, thread, thread->registers[instruction.stype.rd] + instruction.stype.immediate) =
            thread->registers[thread->registers[instruction.stype.r1]];
        break;
    /**/
    case jal_instrc:
        thread->registers[instruction.ltype.r1] = thread->program_counter + 8;
        thread->program_counter += ssextend(instruction.ltype.immediate);
        break;
    case jalr_instrc:
        thread->registers[instruction.stype.rd] = thread->program_counter + 8;
        thread->program_counter += thread->registers[instruction.stype.rd] + ssextend(instruction.stype.immediate);
        break;
    case je_instrc:
        if (thread->registers[instruction.stype.r1] == thread->registers[instruction.stype.rd])
        {
            thread->program_counter += ssextend(instruction.stype.immediate);
        }
        break;
    case jne_instrc:
        if (thread->registers[instruction.stype.r1] != thread->registers[instruction.stype.rd])
        {
            thread->program_counter += ssextend(instruction.stype.immediate);
        }
        break;
    /**/
    case jgu_instrc:
        if (thread->registers[instruction.stype.r1] > thread->registers[instruction.stype.rd])
        {
            thread->program_counter += ssextend(instruction.stype.immediate);
        }
        break;
    case jgs_instrc:
        if ((int64_t)thread->registers[instruction.stype.r1] > (int64_t)thread->registers[instruction.stype.rd])
        {
            thread->program_counter += ssextend(instruction.stype.immediate);
        }
        break;
    case jleu_instrc:
        if (thread->registers[instruction.stype.r1] <= thread->registers[instruction.stype.rd])
        {
            thread->program_counter += ssextend(instruction.stype.immediate);
        }
        break;
    case jles_instrc:
        if ((int64_t)thread->registers[instruction.stype.r1] <= (int64_t)thread->registers[instruction.stype.rd])
        {
            thread->program_counter += ssextend(instruction.stype.immediate);
        }
        break;
    /**/
    case setgur_instrc:
        thread->registers[instruction.rtype.rd] =
            thread->registers[instruction.rtype.r1] > thread->registers[instruction.rtype.r2];
        break;
    case setgui_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] > instruction.stype.immediate;
        break;
    case setgsr_instrc:
        thread->registers[instruction.rtype.rd] =
            (int64_t)thread->registers[instruction.rtype.r1] > (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case setgsi_instrc:
        thread->registers[instruction.stype.rd] =
            (int64_t)thread->registers[instruction.stype.r1] > (int64_t)instruction.stype.immediate;
        break;
    /**/
    case setleur_instrc:
        thread->registers[instruction.rtype.rd] =
            thread->registers[instruction.rtype.r1] <= thread->registers[instruction.rtype.r2];
        break;
    case setleui_instrc:
        thread->registers[instruction.stype.rd] =
            thread->registers[instruction.stype.r1] <= instruction.stype.immediate;
        break;
    case setlesr_instrc:
        thread->registers[instruction.rtype.rd] =
            (int64_t)thread->registers[instruction.rtype.r1] <= (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case setlesi_instrc:
        thread->registers[instruction.stype.rd] =
            (int64_t)thread->registers[instruction.stype.r1] <= (int64_t)instruction.stype.immediate;
        break;
    /**/
    case lui_instrc:
        thread->registers[instruction.ltype.r1] |= instruction.ltype.immediate << 18;
        break;
    case auipc_instrc:
        thread->registers[instruction.ltype.r1] = thread->program_counter + (instruction.ltype.immediate << 17);
        break;
    case pcall_instrc:
        if (instruction.ltype.immediate == 0)
        {
            /* set pcall 0:0 as "no pcall 0 defined"*/
            thread->registers[31] = 0;
        }
        break;
    case pbreak_instrc:
        break;
    default:
        break;
    }
    thread->registers[0] = 0;
    return;
division_by_zero_pcall:
    dispatch_pcall(thread, 0);
}

int run(const char *filename, int argc, char **argv, void (*debugger)(struct thread_t *thread))
{
    struct thread_t *thread = NULL;
    register long size = 0;
    FILE *fp = NULL;
    uint64_t ret_val = 0;

    (void)argv;
    if (filename)
    {
        fp = fopen(filename, "re");
        if (!fp)
        {
            goto destructor_1;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }
    else
    {
        return -1;
    }

    thread = malloc(sizeof(struct thread_t));
    if (!thread)
    {
        goto destructor_2;
    }

    thread->program_counter = 0;
    thread->memory = malloc(UINT16_MAX); /* give some working space (and align it too) */
    thread->memory_size = UINT16_MAX;
    thread->registers[0] = 0;
    thread->registers[31] = argc;
    thread->halt_sig = 0;
    thread->int_vector = 0;

    if (!thread->memory)
    {
        (void)fprintf(stderr, "could not allocate %ld bytes of memory\n", thread->memory_size);
        goto destructor_3;
    }

    if (!fread(thread->memory, size, 1, fp))
    {
        goto destructor_4;
    }

    (void)fclose(fp);

    /* registers should NOT be initialized because code shouldn't rely on that */

    if (debugger)
    {
        debugger(thread);
    }
    else
    {
        while (!thread->halt_sig)
        {
            exec_instruction(thread);
        }
    }

    ret_val = thread->registers[1];

    destroy_thread(thread);

    return (int)ret_val;
destructor_4:
    free(thread->memory);
destructor_3:
    free(thread);
destructor_2:
    fclose(fp);
destructor_1:
    return EXIT_FAILURE;
}

void destroy_thread(struct thread_t *thread)
{
    free(thread->memory);
    free(thread);
}