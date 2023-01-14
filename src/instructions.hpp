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




#include <cstdint>

/**
 * \defgroup VirMac Virtual Instruction Set Emulation
 * \brief all the instruction prefixes used on the emulated vm cpu, all designed by me
 *
 * instructions follow something like risc-v, but with some patches and differences
 *
 * 
 * 
 * @{
 */

namespace VirtMac
{
	extern "C" {
		#include "virtualmachine.h"
	};
	

	/**
	 * @brief R type instruction layout
	 *
	 * Size : 8 bytes
	 */
	struct RInstruction : public r_block_t
	{
		RInstruction(const uint8_t opcode_, const uint8_t r1_, const uint8_t r2_, const uint8_t rd_) 
		{
			this->opcode = opcode_;
			this->r1 = r1_;
			this->r2 = r2_;
			this->rd = rd_;
			this->pad = 0;
		}

		operator uint64_t() const noexcept
		{
			return *(uint64_t *)this;
		}

	};

	/**
	 * @brief S type instruction layout
	 *
	 */
	struct SInstruction : public s_block_t
	{
		SInstruction(const uint8_t opcode_, const uint8_t r1_, const uint8_t rd_, const uint64_t immediate_) {
			this->opcode = opcode_;
			this->r1 = r1_;
			this->rd = rd_;
			this->immediate = immediate_;
		}

		operator uint64_t() const noexcept
		{
			return *(uint64_t *)this;
		}
	} __attribute__((packed));

	/**
	 * @brief L type instruction layout
	 *
	 */
	struct LInstruction : public l_block_t
	{
		LInstruction(const uint8_t opcode_, const uint8_t r1_, const uint64_t immediate_)
		{
			this->opcode = opcode_;
			this->r1 = r1_;
			this->immediate = immediate_;
		}

		operator uint64_t() const noexcept
		{
			return *(uint64_t *)this;
		}
	} __attribute__((packed));

} // namespace VirtMac
/** @} */ // end of group Virtual Instruction Set Emulation
#endif
