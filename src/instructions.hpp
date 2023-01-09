/**
 * \file instructions.hpp
 * \author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * \brief instructions for the virtual machine
 * \version
 * \date 2022-12-02
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef VM_CODE_H
#define VM_CODE_H 1

#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)

#define Reserved MACRO_CONCAT(reserved_instrc, __COUNTER__) //!< macro for reserved instructions (basically padding)

#include <cstdint>

/**
 * \defgroup VirMac Virtual Instruction Set Emulation
 * \brief all the instruction prefixes used on the emulated vm cpu, all designed by me
 *
 * \note reserved instructions are generally alignment or they just wouldnt make sense,
 * with examples examples being "mov" with an immediate destination
 *
 * \note The higher nibble describes an instruction group, while the lower nibble describes its arguments
 * @{
 */


/**
 * \brief get the instruction group
 *
 */
#define getInstructionGroup(instruction) (((instruction)&0xf0) >> 4)
#define getAction(instruction) ((instruction)&0x0f)

namespace VirtMac
{
	
	/**
	 * \brief prefixes for every instruction
	 * 
	 * \note as a design choice, both `(uint64_t)0` and `(uint64_t)-1` are instructions that will be no-op 
	 */
	enum instruction_prefixes : unsigned char
	{
		/* bit-wise operations */
		/* 0x00 - 0x0F*/
		andr_instrc = 0b00000000U, //!< and r#, r#
		andi_instrc = 0b00000001U, //!< and r#, imm
		xorr_instrc = 0b00000010U, //!< xor r#, r#
		xori_instrc = 0b00000011U, //!< xor r#, imm

		orr_instrc = 0b00000100U, //!< or r#, r#
		ori_instrc = 0b00000101U, //!< or r#, imm
		not_instrc = 0b00000110U, //!< not r#
		neg_instrc = 0b00000111U, //!< neg r#

		llsr_instrc = 0b00001000U, //!< lls r#, r#
		llsi_instrc = 0b00001001U, //!< lls r#, imm
		lrsr_instrc = 0b00001010U, //!< lrs r#, r#
		lrsi_instrc = 0b00001011U, //!< lrs r#, imm

		alsr_instrc = 0b00001100U, //!< als r#, r#
		alsi_instrc = 0b00001101U, //!< als r#, imm
		arsr_instrc = 0b00001110U, //!< ars r#, r#
		arsi_instrc = 0b00001111U, //!< ars r#, imm

		/* math pt 1 */
		/* 0x10 - 0x1F */
		add_rwr_instrc = 0b00010000U, //!< add r#, r#
		add_rwi_instrc = 0b00010001U, //!< add r#, imm
		add_rwm_instrc = 0b00010010U, //!< add r#, [r#]
		add_rwp_instrc = 0b00010011U, //!< add r#, [imm]

		adc_rwr_instrc = 0b00010100U, //!< adc r#, r#  
		adc_rwi_instrc = 0b00010101U, //!< adc r#, imm
		adc_rwm_instrc = 0b00010110U, //!< adc r#, [r#]
		adc_rwp_instrc = 0b00010111U, //!< adc r#, [imm]

		sub_rwr_instrc = 0b00011000U, //!< sub r#, r#
		sub_rwi_instrc = 0b00011001U, //!< sub r#, imm
		sub_rwm_instrc = 0b00011010U, //!< sub r#, [r#]
		sub_rwp_instrc = 0b00011011U, //!< sub r#, [imm]

		sbb_rwr_instrc = 0b00011100U, //!< sbb r#, r#  
		sbb_rwi_instrc = 0b00011101U, //!< sbb r#, imm
		sbb_rwm_instrc = 0b00011110U, //!< sbb r#, [r#]
		sbb_rwp_instrc = 0b00011111U, //!< sbb r#, [imm]

		/* math pt 2, 0x20 - 0x27 */
		mul_rwr_instrc = 0b00100000U, //!< mul r#, r#
		mul_rwi_instrc = 0b00100001U, //!< mul r#, imm
		imul_rwr_instrc = 0b00100010U,//!< imul r#, r# 
		imul_rwi_instrc = 0b00100011U,//!< imul r#, imm

		div_rwr_instrc = 0b00100100U, //!< div r#, r#
		div_rwi_instrc = 0b00100101U, //!< div r#, imm
		idiv_rwr_instrc = 0b00100110U, //!< idiv r#, r# 
		idiv_rwi_instrc = 0b00100111U, //!< idiv r#, imm

