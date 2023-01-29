/**
 * @file virtualmachine.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief all virtual machine components (in C!)
 * @version 0.0.1
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef _virtual_machine_h
#define _virtual_machine_h 1



#include "platform.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#ifndef doxygen
/* no need to document these at it */
#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#endif

/**
 * \defgroup virtset Virtual Instruction Set Emulation
 *
 * \brief all the instruction prefixes used on the emulated vm cpu, all designed by me
 *
 * instructions follow something like risc-v, but with some patches and differences
 *
 * @{
 */

/**
 * @brief creates a uniquely "increasing" name for a reserved instruction,
 */
#define Reserved MACRO_CONCAT(reserved_instrc, __COUNTER__)

/**
 * @brief defines a header for a runnable code on the virtual machine
 *
 * @note all positions should \b not consider the header offset
 */
struct virtmacheader_t
{
    uint64_t magic;       //!< file magic "Zenithvm"
    uint64_t version;     //!< current header version
    uint64_t data_size;   //!< initialized data size
    uint64_t data_start;  //!< data section start on the file
    uint64_t data_offset; //!< data section start on memory
    uint64_t code_size;   //!< runnable code size
    uint64_t code_start;  //!< code section start on the file
    uint64_t code_offset; //!< code section start on memory
    uint64_t entry_point; //!< code entry point
    uint64_t pad;         //!< padding value
};


#define HEADER_MAG 0x6D766874696E655A /*!< defines header magic */

/**
 * \brief prefixes for every instruction
 *
 * \note yes this looks like risc-v, but i tried to make it like "minimal" instead of just "reduced"
 *
 * instruction types:
 *
 * R type (registers only) => register-register-register:
 * @code
 * [63-25][24-20][19-15][14-9][8-0]
 *    |      |      |     |      |
 *  [pad]  [r2]   [r1]   [rd] [opcode]
 * @endcode
 *
 * S type ("small" immediate) => register-register-immediate:
 *
 * @code
 * [63-20][19-15][14-9][8-0]
 *    |      |     |     |
 *  [imm]  [r1]  [rd] [opcode]
 * @endcode
 *
 * L type ("long" immediate) => register-immediate:
 *
 * @code
 * [63-15][14-9][8-0]
 *    |     |     |
 *  [imm]  [rd] [opcode]
 * @endcode
 *
 * base instructions are split into 4 groups (named after values at bits [5,4] ):
 *
 * - group 0: bitwise instructions, span opcodes `0x00` to `0x0F`,
 * - group 1: math instructions, span opcodes `0x10` to `0x1F`
 * - group 2: memory / control flow instructions, span opcodes `0x20` to `0x2F`
 * - group 3: other instructions, span opcodes `0x30` to `0x3F` (even if almost all of them are reserved)
 *
 *
 * on groups 0 and 1, the LSB defines if the instruction is an R type (when 0) or an S type (when 1)
 * group 2, has only type S and the `jal` instruction (which is L type)
 * group 3 is always L type (lmfao L)
 */
enum instruction_prefixes
{
    /* group 0*/
    andr_instrc = 0x00, /*!< `and r#, r#, r#` : R type */
    andi_instrc = 0x01, /*!< `and r#, r#, imm` : S type */
    xorr_instrc = 0x02, /*!< `xor r#, r#, r#` : R type */
    xori_instrc = 0x03, /*!< `xor r#, r#, imm` : S type */

    orr_instrc = 0x04, /*!< `or r#, r#, r#` : R type */
    ori_instrc = 0x05, /*!< `or r#, r#, imm` : S type */
    Reserved = 0x06,
    Reserved = 0x07,

    llsr_instrc = 0x08, /*!< `lls r#, r#, r#` : Rtype */
    llsi_instrc = 0x09, /*!< `lls r#, r#, imm` : S type */
    lrsr_instrc = 0x0A, /*!< `lrs r#, r#, r#` : Rtype */
    lrsi_instrc = 0x0B, /*!< `lrs r#, r#, imm` : S type */

    alsr_instrc = 0x0C, /*!< `als r#, r#, r#` : R type */
    alsi_instrc = 0x0D, /*!< `als r#, r#, imm` : S type */
    arsr_instrc = 0x0E, /*!< `ars r#, r#, r#` : R type */
    arsi_instrc = 0x0F, /*!< `ars r#, r#, imm` : S type */

    /* group 1 */
    addr_instrc = 0x10, /*!< `add r#, r#, r#` : R type */
    addi_instrc = 0x11, /*!< `add r#, r#, imm` : S type */
    subr_instrc = 0x12, /*!< `sub r#, r#, r#` : R type */
    subi_instrc = 0x13, /*!< `sub r#, r#, imm` : S type */

    umulr_instrc = 0x14, /*!< `umul r#, r#, r#` : R type */
    umuli_instrc = 0x15, /*!< `umul r#, r#, imm` : S type */
    smulr_instrc = 0x16, /*!< `smul r#, r#, r#` : R type */
    smuli_instrc = 0x17, /*!< `smul r#, r#, imm` : S type */

