#include <zenithvm.h>
#include <stdlib.h>

static int status = 0;


int test_memory_fetch (struct thread_t * thread) {
    uint8_t rand1 = rand();
    uint16_t rand2 = rand();
    uint32_t rand3 = rand() + rand();
    uint64_t rand4 = (uint64_t)rand() << 32L | rand();

    *(uint8_t *)thread->memory = rand1;

    if (fetch8(thread, 0x0000) != rand1) {
        fprintf(stderr, "function `fetch8` returned %d instead of %d\n", fetch8(thread, 0x0000), rand1);
        return 1;
    }

    *(uint16_t *)thread->memory = rand2;

    if (fetch16(thread, 0x0000) != rand2) {
        fprintf(stderr, "function `fetch16` returned %d instead of %d\n", fetch16(thread, 0x0000), rand2);
        return 1;
    }

    *(uint32_t *)thread->memory = rand3;

    if (fetch32(thread, 0x0000) != rand3) {
        fprintf(stderr, "function `fetch32` returned %d instead of %d\n", fetch32(thread, 0x0000), rand3);
        return 1;
    }

    *(uint64_t *)thread->memory = rand4;

    if (fetch64(thread, 0x0000) != rand4) {
        fprintf(stderr, "function `fetch64` returned %ld instead of %ld\n", fetch64(thread, 0x0000), rand4);
        return 1;
    }

    printf("functions `fetch8`, `fetch16`, `fetch32`, and `fetch64` passed tests successfully\n");

    return 0;
}

int test_memory_set(struct thread_t * thread) {
    uint8_t rand1 = rand();
    uint16_t rand2 = rand();
    uint32_t rand3 = rand() + rand();
    uint64_t rand4 = (uint64_t)rand() << 32L | rand();

    set_memory_8(thread, 0x0000, rand1);
    if (*(uint8_t *)thread->memory != rand1) {
        fprintf(stderr, "function `set_memory_8` set %d instead of %d\n", *(uint8_t *)thread->memory, rand1);
        return 1;
    }

    set_memory_16(thread, 0x0000, rand2);
    if (*(uint16_t *)thread->memory != rand2) {
        fprintf(stderr, "function `set_memory_16` set %d instead of %d\n", *(uint16_t *)thread->memory, rand2);
        return 1;
    }

    set_memory_32(thread, 0x0000, rand3);
    if (*(uint32_t *)thread->memory != rand3) {
        fprintf(stderr, "function `set_memory_32` set %d instead of %d\n", *(uint32_t *)thread->memory, rand3);
        return 1;
    }

    set_memory_64(thread, 0x0000, rand4);
    if (*(uint64_t *)thread->memory != rand4) {
        fprintf(stderr, "function `set_memory_64` set %ld instead of %ld\n", *(uint64_t *)thread->memory, rand4);
        return 1;
    }

    printf("functions `set_memory_8`, `set_memory_16`, `set_memory_32`, and `set_memory_64` passed tests successfully\n");

    return 0;
}

void test_debugger(struct thread_t * thread) {
    if((status = test_memory_fetch(thread))) return;
    if((status = test_memory_set(thread))) return;
}

int main(void) {
    run(NULL, 0, NULL, test_debugger);
    return status;    
}
