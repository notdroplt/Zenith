#include "virtualmachine.h"

union instruction_t fetch_instruction(register struct thread_t * thread) {
    assert(thread->program_counter + 8 < thread->memory_size);
    thread->program_counter += 8;
    return *(union instruction_t*)(thread->memory + thread->program_counter - 8);
}

uint8_t fetch8(register struct thread_t * thread, register uint64_t address) {
    assert(address + 1 < thread->memory_size);
    return *(uint8_t*)(thread->memory + address);
}

uint16_t fetch16(register struct thread_t * thread, register uint64_t address) {
    assert(address + 2< thread->memory_size);
    return *(uint16_t*)(thread->memory + address);
}

uint32_t fetch32(register struct thread_t * thread, register uint64_t address) {
    assert(address + 4< thread->memory_size);
    return *(uint32_t*)(thread->memory + address);
}

uint64_t fetch64(register struct thread_t * thread, register uint64_t address) {
    assert(address + 8< thread->memory_size);
    return *(uint64_t*)(thread->memory + address);
}


void set_memory_8(register struct thread_t * thread, register uint64_t address, register uint8_t value) {
    assert(address - 1 < thread->memory_size);
    *(uint8_t*)(thread->memory + address) = value;
}

void set_memory_16(register struct thread_t * thread, register uint64_t address, register uint16_t value) {
    assert(address - 2< thread->memory_size);
    *(uint16_t*)(thread->memory + address) = value;
}

void set_memory_32(register struct thread_t * thread, register uint64_t address, register uint32_t value) {
    assert(address - 4< thread->memory_size);
    *(uint32_t*)(thread->memory + address) = value;
}

void set_memory_64(register struct thread_t * thread, register uint64_t address, register uint64_t value) {
    assert(address - 8< thread->memory_size);
    *(uint64_t*)(thread->memory + address) = value;
}

void exec_instruction(register struct thread_t * thread) {
    register union instruction_t instruction = fetch_instruction(thread);
    thread->registers[0] = 0;
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
    /**/
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
    case setur_instrc:
        thread->registers[instruction.rtype.rd] = thread->registers[instruction.rtype.r1] < thread->registers[instruction.rtype.r2];
        break;
    case setui_instrc:
        thread->registers[instruction.stype.rd] = thread->registers[instruction.stype.r1] < instruction.stype.immediate;
        break;
    case setsr_instrc:
        thread->registers[instruction.rtype.rd] = (int64_t)thread->registers[instruction.rtype.r1] < (int64_t)thread->registers[instruction.rtype.r2];
        break;
    case setsi_instrc:
        thread->registers[instruction.stype.rd] = (int64_t)thread->registers[instruction.stype.r1] < (int64_t)instruction.stype.immediate;
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
    case lui_instrc:
        thread->registers[instruction.ltype.r1] = instruction.ltype.immediate << 18;
        break;
    case auipc_instrc:
        thread->registers[instruction.ltype.r1] = thread->program_counter + (instruction.ltype.immediate << 18);
        break;
    case ecall_instrc:
        /* nothing for now */
        break;
    case ebreak_instrc:
        print_status(thread);
        break;
    default:
        break;
    }
}

void print_status(register struct thread_t * thread) {
    printf("registers:\n");

    for (register int i = 0; i < 32; i += 4)
    {
        printf("| r%02d: 0x%016lX | r%02d: 0x%016lX | r%02d: 0x%016lX | r%02d: 0x%016lX |\n",
        i, thread->registers[i], i + 1, thread->registers[i + 1], i + 2, thread->registers[i + 2], i + 3, thread->registers[i + 3]);
    }
}

#define min(a, b) ((a) > (b) ? b : a)

void run(const char * filename) {
    FILE * fp;
    struct thread_t thread;
    uint8_t *mem = (uint8_t*) malloc(1 << 10);
    if (!mem) return;    

    fp = fopen(filename, "r");
    if (!fp) {
        free(mem);
        return;
    }
    fseek(fp, 0, SEEK_END);
    register long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if(!fread(mem, min(1 << 10, size), 1, fp)) {
        free(mem);
        if (!fclose(fp)) {
            perror("pain");
        }
    }

    for (int i = 0; i < min(1 << 10, size); i++)
    {
        printf(" %02X", mem[i]);
    }
    

    thread.program_counter = 0;
    thread.memory = mem;
    exec_instruction(&thread);
    print_status(&thread);
}