/**
 * @file supernova.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief custom ISA designed for Zenith (C and C++ compatible)
 * @version 0.0.1
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once
#ifndef ZENITH_SUPERNOVA_H
/**@cond */
#define ZENITH_SUPERNOVA_H 1

#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#define xstr(s) str(s)
#define str(s) #s
/**@endcond */

#include <stdint.h>

#if defined(_WIN32)
#if defined(SUPERNOVA_EXPORT)
#define supernova_api __declspec(dllexport)
#else
#define supernova_api __declspec(dllimport)
#endif
#else /* non windows */
/** define export code for the api */
#define supernova_api
#endif

/**
 * @brief creates a uniquely "increasing" name for a reserved instruction,
 */
#define Reserved MACRO_CONCAT(reserved_instrc, __COUNTER__)

/** macro for generating functions headers */
#define sninstr_name(instruction) supernova_##instruction##_dispatch_function

#define sninstr_func(instruction) \
 supernova_api void sninstr_name(instruction)(struct thread_t *thread, const union instruction_t instr)

/** define a call to a dispatch insrtuction */
#define sninstr_func_call(op, thread, instruction) \
    sninstr_name(op)(thread, instruction)

/**
 * \brief supernova opcodes (64 bit version)
 *
 * instruction types:
 *
 * R type (registers only) => register-register-register
 *
 * bitwise representation:
 * @code
 * [63-24][23-19][18-14][13-8][7-0]
 *    |      |      |     |      |
 *  [pad]  [r2]   [r1]   [rd] [opcode]
 * @endcode
 *
 * S type ("small" immediate) => register-register-immediate
 *
 * bitwise representation:
 * @code
 * [63-19][18-14][13-8][7-0]
 *    |      |     |     |
 *  [imm]  [r1]  [rd] [opcode]
 * @endcode
 *
 * L type ("long" immediate) => register-immediate
 *
 * bitwise representation:
 * @code
 * [63-14][13-8][7-0]
 *    |     |     |
 *  [imm]  [rd] [opcode]
 * @endcode
 *
 * base instructions are split into 8 groups (named after values at bits [6,4]):
 *
 * groups 0-3 make part of a "base" group, on which all thread models need to implement
 * most of the operations, the exception being division and stack instructions on group 2.
 *
 * groups 4-7 make part of an "extension" group, the cpu will have flags that can be checked
 * to see if those operations are hardware-implemented
 *
 * - group 0: bitwise instructions, span opcodes `0x00` to `0x0F`,
 * - group 1: math / control flow instructions, span opcodes `0x10` to `0x1F`
 * - group 2: memory / control flow instructions, span opcodes `0x20` to `0x2F`
 * - group 3: conditional set / interrupts / IO, span opcodes `0x30` to `0x3F`
 *
 *
 * on groups 0 and 1, the LSB defines if the instruction is an R type (when 0)
 * or an S type (when 1) group 2, has only S types and the `jal` instruction
 * (L type) group 3 is always L type
 */
enum instruction_prefixes
{
    /**
     * @defgroup InP instruction prefixes
     * 
     * @brief those groups define instructions that have similar behaviors 
     * between themselves
     * 
     * @{
    */

    /**
     * @defgroup InPG0 instruction prefixes, group 0
     * 
     * @brief Group 0 defines bitwise instructions to operate on registers or 
     * immediates
     * 
     * - \ref supernova_andr_instrc_dispatch_function "and register-register": (R type)
     * - \ref supernova_andi_instrc_dispatch_function "and register-immediate": (S type)
     * - \ref supernova_xorr_instrc_dispatch_function "xor register-register": (R type)
     * - \ref supernova_xori_instrc_dispatch_function "xor register-immediate": (S type)
     * - \ref supernova_orr_instrc_dispatch_function "or register-register":  (R type) 
     * - \ref supernova_ori_instrc_dispatch_function "or register-immediate":  (S type)
     * - \ref supernova_not_instrc_dispatch_function "not register":  (R type) 
     * - \ref supernova_cnt_instrc_dispatch_function "count register":  (S type, works as an R instruction)
     * - \ref supernova_llsr_instrc_dispatch_function "logic left shift register-register": (R type)
     * - \ref supernova_llsi_instrc_dispatch_function "logic left shift register-immediate": (S type)
     * - \ref supernova_lrsr_instrc_dispatch_function "logic right shift register-register": (R type)
     * - \ref supernova_lrsi_instrc_dispatch_function "logic right shift register-immediate": (S type)
     * - \ref supernova_alsr_instrc_dispatch_function "arithmetic left shift register-register": (R type)
     * - \ref supernova_alsi_instrc_dispatch_function "arithmetic left shift register-immediate": (S type)
     * - \ref supernova_arsr_instrc_dispatch_function "arithmetic right shift register-register": (R type)
     * - \ref supernova_arsi_instrc_dispatch_function "arithmetic right shift register-immediate": (S type)
    */