    udivr_instrc = 0x18, /*!< `udiv r#, r#, r#` : R type */
    udivi_instrc = 0x19, /*!< `udiv r#, r#, imm` : S type */
    sdivr_instrc = 0x1A, /*!< `sdiv r#, r#, r#` : R type */
    sdivi_instrc = 0x1B, /*!< `sdiv r#, r#, imm` : S type */

    Reserved = 0x1C, /*!< `setu r#, r#, r#` : R type */
    Reserved = 0x1D, /*!< `setu r#, r#, imm` : S type */
    Reserved = 0x1E, /*!< `setr r#, r#, r#` : R type */
    Reserved = 0x1F, /*!< `setr r#, r#, imm` : S type */

    /* group 2 */
    ld_byte_instrc = 0x20, /*!< `ldb r#, r#, imm` : S type */
    ld_half_instrc = 0x21, /*!< `ldh r#, r#, imm` : S type */
    ld_word_instrc = 0x22, /*!< `ldw r#, r#, imm` : S type */
    ld_dwrd_instrc = 0x23, /*!< `ldd r#, r#, imm` : S type */

    st_byte_instrc = 0x24, /*!< `stb r#, r#, imm` : S type */
    st_half_instrc = 0x25, /*!< `sth r#, r#, imm` : S type */
    st_word_instrc = 0x26, /*!< `stw r#, r#, imm` : S type */
    st_dwrd_instrc = 0x27, /*!< `std r#, r#, imm` : S type */

    jal_instrc = 0x28,  /*!< `jal r#, imm` : L type */
    jalr_instrc = 0x29, /*!< `jalr r#, r#, imm` : S type */
    je_instrc = 0x2A,   /*!< `je r#, r#, imm` : S type */
    jne_instrc = 0x2B,  /*!< `jne r#, r#, imm` : S type */

    jlu_instrc = 0x2C,  /*!< `jlu r#, r#, imm` : S type */
    jls_instrc = 0x2D,  /*!< `jls r#, r#, imm` : S type */
    jleu_instrc = 0x2E, /*!< `jleu r#, r#, imm` : S type */
    jles_instrc = 0x2F, /*!< `jles r#, r#, imm` : S type */

    /* other instructions, 0x30 - 0x32 */
    setleur_instrc = 0x30, /*!< `setleu r#, r#, r#`: R type */
    setleui_instrc = 0x31, /*!< `setleu r#, r#, imm`: S type */
    setlesr_instrc = 0x32, /*!< `setles r#, r#, r#`: R type */
    setlesi_instrc = 0x33, /*!< `setles r#, r#, imm`: S type */

    setlur_instrc = 0x34, /*!< `setlu r#, r#, r#`: R type */
    setlui_instrc = 0x35, /*!< `setlu r#, r#, imm`: S type */
    setlsr_instrc = 0x36, /*!< `setls r#, r#, r#`: R type */
    setlsi_instrc = 0x37, /*!< `setls r#, r#, imm`: S type */

    lui_instrc = 0x38,    /*!< `lu r#, imm` : L type */
    auipc_instrc = 0x39,  /*!< `auipc r#, imm` : L type */
    ecall_instrc = 0x3A,  /*!< `ecall r#, imm` : L type */
    ebreak_instrc = 0x3B, /*!< `ebreak 0` : L type */

    // [unused] outb_instrc = 0x3C, /*!< `outb, r#, r#, 0` S type */
    // [unused] outw_instrc = 0x3D, /*!< `outw, r#, r#, 0` S type */
    // [unused] inb_instrc = 0x3E, /*!< `inb, r#, r#, 0` S type */
    // [unused] inw_instrc = 0x3F /*!< `inw, r#, r#, 0` S type */

};

/**
 * @brief R type instruction layout
 *
 * Size : 8 bytes
 */
struct r_block_t
{
    uint32_t opcode : 8; /*!< operation code */
    uint32_t r1 : 5;     /*!< first operand register */
    uint32_t r2 : 5;     /*!< second operand register */
    uint32_t rd : 5;     /*!< destination register */
    uint64_t pad : 41;   /*!< padding, ignored but should be zero */
} __attribute__((packed));

/**
 * @brief S instruction layout
 *
 * Size : 8 bytes
 */
struct s_block_t
{
    uint32_t opcode : 8;     /*!< operation code */
    uint32_t r1 : 5;         /*!< first operand register */
    uint32_t rd : 5;         /*!< destination register */
    uint64_t immediate : 46; /*!< immediate value */
} __attribute__((packed));

/**
 * @brief L instruction layout
 *
 * Size : 8 bytes
 */
struct l_block_t
{
    uint32_t opcode : 8;     /*!< operation code */
    uint32_t r1 : 5;         /*!< first operand register */
    uint64_t immediate : 51; /*!< immediate value */
} __attribute__((packed));

