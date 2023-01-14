/**
 * @file virtualmachine.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief all virtual machine components (in C!)
 * @version
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef _virtual_machine_h
#define _virtual_machine_h 1

#ifdef __cplusplus
#define extc extern "C"
#include <cstdint>
#else
#define extc
#include <stdint.h>
#endif

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
 * @brief creates a uniquely "increasing" name for a reserved instruction,
 */
#define Reserved MACRO_CONCAT(reserved_instrc, __COUNTER__)

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
    /*0x00*/ andr_instrc = 0b000000U, //!< `and r#, r#, r#` : R type
    /*0x01*/ andi_instrc = 0b000001U, //!< `and r#, r#, imm` : S type
    /*0x02*/ xorr_instrc = 0b000010U, //!< `xor r#, r#, r#` : R type
    /*0x03*/ xori_instrc = 0b000011U, //!< `xor r#, r#, imm` : S type

    /*0x04*/ orr_instrc = 0b000100U, //!< `or r#, r#, r#` : R type
    /*0x05*/ ori_instrc = 0b000101U, //!< `or r#, r#, imm` : S type
    /*0x06*/ Reserved = 0b000110U,
    /*0x07*/ Reserved = 0b000111U,

    /*0x08*/ llsr_instrc = 0b001000U, //!< `lls r#, r#, r#` : Rtype
    /*0x09*/ llsi_instrc = 0b001001U, //!< `lls r#, r#, imm` : S type
    /*0x0A*/ lrsr_instrc = 0b001010U, //!< `lrs r#, r#, r#` : Rtype
    /*0x0B*/ lrsi_instrc = 0b001011U, //!< `lrs r#, r#, imm` : S type

    /*0x0C*/ alsr_instrc = 0b001100U, //!< `als r#, r#, r#` : R type
    /*0x0D*/ alsi_instrc = 0b001101U, //!< `als r#, r#, imm` : S type
    /*0x0E*/ arsr_instrc = 0b001110U, //!< `ars r#, r#, r#` : R type
    /*0x0F*/ arsi_instrc = 0b001111U, //!< `ars r#, r#, imm` : S type

    /* group 1 */
    /*0x10*/ addr_instrc = 0b010000U, //!< `add r#, r#, r#` : R type
    /*0x11*/ addi_instrc = 0b010001U, //!< `add r#, r#, imm` : S type
    /*0x12*/ subr_instrc = 0b010010U, //!< `sub r#, r#, r#` : R type
    /*0x13*/ subi_instrc = 0b010011U, //!< `sub r#, r#, imm` : S type

    /*0x14*/ umulr_instrc = 0b010100U, //!< `umul r#, r#, r#` : R type
    /*0x15*/ umuli_instrc = 0b010101U, //!< `umul r#, r#, imm` : S type
    /*0x16*/ smulr_instrc = 0b010110U, //!< `smul r#, r#, r#` : R type
    /*0x17*/ smuli_instrc = 0b010111U, //!< `smul r#, r#, imm` : S type

    /*0x18*/ udivr_instrc = 0b011000U, //!< `udiv r#, r#, r#` : R type
    /*0x19*/ udivi_instrc = 0b011001U, //!< `udiv r#, r#, imm` : S type
    /*0x1A*/ sdivr_instrc = 0b011010U, //!< `sdiv r#, r#, r#` : R type
    /*0x1B*/ sdivi_instrc = 0b011011U, //!< `sdiv r#, r#, imm` : S type

    /*0x1C*/ setur_instrc = 0b011100U, //!< `setu r#, r#, r#` : R type
    /*0x1D*/ setui_instrc = 0b011101U, //!< `setu r#, r#, imm` : S type
    /*0x1E*/ setsr_instrc = 0b011110U, //!< `setr r#, r#, r#` : R type
    /*0x1F*/ setsi_instrc = 0b011111U, //!< `setr r#, r#, imm` : S type

    /* group 2 */
    /*0x20*/ ld_byte_instrc = 0b100000U, //!< `ldb r#, r#, imm` : S type
    /*0x21*/ ld_half_instrc = 0b100001U, //!< `ldh r#, r#, imm` : S type
    /*0x22*/ ld_word_instrc = 0b100010U, //!< `ldw r#, r#, imm` : S type
    /*0x23*/ ld_dwrd_instrc = 0b100011U, //!< `ldd r#, r#, imm` : S type

    /*0x24*/ st_byte_instrc = 0b100100U, //!< `stb r#, r#, imm` : S type
    /*0x25*/ st_half_instrc = 0b100101U, //!< `sth r#, r#, imm` : S type
    /*0x26*/ st_word_instrc = 0b100110U, //!< `stw r#, r#, imm` : S type
    /*0x27*/ st_dwrd_instrc = 0b100111U, //!< `std r#, r#, imm` : S type

    /*0x28*/ jal_instrc = 0b101000U,  //!< `jal r#, imm` : L type
    /*0x29*/ jalr_instrc = 0b101001U, //!< `jalr r#, r#, imm` : S type
    /*0x2A*/ je_instrc = 0b101010U,   //!< `je r#, r#, imm` : S type
    /*0x2B*/ jne_instrc = 0b101011U,  //!< `jne r#, r#, imm` : S type

    /*0x2C*/ jlu_instrc = 0b101100U,  //!< `jlu r#, r#, imm` : S type
    /*0x2D*/ jls_instrc = 0b101101U,  //!< `jls r#, r#, imm` : S type
    /*0x2E*/ jleu_instrc = 0b101110U, //!< `jleu r#, r#, imm` : S type
    /*0x2F*/ jles_instrc = 0b101111U, //!< `jles r#, r#, imm` : S type

    /* other instructions, 0x30 - 0x32 */
    /*0x30*/ lui_instrc = 0b110000U,   //!< `lu r#, imm` : L type
    /*0x31*/ auipc_instrc = 0b110001U, //!< `auipc r#, imm` : L type
    /*0x32*/ ecall_instrc = 0b110010U, //!< `ecall r#, imm` : L type
    /*0x33*/ ebreak_instrc = 0b110011U, //!< `ebreak` : L type

    /*0x34*/ Reserved = 0b110100U,
    /*0x35*/ Reserved = 0b110101U,
    /*0x36*/ Reserved = 0b110110U,
    /*0x37*/ Reserved = 0b110111U,

    /*0x38*/ Reserved = 0b111100U,
    /*0x39*/ Reserved = 0b111101U,
    /*0x3A*/ Reserved = 0b111110U,
    /*0x3B*/ Reserved = 0b111111U,

    /*0x3C*/ Reserved = 0b111100U,
    /*0x3D*/ Reserved = 0b111101U,
    /*0x3E*/ Reserved = 0b111110U,
    /*0x3F*/ Reserved = 0b111111U,

};