    /** `and r#, r#, r#`  : R type */ 
    andr_instrc = 0x00, 

    /** `and r#, r#, imm` : S type */ 
    andi_instrc = 0x01, 

    /** `xor r#, r#, r#`  : R type */ 
    xorr_instrc = 0x02, 

    /** `xor r#, r#, imm` : S type */ 
    xori_instrc = 0x03, 

    /**< `or r#, r#, r#`   : R type */
    orr_instrc = 0x04, 
    
    /**< `or r#, r#, imm`  : S type */
    ori_instrc = 0x05, 
    
    /**< `not r#, r#, r#`  : R type */
    not_instrc = 0x06, 
    
    /**< `cnt r#, r#, imm` : S type */
    cnt_instrc = 0x07, 
    
    /**< `lls r#, r#, r#`  : R type */
    llsr_instrc = 0x08, 
    
    /**< `lls r#, r#, imm` : S type */
    llsi_instrc = 0x09, 
    
    /**< `lrs r#, r#, r#`  : R type */
    lrsr_instrc = 0x0A, 
    
    /**< `lrs r#, r#, imm` : S type */
    lrsi_instrc = 0x0B, 

    /**< `als r#, r#, r#`  : R type */
    alsr_instrc = 0x0C, 
    
    /**< `als r#, r#, imm` : S type */
    alsi_instrc = 0x0D, 
    
    /**< `ars r#, r#, r#`  : R type */
    arsr_instrc = 0x0E, 
    
    /**< `ars r#, r#, imm` : S type */
    arsi_instrc = 0x0F, 
    
    /** @} */ /* InPG0 */

    /**
     * @defgroup InPG1 Instruction prefixes, group 1
     * 
     * @brief define prefixes for arithimetic instructions1
    */
    /* group 1 */
    addr_instrc = 0x10, /**< `add r#, r#, r#`  : R type */
    addi_instrc = 0x11, /**< `add r#, r#, imm` : S type */
    subr_instrc = 0x12, /**< `sub r#, r#, r#`  : R type */
    subi_instrc = 0x13, /**< `sub r#, r#, imm` : S type */

    umulr_instrc = 0x14, /**< `umul r#, r#, r#`  : R type */
    umuli_instrc = 0x15, /**< `umul r#, r#, imm` : S type */
    smulr_instrc = 0x16, /**< `smul r#, r#, r#`  : R type */
    smuli_instrc = 0x17, /**< `smul r#, r#, imm` : S type */

    udivr_instrc = 0x18, /**< `udiv r#, r#, r#`  : R type */
    udivi_instrc = 0x19, /**< `udiv r#, r#, imm` : S type */
    sdivr_instrc = 0x1A, /**< `sdiv r#, r#, r#`  : R type */
    sdivi_instrc = 0x1B, /**< `sdiv r#, r#, imm` : S type */

    call_instrc = 0x1C, /**< `call r#, r#, r#`  : R type */
    push_instrc = 0x1D, /**< `push r#, r#, imm` : S type */
    retn_instrc = 0x1E, /**< `retn r#, r#, r#`  : R type */
    pull_instrc = 0x1F, /**< `pull r#, r#, imm` : S type */

    /* group 2 */
    ld_byte_instrc = 0x20, /**< `ldb r#, r#, imm` : S type */
    ld_half_instrc = 0x21, /**< `ldh r#, r#, imm` : S type */
    ld_word_instrc = 0x22, /**< `ldw r#, r#, imm` : S type */
    ld_dwrd_instrc = 0x23, /**< `ldd r#, r#, imm` : S type */

    st_byte_instrc = 0x24, /**< `stb r#, r#, imm` : S type */
    st_half_instrc = 0x25, /**< `sth r#, r#, imm` : S type */
    st_word_instrc = 0x26, /**< `stw r#, r#, imm` : S type */
    st_dwrd_instrc = 0x27, /**< `std r#, r#, imm` : S type */

    jal_instrc = 0x28,  /**< `jal r#, imm`      : L type */
    jalr_instrc = 0x29, /**< `jalr r#, r#, imm` : S type */
    je_instrc = 0x2A,   /**< `je r#, r#, imm`   : S type */
    jne_instrc = 0x2B,  /**< `jne r#, r#, imm`  : S type */

