#include <zenithvm.h>

uint8_t fetch8(register struct thread_t *thread, register uint64_t address)
{
    return *(uint8_t *)(thread->memory + address);
}

uint16_t fetch16(register struct thread_t *thread, register uint64_t address)
{
    return *(uint16_t *)(thread->memory + address);
}

uint32_t fetch32(register struct thread_t *thread, register uint64_t address)
{
    return *(uint32_t *)(thread->memory + address);
}

uint64_t fetch64(register struct thread_t *thread, register uint64_t address)
{
    return *(uint64_t *)(thread->memory + address);
}

void set_memory_8(register struct thread_t *thread, register uint64_t address, register uint8_t value)
{
    *(uint8_t *)(thread->memory + address) = value;
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
