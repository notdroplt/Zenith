#ifndef HOME_DROPLT_PROJECTS_ZENITH_SRC_SUPERNOVA_SUPERNOVA_PRIVATE_H
#define HOME_DROPLT_PROJECTS_ZENITH_SRC_SUPERNOVA_SUPERNOVA_PRIVATE_H

#pragma once
#ifndef ZENITH_SUPERNOVA_VM_H
#define ZENITH_SUPERNOVA_VM_H 1

#include "supernova.h"

/**
 * \defgroup virtset Virtual Instruction Set Emulation
 *
 * \brief all the instruction prefixes used on the emulated vm cpu
 * 
 * @{
 */

/**
 * @brief defines a header for a runnable code on the virtual machine
 *
 * @note all positions should \b not consider the header offset
 */
struct virtmacheader_t {
    /** file magic "Zenithvm" */
    uint64_t magic;        

    /** current header version */
    uint64_t version;      

    /** initialized data size */
    uint64_t data_size;    

    /** data section start on the file */
    uint64_t data_start;   

    /** data section start on memory */
    uint64_t data_offset;  

    /** runnable code size */
    uint64_t code_size;    

    /** code section start on the file */
    uint64_t code_start;   

    /** code section start on memory */
    uint64_t code_offset;  

    /** code entry point */
    uint64_t entry_point;

    /** padding value */
    uint64_t pad;

} __attribute__((aligned(128)));

/**
 * @brief fetches one byte from a memory address
 *
 * @param [in, out] thread current thread
 * @param address address to fetch
 * @return fetched value
 * 
 * @private @memberof thread_t
 */
auto fetch8(struct thread_t *thread, uint64_t address) -> uint8_t __attribute__((hot));

/**
 * @brief fetches two bytes from a memory address
 *
 * @param [in, out] thread current thread
 * @param address address to fetch
 * @return fetched value
 * 
 * @private @memberof thread_t
 */
auto fetch16(struct thread_t *thread, uint64_t address) -> uint16_t __attribute__((hot));

/**
 * @brief fetches four bytes from a memory address
 *
 * @param [in, out] thread current thread
 * @param address address to fetch
 * @return fetched value
 * 
 * @private @memberof thread_t
 */
auto fetch32(struct thread_t *thread, uint64_t address) -> uint32_t __attribute__((hot));

/**
 * @brief fetches eight bytes from a memory address
 *
 * @param [in, out] thread current thread
 * @param address address to fetch
 * @return fetched value
 * 
 * @private @memberof thread_t
 */
auto fetch64(struct thread_t *thread, uint64_t address) -> uint64_t __attribute__((hot));

/**
 * @brief sets one byte of memory at a specified address
 *
 * @param [in, out] thread current thread
 * @param address address to set
 * @param value value to set at address
 * 
 * @private @memberof thread_t
 */
void set_memory_8(struct thread_t *thread, uint64_t address, uint8_t value);

/**
 * @brief sets two bytes of memory at a specified address
 *
 * @param [in, out] thread current thread
 * @param address address to set
 * @param value value to set at address
 * 
 * @private @memberof thread_t
 */
void set_memory_16(struct thread_t *thread, uint64_t address, uint16_t value);

/**
 * @brief sets four bytes of memory at a specified address
 *
 * @param [in, out] thread current thread
 * @param address address to set
 * @param value value to set at address
 * 
 * @private @memberof thread_t
 */
void set_memory_32(struct thread_t *thread, uint64_t address, uint32_t value);

/**
 * @brief sets eight bytes of memory at a specified address
 *
 * @param [in, out] thread current thread
 * @param address address to set
 * @param value value to set at address
 * 
 * @private @memberof thread_t
 */
void set_memory_64(struct thread_t *thread, uint64_t address, uint64_t value);

/**
 * @brief execute one instruction while in a thread
 *
 * @param [in, out] thread current thread object to emulate
 * 
 * @private @memberof thread_t
 */
void exec_instruction(struct thread_t *thread) __attribute__((hot));

/**
 * @brief prints the status of a thread to `stdout`
 *
 * @param [in, out] thread thread to print status from
 * 
 * @private @memberof thread_t
 */
void print_status(struct thread_t *thread);

/**
 * @brief run code from a file
 *
 * @param [in] filename file name to run
 * @param argc main's argc
 * @param [in] argv main's argv
 * @param debugger the debugger function
 *
 * @return exit code
 * 
 * @public @memberof thread_t
 */
auto run(const char *filename, int argc, char **argv, void (*debugger)(struct thread_t *)) -> int;

/**
 * @brief destroys a thread object
 *
 * @param thread thread to be destroyed
 * 
 * @public @memberof thread_t
 */
void destroy_thread(struct thread_t *thread);

/** @} */ /* end of group Virtual Instrucion Set Emulation */

#endif

#endif