    jgu_instrc = 0x2C,  /**< `jgu r#, r#, imm`  : S type */
    jgs_instrc = 0x2D,  /**< `jgs r#, r#, imm`  : S type */
    jleu_instrc = 0x2E, /**< `jleu r#, r#, imm` : S type */
    jles_instrc = 0x2F, /**< `jles r#, r#, imm` : S type */

    /* group 3 */
    setgur_instrc = 0x30, /**< `setgu r#, r#, r#`  : R type */
    setgui_instrc = 0x31, /**< `setgu r#, r#, imm` : S type */
    setgsr_instrc = 0x32, /**< `setgs r#, r#, r#`  : R type */
    setgsi_instrc = 0x33, /**< `setgs r#, r#, imm` : S type */

    setleur_instrc = 0x34, /**< `setleu r#, r#, r#`  : R type */
    setleui_instrc = 0x35, /**< `setleu r#, r#, imm` : S type */
    setlesr_instrc = 0x36, /**< `setles r#, r#, r#`  : R type */
    setlesi_instrc = 0x37, /**< `setles r#, r#, imm` : S type */

    lui_instrc = 0x38,    /**< `lui r#, imm`   : L type */
    auipc_instrc = 0x39,  /**< `auipc r#, imm` : L type */
    pcall_instrc = 0x3A,  /**< `pcall r#, imm` : L type */
    pbreak_instrc = 0x3B, /**< `pbreak 0`      : L type */

    bout_instrc = 0x3C,  /**< `outb r#, r#, 0` S type */
    out_instrc = 0x3D,   /**< `outw r#, r#, 0` S type */
    bin_instrc = 0x3E, /**< `inb r#, r#, 0`  S type */
    in_instrc = 0x3F,  /**< `inw r#, r#, 0`  S type */

    /** group 4, floating point operation instructions */

    flt_ldu_instrc = 0x40, /**< `fldu fr#, r#, 0` : R type */
    flt_lds_instrc = 0x41, /**< `flds fr#, r#, 0` : R type */
    flt_stu_instrc = 0x42, /**< `fstu r#, fr#, 0` : R type */
    flt_sts_instrc = 0x43, /**< `fsts r#, fr#, 0` : R type */

    flt_add_instrc = 0x44, /**< `fadd fr#, fr#, fr#` : R type */
    flt_sub_instrc = 0x45, /**< `fsub fr#, fr#, fr#` : R type */
    flt_mul_instrc = 0x46, /**< `fmul fr#, fr#, fr#` : R type */
    flt_div_instrc = 0x47, /**< `fdiv fr#, fr#, fr#` : R type */

    flt_ceq_instrc = 0x48, /**< `fcmpeq r#, fr#, fr#` : R type */
    flt_cne_instrc = 0x49, /**< `fcmpne r#, fr#, fr#` : R type */
    flt_cgt_instrc = 0x4A, /**< `fcmpgt r#, fr#, fr#` : R type */
    flt_cle_instrc = 0x4B, /**< `fcmple r#, fr#, fr#` : R type */

    flt_rou_instrc = 0x4C, /**< `fround fr#, fr#, imm` : S type */
    flt_flr_instrc = 0x4D, /**< `ffloor fr#, fr#, imm` : S type */
    flt_cei_instrc = 0x4E, /**< `fceil fr#, fr#, imm` : S type */
    flt_trn_instrc = 0x4F, /**< `ftrnc fr#, fr#, imm` : S type */

    /** TODO: group 5, conditional set / move instructions */
    /** TODO: group 6, memory fences */
    /** @} */ /* InP */
};

enum Processor_call_offsets {
	DivisionByZero = 0,
	GeneralFault,
	DoubleFault,
	TripleFault,
	InvalidInstruction,
	PageFault,

};

/**
 * @brief R type instruction layout
 *
 * Size : 8 bytes
 */
struct r_block_t
{
    uint64_t opcode : 8; /**< operation code */
    uint64_t r1 : 5;     /**< first operand register */
    uint64_t rd : 5;     /**< destination register */
    uint64_t r2 : 5;     /**< second operand register */
    uint64_t pad : 41;   /**< padding, ignored but should be zero */
} __attribute__((packed));

/**
 * @brief S instruction layout
 *
 * Size : 8 bytes
 */
struct s_block_t
{
    uint64_t opcode : 8;     /**< operation code */
    uint64_t r1 : 5;         /**< first operand register */
    uint64_t rd : 5;         /**< destination register */
    uint64_t immediate : 46; /**< immediate value */
} __attribute__((packed));
	