		Reserved = 0b00101000U,
		Reserved = 0b00101001U,
		Reserved = 0b00101010U,
		Reserved = 0b00101011U,

		Reserved = 0b00101100U,
		Reserved = 0b00101101U,
		Reserved = 0b00101110U,
		Reserved = 0b00101111U,

		/* fpu instructions, 0x3x */
		fpu_add_instrc = 0b00110000U,
		fpu_sub_instrc = 0b00110001U,
		fpu_mul_instrc = 0b00110010U,
		fpu_div_instrc = 0b00110011U,

		fpu_trnc_instrc = 0b00110100U,
		fpu_rou_instrc = 0b00110101U,
		fpu_floor_instrc = 0b00110110U,
		fpu_ceil_instrc = 0b00110111U,

		fpu_pow_instrc = 0b00111000U,
		fpu_root_instrc = 0b00111001U,
		fpu_log_instrc = 0b00111010U,
		fpu_tan_instrc = 0b00111011U,

		fpu_sin_instrc = 0b00111100U,
		fpu_cos_instrc = 0b00111101U,
		fpu_set_instrc = 0b00111110U,
		fpu_get_instrc = 0b00111111U,

		/* move instructions, 0x4x */
		mov_rtr_instrc = 0b01000000U,  //!< mov r#, r#
		mov_rtim_instrc = 0b01000001U, //!< mov r#, [imm]
		mov_rtm_instrc = 0b01000010U,  //!< mov r#, [r#]
		Reserved = 0b01000011U, 

		mov_itr_instrc = 0b01000100U,  //!< mov imm, r# 
		mov_imtr_instrc = 0b01000101U, //!< mov [imm], r#
		mov_mtr_instrc = 0b01000110U,  //!< mov [r#], r#
		Reserved = 0b01000111U,

		cmovz_rtr_instrc = 0b01001000U, //!< cmovz r#, r#  
		cmovz_rtm_instrc = 0b01001001U, //!< cmovz r#, [imm]
		cmovz_itr_instrc = 0b01001010U, //!< cmovz imm, r#
		cmovz_mtr_instrc = 0b01001011U, //!< cmovz [imm], r#

		sete_instrc = 0b01001100U, //!< sete r#
		setne_instrc = 0b01001101U, //!< setne r#
		setz_instrc = 0b01001110U, //!< setz r#
		setnz_instrc = 0b01001111U, //!< setnz r#

		/* flag changing instructions, 0x5x */

		cli_instrc = 0b01010000U, //!< cli
		clp_instrc = 0b01010001U, //!< clp
		clc_instrc = 0b01010010U, //!< clc
		cls_instrc = 0b01010011U, //!< cls

		clz_instrc = 0b01010100U, //!< clz
		clo_instrc = 0b01010101U, //!< clo
		clb_instrc = 0b01010110U, //!< clb
		cld_instrc = 0b01010111U, //!< cld

		sti_instrc = 0b01011000U, //!< sti
		stp_instrc = 0b01011001U, //!< stp
		stc_instrc = 0b01011010U, //!< stc
		sts_instrc = 0b01011011U, //!< sts

		stz_instrc = 0b01011100U, //!< stz
		sto_instrc = 0b01011101U, //!< sto
		stb_instrc = 0b01011110U, //!< stb
		std_instrc = 0b01011111U, //!< std

		/* control register manipulation instructions, 0x6x */
		load_cr0_instrc = 0b01100000U, /* instruction pointer register */
		load_cr1_instrc = 0b01100001U, /* descriptor table register	*/
		load_cr2_instrc = 0b01100010U, /* interrupt table pointer register */
		load_cr3_instrc = 0b01100011U, /* top level page pointer register	*/

		load_cr4_instrc = 0b01100100U, /* stack top register */
		load_cr5_instrc = 0b01100101U, /* stack base register */
		load_cr6_instrc = 0b01100110U, /* interrupt return */
		load_cr7_instrc = 0b01100111U, /* interrupt fault */

		stor_cr0_instrc = 0b01101000U,
		stor_cr1_instrc = 0b01101001U,
		stor_cr2_instrc = 0b01101010U,
		stor_cr3_instrc = 0b01101011U,

		stor_cr4_instrc = 0b01101100U,
		stor_cr5_instrc = 0b01101101U,
		stor_cr6_instrc = 0b01101110U,
		stor_cr7_instrc = 0b01101111U,

		/* test instructions 0x7x*/
		test_rwr_instrc = 0b01110000U,
		test_rwi_instrc = 0b01110001U,
		test_rwm_instrc = 0b01110010U,
		test_iwr_instrc = 0b01110011U,

