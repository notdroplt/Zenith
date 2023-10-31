/**
 * @file group1.h
 * @author notdroplt (117052412+notdroplt@users.noreply.github.com)
 * @brief define the bitwise instruction group dispatch functions
 * @version 0.0.1
 * @date 2023-10-28
 *
 * @copyright Copyright (c) 2023
 */

#pragma once
#ifndef LIBSUPERNOVA_GROUP1
#define LIBSUPERNOVA_GROUP1 1
#include "supernova_private.h"

/**
 * @defgroup SNIG0 Supernova Instructions, Group 1
 *
 * @brief bitwise instructions
 *
 * Group 0 instructions are instructions that perform bitwise level operations between
 * two registers (last bit unset) or a register and an immediate (last bit set)
 *
 * @{
 */

/**
 * @brief bitwise and register:register
 *
 * set `rd` to the bitwise `and` between `r1` and `r2`
 *
 * ```
 * rd <- r1 & r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(andr_instrc);

/**
 * @brief bitwise and register:immediate
 *
 * set `rd` to the bitwise `and` between `r1` and an immediate value
 *
 * ```
 * rd <- r1 & imm
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(andi_instrc);

/**
 * @brief bitwise xor register:register
 *
 * set `rd` to the bitwise `exclusive or` between `r1` and `r2`
 *
 * ```
 * rd <- r1 ^ r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(xorr_instrc);

/**
 * @brief bitwise xor register:immediate
 *
 * set `rd` to the bitwise `exclusive or` between `r1` and an immediate
 *
 * ```
 * rd <- r1 ^ imm
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(xori_instrc);

/**
 * @brief bitwise or register:register
 *
 * set `rd` to the bitwise `or` between `r1` and `r2`
 *
 * ```
 * rd <- r1 | r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(orr_instrc);

/**
 * @brief bitwise or register:immediate
 *
 * set `rd` to the bitwise `or` between `r1` and an immediate
 *
 * ```
 * rd <- r1 | r2
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(ori_instrc);

/**
 * @brief bitwise not register
 *
 * set `rd` to the one's complement of `r1`, `r2` is unused
 *
 * ```
 * rd <- ~r1
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr exectued instruction
 */
sninstr_func(not_instrc);

/**
 * @brief popcount register
 *
 * set `rd` to the amount of bits set in `r1`, immediate is unused
 *
 * ```
 * rd <- popcnt(r1)
 * ```
 *
 * @param [in, out] thread current working thread
 * @param instr executed instruction
 */
sninstr_func(cnt_instrc);

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