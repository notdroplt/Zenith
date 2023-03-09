#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <zenithvm.h>

#define color_old(old, new) ((old) > (new) ? Color_Green : ((new) > (old) ? Color_Red : ""))
#define color_new(old, new) ((old) < (new) ? Color_Green : ((new) < (old) ? Color_Red : ""))

void print_status(register struct thread_t *thread)
{
    register int i = 0;

    fprintf(stdout, "| inr: 0x%016lX | memory size: %ld bytes | halt signal: %d |\n", thread->program_counter, thread->memory_size, thread->halt_sig);
    for (; i < 32; i += 4)
        fprintf(stdout, "| r%02d: 0x%016lX | r%02d: 0x%016lX | r%02d: 0x%016lX | r%02d: 0x%016lX |\n",
                i, thread->registers[i], i + 1, thread->registers[i + 1], i + 2, thread->registers[i + 2], i + 3, thread->registers[i + 3]);
}

void print_diff_status(register struct thread_t *old_thread, register struct thread_t *new_thread)
{
    register int i = 0;

    fprintf(stdout, "| inr: 0x%016lX -> 0x%016lX | memory size: %ld bytes | halt signal: %d |\n",
            old_thread->program_counter, new_thread->program_counter, new_thread->memory_size, new_thread->halt_sig);
    for (; i < 32; i += 4)
        fprintf(stdout, "| r%02d:%s 0x%016lX " Color_Reset "->%s 0x%016lX " Color_Reset "| r%02d:%s 0x%016lX " Color_Reset "->%s 0x%016lX " Color_Reset "|\n"
                        "| r%02d:%s 0x%016lX " Color_Reset "->%s 0x%016lX " Color_Reset "| r%02d:%s 0x%016lX " Color_Reset "->%s 0x%016lX " Color_Reset "|\n",
                i + 0, color_old(old_thread->registers[i + 0], new_thread->registers[i + 0]), old_thread->registers[i + 0],
                color_new(old_thread->registers[i + 0], new_thread->registers[i + 0]), new_thread->registers[i + 0],

                i + 1, color_old(old_thread->registers[i + 1], new_thread->registers[i + 1]), old_thread->registers[i + 1],
                color_new(old_thread->registers[i + 1], new_thread->registers[i + 1]), new_thread->registers[i + 1],

                i + 2, color_old(old_thread->registers[i + 2], new_thread->registers[i + 2]), old_thread->registers[i + 2],
                color_new(old_thread->registers[i + 2], new_thread->registers[i + 2]), new_thread->registers[i + 2],

                i + 3, color_old(old_thread->registers[i + 3], new_thread->registers[i + 3]), old_thread->registers[i + 3],
                color_new(old_thread->registers[i + 3], new_thread->registers[i + 3]), new_thread->registers[i + 3]);
}

void debugger_func(struct thread_t *thread)
{
    struct thread_t * saved_state = NULL;
    char action[16] = {'\0'};
    // memcpy(&saved_state, thread, sizeof(struct thread_t));

    saved_state = calloc(1, sizeof(struct thread_t));
    if (!saved_state) {
        ZenithOutOfMemory;
        return;
    }

    while (1)
    {
        fputs("zdb> ", stdout);
        if (scanf(" %15s", action) == EOF)
        {
            free(saved_state);
            fputs("exit\n", stdout);
            break;
        }
        if (strcmp(action, "info") == 0 || strcmp(action, "help") == 0)
            fputs("Zenith debugger\n"
                  "commands to the debugger are listed below:\n===\n"
                  "step\t: steps one instruction in the virtual machine\n"
                  "state\t: display information about thread registers and program counter\n"
                  "instrc\t: print current instruction\n"
                  "continue: run code until the halt flag is set\n"
                  "save\t: save curret thread state (only one save so it does overwrite older ones)\n"
                  "rollback: rollback to last saved state\n"
                  "diff\t: print differences betweed saved and current state\n"
                  "\t| values go \"green -> red\" on decrease, \"red -> green\" on increase, and white if they are the same \n",
                  stdout);
        else if (strcmp(action, "step") == 0)
        {
            exec_instruction(thread);
            printf("0x%08lX | ", thread->program_counter);
            disassemble_instruction(*(union instruction_t *)(thread->memory + thread->program_counter));
        }
        else if (strcmp(action, "state") == 0)
            print_status(thread);
        else if (strcmp(action, "continue") == 0)
        {
            while (!thread->halt_sig)
                exec_instruction(thread);
            fprintf(stdout, "stopping on halt at:\n0x%08lX: ", thread->program_counter);
            disassemble_instruction(*(union instruction_t *)(thread->memory + thread->program_counter));
        }

        else if (strcmp(action, "instrc") == 0)
        {
            fprintf(stdout, "0x%08lX: ", thread->program_counter);
            disassemble_instruction(*(union instruction_t *)(thread->memory + thread->program_counter));
        }
        else if (strcmp(action, "save") == 0)
            memcpy(saved_state, thread, sizeof(struct thread_t));
        else if (strcmp(action, "rollback") == 0)
            memcpy(thread, saved_state, sizeof(struct thread_t));
        else if (strcmp(action, "diff") == 0)
            print_diff_status(saved_state, thread);
        else if (strcmp(action, "exit") == 0){
            free(saved_state);
            break;
        }
        else
            fprintf(stdout, "\"%s\" is not recognized as a command.\n", action);

        memset(action, 0, 16);
    }

}