/**
 * @brief R type instruction layout
 *
 * Size : 8 bytes
 */
struct r_block_t
{
    uint32_t opcode : 8; //!< operation code
    uint32_t r1 : 5;     //!< first operand register
    uint32_t r2 : 5;     //!< second operand register
    uint32_t rd : 5;     //!< destination register
    uint64_t pad : 41;   //!< padding, ignored but should be zero
} __attribute__((packed));

/**
 * @brief S instruction layout
 *
 * Size : 8 bytes
 */
struct s_block_t
{
    uint32_t opcode : 8;     //!< operation code
    uint32_t r1 : 5;         //!< first operand register
    uint32_t rd : 5;         //!< destination register
    uint64_t immediate : 46; //!< immediate value
} __attribute__((packed));

/**
 * @brief L instruction layout
 *
 * Size : 8 bytes
 */
struct l_block_t
{
    uint32_t opcode : 8;     //!< operation code
    uint32_t r1 : 5;         //!< first operand register
    uint64_t immediate : 51; //!< immediate value
} __attribute__((packed));

/**
 * @brief defines a castable type between a raw `uint64_t` and 
 * instruction blocks
 * 
 * Size : 8 bytes
 */
union instruction_t
{
    uint64_t value; //!< raw instruction
    struct r_block_t rtype; //!< parsed block: 'r' 
    struct s_block_t stype; //!< parsed block: 's'
    struct l_block_t ltype; //!< parsed block: 'l'
};

/**
 * @brief defines a thread that will run vm code
 * 
 * Size: 320 bytes (aligned)
 */
struct thread_t
{
    uint64_t registers[32];
    uint64_t program_counter;
    uint64_t memory_size;
    uint8_t *memory;
} __attribute__((aligned(64)));

/**
 * @brief fetches one byte from a memory address
 *
 * @param thread current thread 
 * @param address address to fetch
 * @return fetched value
 */
uint8_t fetch8(struct thread_t *thread, uint64_t address);

/**
 * @brief fetches two bytes from a memory address
 * 
 *
 * @param thread current thread 
 * @param address address to fetch
 * @return fetched value
 */
uint16_t fetch16( struct thread_t *thread,  uint64_t address);

/**
 * @brief fetches four bytes from a memory address
 * 
 *
 * @param thread current thread 
 * @param address address to fetch
 * @return fetched value
 */
uint32_t fetch32(struct thread_t *thread, uint64_t address);

/**
 * @brief fetches eight bytes from a memory address
 * 
 *
 * @param thread current thread 
 * @param address address to fetch
 * @return fetched value
 */
uint64_t fetch64(struct thread_t *thread, uint64_t address);

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
void exec_instruction(struct thread_t *thread);

/**
 * @brief prints the status of a thread to `stdout`
 * 
 * @param thread thread to print status from
 */
void print_status(struct thread_t * thread);

/**
 * @brief run code from a file
 * 
 * @param filename file name to run
 */
void run(const char *filename);

#endif