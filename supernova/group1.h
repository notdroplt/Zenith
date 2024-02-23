/**
 * @file group1.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief define the bitwise instruction group dispatch functions
 * @version 0.0.1
 * @date 2023-11-12
 *
 * @copyright Copyright (c) 2023
 */

#pragma once
#ifndef LIBSUPERNOVA_GROUP1
#define LIBSUPERNOVA_GROUP1 1
#include "supernova_private.h"

/**
 * @defgroup SNIG1 Supernova Instructions, Group 1
 *
 * @brief arithmetic instructions
 *
 * Group 1 instructions are instructions that perform basic arithimetic in signed and unsigned integers
 * 
 * @note Division instructions are implemented in the virtual machine, but don't need to be on real hardware
 *
 * @{
 */

/**
 * @brief add register:register
 *
 * set `rd` to the sum of `r1` and `r2`
 *
 * ```
 * rd <- r1 + r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(addr_instrc);

/**
 * @brief add register:immediate
 *
 * set `rd` to the sum of `r1` and an immediate value
 *
 * ```
 * rd <- r1 + imm
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(addi_instrc);

/**
 * @brief subtract register:register
 *
 * set `rd` to the result of subtracting `r2` from `r1`
 *
 * ```
 * rd <- r1 - r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(subr_instrc);

/**
 * @brief subtract register:immediate
 *
 * set `rd` to the result of subtracting an immediate from `r1`
 *
 * ```
 * rd <- r1 - imm
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(subi_instrc);

/**
 * @brief unsigned multiply register:register
 *
 * set `rd` to the multiplication between `r1` and `r2`, with both registers
 * being treated as unsigned integers
 *
 * ```
 * rd <- r1 * r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(umulr_instrc);

/**
 * @brief unsigned multiply register:immediate
 *
 * set `rd` to the multiplication between `r1` and an immediate, both treated as unsigned integers
 *
 * ```
 * rd <- r1 * imm
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(umuli_instrc);

/**
 * @brief signed multiply register:register
 *
 * set `rd` to the multiplication between `r1` and `r2`, both treated as signed integers
 *
 * ```
 * rd <- (signed)r1 * (signed)r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr exectued instruction
 */
sninstr_func(smulr_instrc);

/**
 * @brief signed multiply register:immediate
 *
 * set `rd` to the multiplication between `r1` and an immediate, both treated as signed integers
 *
 * ```
 * rd <- (signed)r1 * (signed)immediate
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(smuli_instrc);

/**
 * @brief logical left shift by register
 *
 * set `rd` to the value of `r1` (unsigned) shifted `r2` places to the left
 *
 * in the case that `r2` is greater than the register size, `rd` is set to zero
 *
 * ```
 * if r2 > regsize
 *  rd <- 0
 * else
 *  rd <- r1 << r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(llsr_instrc);

/**
 * @brief logical left shift by immediate
 *
 * set `rd` to the value of `r1` (unsigned) shifted to the left by an immediate value
 *
 * in case that the value is greater than the register size, `rd` is set to zero
 *
 * ```
 * if imm > regsize
 *  rd <- 0
 * else
 *  rd <- r1 << imm
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(llsi_instrc);

/**
 * @brief logical right shift by register
 *
 * set `rd` to the value of `r1` (unsigned) shifted `r2` places to the right
 *
 * in the case that `r2` is greater than the register size, `rd` is set to zero
 *
 * ```
 * if r2 > regsize
 *  rd <- 0
 * else
 *  rd <- r1 >> r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(lrsr_instrc);

/**
 * @brief logical right shift by immediate
 *
 * set `rd` to the value of `r1` (unsigned) shifted to the right by an immediate value
 *
 * in case that the value is greater than the register size, `rd` is set to zero
 *
 * ```
 * if imm > regsize
 *  rd <- 0
 * else
 *  rd <- r1 >> imm
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(lrsi_instrc);

/**
 * @brief arithmetical left shift by register
 *
 * set `rd` to the value of `r1` (signed) shifted `r2` places to the left
 *
 * in the case that `r2` is greater than the register size, `rd` is set to zero
 *
 * ```
 * if r2 > regsize
 *  rd <- 0
 * else
 *  rd <- r1 <<< r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(alsr_instrc);

/**
 * @brief arithmetical left shift by immediate
 *
 * set `rd` to the value of `r1` (signed) shifted to the left by an immediate value
 *
 * in case that the value is greater than the register size, `rd` is set to zero
 *
 * ```
 * if imm > regsize
 *  rd <- 0
 * else
 *  rd <- r1 <<< imm
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(alsi_instrc);

/**
 * @brief arithmetical right shift by register
 *
 * set `rd` to the value of `r1` (signed) shifted `r2` places to the right
 *
 * in the case that `r2` is greater than the register size, `rd` is set to zero
 *
 * ```
 * if r2 > regsize
 *  rd <- 0
 * else
 *  rd <- r1 >>> r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(arsr_instrc);

/**
 * @brief arithmetical right shift by immediate
 *
 * set `rd` to the value of `r1` (signed) shifted to the right by an immediate value
 *
 * in case that the value is greater than the register size, `rd` is set to zero
 *
 * ```
 * if imm > regsize
 *  rd <- 0
 * else
 *  rd <- r1 >>> imm
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(arsi_instrc);

/** @} */

#endif