		Reserved = 0b01110100U,
		Reserved = 0b01110101U,
		Reserved = 0b01110110U,
		Reserved = 0b01110111U,

		cmp_rwr_instrc = 0b01111000U,
		cmp_rwi_instrc = 0b01111001U,
		cmp_rwm_instrc = 0b01111010U,
		cmp_iwr_instrc = 0b01111011U,

		Reserved = 0b01111100U,
		Reserved = 0b01111101U,
		Reserved = 0b01111110U,
		Reserved = 0b01111111U,

		/* jumps  0x8x*/
		jmp_instrc = 0b10000000U,
		jmpr_instrc = 0b10000001U,
		jl_instrc = 0b10000010U,
		jge_instrc = 0b10000011U,

		jg_instrc = 0b10000100U,
		jle_instrc = 0b10000101U,
		je_instrc = 0b10000110U,
		jne_instrc = 0b10000111U,

		jc_instrc = 0b10001000U,
		jnc_instrc = 0b10001001U,
		jz_instrc = 0b10001010U,
		jnz_instrc = 0b10001011U,

		js_instrc = 0b10001100U,
		jns_instrc = 0b10001101U,
		jo_instrc = 0b10001110U,
		jno_instrc = 0b10001111U,

		/* stack functions  0x9x*/
		pushi_instrc = 0b10010000U,
		pushr_instrc = 0b10010001U,
		pushflg_instrc = 0b10010010U,
		pushall_instrc = 0b10010011U,

		popn_instrc = 0b10010100U,
		popr_instrc = 0b10010101U,
		popflg_instrc = 0b10010110U,
		popall_instrc = 0b10010111U,

		calli_instrc = 0b10011000U,
		callr_instrc = 0b10011001U,
		Reserved = 0b10011010U,
		ret_instrc = 0b10011011U,

		int_instrc = 0b10011100U,
		iret_instrc = 0b10011101U,
		Reserved = 0b10011110U,
		Reserved = 0b10011111U,

		// almost 100 instructions still able to fit in (not counting reserved ones)
	};

	enum instruction_mask_attributes
	{
		attr_reg_low_00 = 0b00000000U,
		attr_reg_low_01 = 0b00000001U,
		attr_reg_low_02 = 0b00000010U,
		attr_reg_low_04 = 0b00000100U,
		attr_reg_low_08 = 0b00001000U,

		attr_reg_hig_00 = 0b00000000U,
		attr_reg_hig_01 = 0b00010000U,
		attr_reg_hig_02 = 0b00100000U,
		attr_reg_hig_04 = 0b01000000U,
		attr_reg_hig_08 = 0b10000000U,

		attr_reg_size   = 0b00001111U,
		attr_reg_offset = 0b00001000U
	};

	/* flags register structure   */
	/* | 0x0F - 0x0a | 0x09 | 0x08 |    0x07    |    0x06    |   0x05   | 0x04 | 0x03 |  0x02  |  0x01  |    0x00    |*/
	/* |  cpu props  |  io  | halt | directionb | breakpoint | overflow | zero | sign | carryf | paging | interrupts |*/
	enum vm_flags
	{
		interrupt_flag = 0b0000000000000001U,
		paging_flag = 0b0000000000000010U,
		carry_flag = 0b0000000000000100U,
		sign_flag = 0b0000000000001000U,
		zero_flag = 0b0000000000010000U,
		overflow_flag = 0b0000000000100000U,
		breakpoint_flag = 0b0000000001000000U,
		direction_flag = 0b0000000010000000U,
		halt_flag = 0b0000000100000000U,
		io_flag = 0b0000001000000000U,
	};

	enum cpu_groups
	{
		bitwise_ops = 0b00000000U,
		add_sub_ops = 0b00010000U,
		mul_div_ops = 0b00100000U,
		fpu_ops = 0b00110000U,
		move_ops = 0b01000000U,
		flag_ops = 0b01010000U,
		control_ops = 0b01100000U,
		test_ops = 0b01110000U,
		jump_ops = 0b10000000U,
		stack_ops = 0b10010000U,

	};

	/**
	 * @brief defines a struct that holds important information about a instruction
	 * 
	 */
	struct Instruction
	{
		uint8_t attr; //!< 1st byte: instruction attibutes
		uint8_t prefix; //!< 2nd byte: instruction prefix
	};
}  // namespace VirtMac
/** @} */ //end of group Virtual Instruction Set Emulation
#endif