/**
 * @brief defines a castable type between a raw `uint64_t` and
 * instruction blocks
 *
 * Size : 8 bytes
 */
union instruction_t
{
    uint64_t value;         /*!< raw instruction */
    struct r_block_t rtype; /*!< parsed block: 'r' */
    struct s_block_t stype; /*!< parsed block: 's' */
    struct l_block_t ltype; /*!< parsed block: 'l' */
};

/**
 * @brief R-type instruction constructor
 *
 * @param opcode instruction operation code
 * @param r1 first argument register
 * @param r2 second argument register
 * @param rd instruction result destination register
 *
 * @return formatted instruction
 */
union instruction_t RInstruction(const uint8_t opcode, const uint8_t r1, const uint8_t r2, const uint8_t rd);

/**
 * @brief S-type instruction constructor
 *
 * @param opcode instruction operation code
 * @param r1 argument register
 * @param rd instruction result destination register
 * @param immediate immediate argument
 * @return formatted instruction
 */
union instruction_t SInstruction(const uint8_t opcode, const uint8_t r1, const uint8_t rd, const uint64_t immediate);

/**
 * @brief L-type instruction constructor
 *
 * @param opcode instruction operation code
 * @param r1 argument register
 * @param immediate immediate argument
 * @return formatted instruction
 */
union instruction_t LInstruction(const uint8_t opcode, const uint8_t r1, const uint64_t immediate);

/**
 * @brief defines a thread that will run vm code
 *
 * Size: 320 bytes (aligned)
 */
struct thread_t
{
    uint64_t registers[32];   /*!< thread registers */
    uint64_t program_counter; /*!< thread instructon pointer */
    uint64_t memory_size;     /*!< thread memory size */
    uint8_t *memory;          /*!< thread memory pointer */
    uint8_t halt_sig;         /*!< defines when program should stop */
} __attribute__((aligned(64)));

/**
 * @brief fetches one byte from a memory address
 *
 * @param thread current thread
 * @param address address to fetch
 * @return fetched value
 */
uint8_t fetch8(struct thread_t *thread, uint64_t address) __attribute__((hot));

/**
 * @brief fetches two bytes from a memory address
 *
 *
 * @param thread current thread
 * @param address address to fetch
 * @return fetched value
 */
uint16_t fetch16(struct thread_t *thread, uint64_t address) __attribute__((hot));

/**
 * @brief fetches four bytes from a memory address
 *
 *
 * @param thread current thread
 * @param address address to fetch
 * @return fetched value
 */
uint32_t fetch32(struct thread_t *thread, uint64_t address) __attribute__((hot));

/**
 * @brief fetches eight bytes from a memory address
 *
 *
 * @param thread current thread
 * @param address address to fetch
 * @return fetched value
 */
uint64_t fetch64(struct thread_t *thread, uint64_t address) __attribute((hot));

/**
 * @brief sets one byte of memory at a specified address
 *
 * @param thread current thread
 * @param address address to set
 * @param value value to set at address
 */
void set_memory_8(struct thread_t *thread, uint64_t address, uint8_t value);

/**
 * @brief sets two bytes of memory at a specified address
 *
 * @param thread current thread
 * @param address address to set
 * @param value value to set at address
 */
void set_memory_16(struct thread_t *thread, uint64_t address, uint16_t value);

/**
 * @brief sets four bytes of memory at a specified address
 *
 * @param thread current thread
 * @param address address to set
 * @param value value to set at address
 */
void set_memory_32(struct thread_t *thread, uint64_t address, uint32_t value);

/**
 * @brief sets eight bytes of memory at a specified address
 *
 * @param thread current thread
 * @param address address to set
 * @param value value to set at address
 */
void set_memory_64(struct thread_t *thread, uint64_t address, uint64_t value);

/**
 * @brief execute one instruction while in a thread
 *
 * @param thread current thread object to emulate
 */
void exec_instruction(struct thread_t *thread) __attribute__((hot));

/**
 * @brief prints the status of a thread to `stdout`
 *
 * @param thread thread to print status from
 */
void print_status(struct thread_t *thread);

/**
 * @brief run code from a file
 *
 * @param filename file name to run
 * @param argc main's argc
 * @param argv main's argv
 * @param debugger the debugger function
 *
 * @return exit code
 */
int run(const char *filename, int argc, char **argv, void (*debugger)(struct thread_t *));

/**
 * @brief defines a debugger to the virtual machine
 *
 * @param thread
 */
void debugger_func(struct thread_t *thread);

/**
 * @brief disassembles a single instruction
 *
 * @param inst instruction value union
 */
void disassemble_instruction(union instruction_t inst);

/**
 * @brief function that shows diassembled code
 *
 * @param filename
 */
void disassemble_file(const char *filename);
/** @} */ /* end of group Virtual Instrucion Set Emulation */
#endif