/**
 * @brief L instruction layout
 *
 * Size : 8 bytes
 */
struct l_block_t
{
    uint64_t opcode : 8;     /**< operation code */
    uint64_t r1 : 5;         /**< first operand register */
    uint64_t immediate : 51; /**< immediate value */
} __attribute__((packed));

/**
 * @brief defines a castable type between a raw `uint64_t` and
 * instruction blocks
 *
 * Size : 8 bytes
 */
union instruction_t
{
    uint64_t value;         /**< raw instruction */
    struct r_block_t rtype; /**< parsed block: 'r' */
    struct s_block_t stype; /**< parsed block: 's' */
    struct l_block_t ltype; /**< parsed block: 'l' */
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
 * 
 * @public @memberof r_block_t
 */
supernova_api union instruction_t RInstruction(const uint8_t opcode, const uint8_t r1, const uint8_t r2, const uint8_t rd);

/**
 * @brief S-type instruction constructor
 *
 * @param opcode instruction operation code
 * @param r1 argument register
 * @param rd instruction result destination register
 * @param immediate immediate argument
 * @return formatted instruction
 * 
 * @public @memberof s_block_t
 */
supernova_api union instruction_t SInstruction(const uint8_t opcode, const uint8_t r1, const uint8_t rd, const uint64_t immediate);
/**
 * @brief L-type instruction constructor
 *
 * @param opcode instruction operation code
 * @param r1 argument register
 * @param immediate immediate argument
 * @return formatted instruction
 * 
 * @public @memberof l_block_t
 */
supernova_api union instruction_t LInstruction(const uint8_t opcode, const uint8_t r1, const uint64_t immediate);

struct thread_t;

/**
 * @brief defines a function type related to interrupts
 *
 */
typedef void (*interrupt_function_t)(int, struct thread_t *);

/**
 * @brief defines a function for generating instructions
 */
typedef void (*instr_dispatch_t)(register struct thread_t *, union instruction_t instruction);

enum config_flags_1
{
    /** support for memory paging */
    confflags_paging = 0,

    /** support for stack instructions */
    confflags_stack,

    /** support for integer division instructions */
    confflags_intdiv,

    /** support for software interrupts */
    confflags_interrupts,

    /** support for hardware floating point */
    confflags_floats,

    /** support for memory fences */
    confflags_fences,     

    /** support for conditional get/set */
    confflags_condset,    

    /** support for conditional move */
    confflags_condmove,   

    /** multiple execution instructions, 64 bit */
    confflags_multi64,    

    /** multiple execution instructions, 128 bit */
    confflags_multi128,   

    /** multiple execution instructions, 256 bit */
    confflags_multi256,   

    /** multiple execution instructions, 512 bit */
    confflags_multi512,   

    /** @b programmable hardware interrupts */
    confflags_ioint
};


#define flag_bit(flag) (1 << (flag))

struct thread_model_t
{
    /** processor functionality flags */
    uint64_t flags;

    /** how many interrupts the processor is able to handle*/
    uint64_t interrupt_count;

    /** how deep can pages go */
    uint64_t page_level;

    /** how big can pages be*/
    uint64_t page_size;

    /** string with model name */
    uint64_t model_name[4];

    /** define the last known i/o address */
    uint64_t io_address_space;

    /** define the last instruction opcode */
    uint64_t last_instruction_index;
};

/**
 * @brief defines a thread that will run vm code
 *
 * threads have 32 registers, but 31 are actually usable (r0 is reset to zero
 * every cycle). besides that, the architecture design only worries about stack
 * registers to save state on interrupts, 
 * but the compiler does set some conventions, being:
 *
 * r1: 1st return register
 * r2: stack pointer
 * r3: base pointer
 *
 * r28: second processor call argument (function switch)
 * r29: first processor call argument (interrupt space)
 *
 * r31 going upwards: function arguments
 *
 * Size: 296 bytes 
 */
struct thread_t
{
    uint64_t registers[32];       /**< thread registers */
    uint64_t program_counter;     /**< thread instructon pointer */
    uint64_t int_vector;          /**< interrupt vector pointer*/
    uint64_t memory_size;         /**< thread memory size */
    uint8_t *memory;              /**< thread memory pointer */
    struct thread_model_t *model; /**< thread model pointer */
    uint8_t halt_sig;             /**< defines when program should stop */
};

#undef CONCAT_IMPL
#undef MACRO_CONCAT
#undef Reserved

/**
 * @brief defines a function type related to interrupts
 *
 */
typedef void (*interrupt_function_t)(int, struct thread_t *);

#endif
