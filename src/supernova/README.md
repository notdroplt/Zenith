# Supernova Instruction Set

A reduced instruction set designed to be the first compiler target,
opcodes and behaviors might change in future releases.

## Instruction Layouts

This table defines how are the instruction types laid out, bit by bit,
with the most significant bit first.

R type instructions (R standing for register) are instructions that
use 3 registers and normally operate as `rd <- r1 ○ r2`, as `r1`
referring to the first argument (and not register r01) and r2
referring to the second argument (and not register r02). Register
`rd` is the destination register, nothing stops `rd` to be `r1` or
`r2`

instruction type | bits 63-23 | bits 22-18 | bits 17-13 | bits 12-8 | bits 7-0
:-:|:-:|:-:|:-:|:-:|:-:
R type | unused | rd | r2 | r1 | op

instruction type | bits 63-18 | bits 17-13 | bits 12-8 | bits 7-0
:-:|:-:|:-:|:-:|:-:
S type | immediate | rd | r1 | op

instruction type | bits 63-13 | bits 12-8 | bits 7-0
:-:|:-:|:-:|:-:
L type | immediate | r1 | op

## Register Layouts

`r00` is hardwired to zero, `r01` and `r02` are required for the interrupt stack

register index | used for
:-: | :-:
r00 | zero register
r01 | stack pointer
r02 | frame pointer
r03 - r31 | registers that work as arguments to functions

## Instruction Set Resume

- group zero: bitwise instruction group [opcodes `0x00 - 0x0F`]
  - andr: [opcode `0x00`, R type]
    - executes a bitwise AND between `r1` and `r2`, result on `rd`
    - executes: `rd <- r1 & r2`

  - andi: [opcode `0x01`, S type]
    - executes a bitwise AND between `r1` and a mask immediate, result on `rd`
    - executes: `rd <- r1 & imm`

  - xorr: [opcode `0x02`, R type]
    - executes a bitwise XOR between `r1` and `r2`, result on `rd`
    - executes: `rd <- r1 ^ r2`

  - xori: [opcode `0x03`, S type]
    - executes a bitwise XOR between `r1` and a mask immediate, result on `rd`
    - executes: `rd <- r1 ^ imm`

  - orr: [opcode `0x04`, R type]
    - executes a bitwise OR between `r1` and `r2`, result on `rd`
    - executes: `rd <- r1 | r2`

  - ori: [opcode `0x05`, S type]
    - executes a bitwise OR between `r1` and a mask immediate, result on `rd`
    - executes: `rd <- r1 | imm`

  - not: [opcode `0x06`, R type]
    - executes a one's complement on `r1`, discard `r2`, result on `rd`
    - executes: `rd <- ~r1`

  - cnt [opcode `0x07`, S type]
    - executes a population count on register `r1`, excluding the highest `imm` bits, result on `rd`
    - executes: `rd <- popcnt(r1 & ((1 << imm) - 1))`
    - edge case:
      - if `imm >= 64`, clear `rd`

  - llsr [opcode `0x08`, R type]
    - executes a logical left shift on register `r1` for `r2` bits, result on `rd`
    - executes: `rd <- r1 << r2`
    - edge case:
      - if `r2 >= 64`, clear `rd`

  - llsi [opcode `0x09`, S type]
    - executes a logical left shift on register `r1` for `imm` bits, result on `rd`
    - executes: `rd <- r1 << imm`
    - edge case:
      - if `imm >= 64`, clear `rd`

  - lrsr [opcode `0x0A`, R type]
    - executes a logical right shift on register `r1` for `r2` bits, result on `rd`
    - executes: `rd <- r1 >> r2`
    - edge case:
      - if `r2 >= 64`, clear `rd`

  - lrsi [opcode `0x0B`, S type]
    - executes a logical right shift on register `r1` for `imm` bits, result on `rd`
    - executes: `rd <- r1 >> imm`
    - edge case:
      - if `imm >= 64`, clear `rd`

  - alsr [opcode `0x0C`, R type]
    - executes a arithmetical left shift on register `r1` for `r2` bits, result on `rd`
    - executes: `rd <- r1 <<< r2`
    - edge case:
      - if `r2 >= 64`, clear `rd`

  - alsi [opcode `0x0D`, S type]
    - executes a arithmetical left shift on register `r1` for `imm` bits, result on `rd`
    - executes: `rd <- r1 <<< imm`
    - edge case:
      - if `imm >= 64`, clear `rd`

  - arsr [opcode `0x0E`, R type]
    - executes a arithmetical right shift on register `r1` for `r2` bits, result on `rd`
    - executes: `rd <- r1 >>> r2`
    - edge case:
      - if `r2 >= 64`, clear `rd`

  - arsi [opcode `0x0F`, S type]
    - executes a arithmetical right shift on register `r1` for `imm` bits, result on `rd`
    - executes: `rd <- r1 >>> imm`
    - edge case:
      - if `imm >= 64`, clear `rd`

- group one:
  - addr [opcode `0x10`, R type]
    - adds `r2` to `r1` and set `rd` as the result
    - executes: `rd <- r1 + r2`
    - edge case:
      - overflow is discarted

  - addi [opcode `0x11`, S type]
    - adds `imm` to `r1` and set `rd` as the result
    - executes: `rd <- r1 + imm`
    - edge case:
      - overflow is discarted

  - subr [opcode `0x12`, R type]
    - subtracts `r2` from `r1` and set `rd` as the result
    - executes: `rd <- r1 - r2`
    - edge case:
      - overflow is discarted

  - subi [opcode `0x13`, S type]
    - subtracts `imm` from `r1` and set `rd` as the result
    - executes: `rd <- r1 - imm`
    - edge case:
      - overflow is discarted

  - umulr [opcode `0x14`, R type]
    - multiplies `r2 (unsigned)` with `r1 (unsigned)` and set `rd` as the result
    - executes: `rd <- (uint64_t)r1 * (uint64_t)r2`
    - edge case:
      - overflow is discarted

  - umuli [opcode `0x15`, S type]
    - multiplies `imm (unsigned)` with `r1 (unsigned)` and set `rd` as the result
    - executes: `rd <- (uint64_t)r1 * (uint64_t)imm`
    - edge case:
      - overflow is discarted

  - smulr [opcode `0x16`, R type]
    - multiplies `r2 (signed)` with `r1 (signed)` and set `rd` as
    - executes: `rd <- (int64_t)r1 * (int64_t)r2`
    - edge case:
      - overflow is discarted

  - smuli [opcode `0x17`, S type]
    - multiplies `imm` with `r1` and set `rd` as the result
    - executes: `rd <- (int64_t)r1 * (int64_t)imm`
    - edge case:
      - overflow is discarted

  - udivr [opcode `0x18`, R type]
    - divides `r1 (unsigned)` by `r2 (unsigned)` and set `rd` as the result
    - executes: `rd <- (uint64_t)r1 / (uint64_t)r2`
    - edge case:
      - overflow is discarted
      - `r2 = 0` triggers `pcall 1`

  - udivi [opcode `0x19`, S type]
    - divides `r1 (unsigned)` by `imm (unsigned)` and set `rd` as the result
    - executes: `rd <- (uint64_t)r1 / (uint64_t)imm`
    - edge case:
      - overflow is discarted
      - `imm = 0` triggers `pcall 1`

  - sdivr [opcode `0x1A`, R type]
    - divides `r1 (signed)` by `r2 (signed)` and set `rd` as
    - executes: `rd <- (int64_t)r1 / (int64_t)r2`
    - edge case:
      - overflow is discarted
      - `r2 = 0` triggers `pcall 1`

  - sdivi [opcode `0x1B`, S type]
    - divides `r1 (signed)` by `imm (signed)` and set `rd` as the result
    - executes: `rd <- (int64_t)r1 / (int64_t)imm`
    - edge case:
      - overflow is discarted
      - `imm = 0` triggers `pcall 1`

  - call [opcode `0x1C`, R type]
    - change execution context to another place
    - semantic renaming: `call rd, r1, r2` -> `call addr, sp, bp`
    - executes:
      - `u64[sp + 0] <- bp`
      - `u64[sp + 8] <- pc + 8`
      - `sp <- sp + 16`
      - `bp <- sp`
      - `pc <- addr`

  - push [opcode `0x1D`, S type]
    - push a value into given stack
    - semantic renaming `push rd, r1, imm` -> `push sp, rv, imv`
    - executes:
      - `u64[sp] <- rv + imv`
      - `sp <- sp + 8`

  - retn [opcode `0x1E`, R type]
    - return execution to previous context
    - semantic renaming `retn rd, r1, r2` -> `retn x0, sp, bp`
    - executes:
      - `sp <- sp - 16`
      - `bp <- u64[sp + 0]`
      - `pc <- u64[sp + 8]`
    - `rd` is ignored

  - pull [opcode `0x1F`, S type]
    - pull a value out of a given stack
    - semantic renaming `pull rd, r1, imm` -> `pull rd, sp, imm`
    - executes:
      - `rd <- u64[sp - 8]`
      - `sp <- sp - 8`
    - `imm` is ignored

- group two:
  - ldb [opcode `0x20`, S type]
    - load byte from memory into a register
    - executes: `rd <- r0 | u8[r1 + imm]`
    - side effects:
      - if `r1 + imm` is bigger than memory size, `pcall 4` is triggered

  - ldh [opcode `0x21`, S type]
    - load half word from memory into a register
    - executes: `rd <- r0 | u16[r1 + imm]`
    - side effects:
      - if `r1 + imm` is bigger than memory size, `pcall 4` is triggered

  - ldw [opcode `0x22`, S type]
    - load word from memory into a register
    - executes: `rd <- r0 | u32[r1 + imm]`
    - side effects:
      - if `r1 + imm` is bigger than memory size, `pcall 4` is triggered

  - ldd [opcode `0x23`, S type]
    - load double word from memory into a register
    - executes: `rd <- u64[r1 + imm]`
    - side effects:
      - if `r1 + imm` is bigger than memory size, `pcall 4` is triggered

  - stb [opcode `0x24`, S type]
    - store byte from register into memory
    - executes: `u8[rd + imm] <- r1 & 0xff`
    - side effects:
      - if `rd + imm` is bigger than memory size, `pcall 4` is triggered

  - sth [opcode `0x25`, S type]
    - store half word from register into memory
    - executes: `u16[rd + imm] <- r1 & 0xffff`
    - side effects:
      - if `rd + imm` is bigger than memory size, `pcall 4` is triggered

  - stw [opcode `0x26`, S type]
    - store word from register into memory
    - executes: `u32[rd + imm] <- r1 & 0xffffffff`
    - side effects:
      - if `rd + imm` is bigger than memory size, `pcall 4` is triggered

  - std [opcode `0x27`, S type]
    - store half word from register into memory
    - executes: `u64[rd + imm] <- r2`
    - side effects:
      - if `rf + imm` is bigger than memory size, `pcall 4` is triggered

  - jal [opcode `0x28`, L type]
    - jump to a place in memory
    - executes:
      - `rd <- pc + 8`
      - `pc <- pc + imm`

  - jalr [opcode `0x29`, S type]
    - jump to a place in memory
    - executes:
      - `rd <- pc + 8`
      - `pc <- pc + r1 + imm`

  - je [opcode `0x2A`, S type]
    - jump to a place in memory when `rd == r1`
    - executes:
      - `pc <- rd ^ r1 ? pc + imm * 8 : pc + 8`

  - jne [opcode `0x2B`, S type]
    - jump to a place in memory when `rd != r1`
    - executes:
      - `pc <- rd ^ r1 ? ? pc + imm * 8 : pc + 8`

  - jgu [opcode `0x2C`, S type]
    - jump to a place in memory when `rd > r1`, both unsigned
    - executes:
      - `pc <- (rd - r1) & (sign bit) ? pc + 8 : pc + imm * 8`

  - jleu [opcode `0x2D`, S type]
    - jump to a place in memory when `rd <= r1`
    - executes:
      - `pc <- (rd - r1) & (sign bit) ? pc + 8 :`

## interrupts

### default interrupts/exceptions used by the virtual machine

- `pcall -1`: [Processor interface](#pcall--1)
- `pcall 0`: Divison by zero
- `pcall 1`: General fault
- `pcall 2`: Double fault
- `pcall 3`: Triple fault
- `pcall 4`: Invalid instruction
- `pcall 5`: Page fault
- `pcall 6`: Invalid IO

everything after this is programmable (in theory), but some different
implementations might use other values

#### `pcall 0`: Division by zero

As the name suggests, this program call is triggered every time there is
a division by zero on the program. A compiler can simply put a divide by zero
instruction on a program and call it a breakpoint

#### `pcall 1`: General Fault

General faults occur by any kind of unhandled exception the processor is not
able to detect or recognize

#### `pcall 2`: Double Fault

A double fault occurs when any interrupt is called/triggered by a general fault

#### `pcall 3`: Triple Fault

A triple fault is one of the fatal faults inside the processor.
Causing it possibly triggers hardware functions for resetting

## `pcall -1`

Only `pcall -1` is hardware/vm defined, all the other $2^{51}-1$ possible interrupts are programmable with a call to `pcall -1`:

The interface defines `r29:r28` as the interrupt space and functionality switches, while other registers are used accordingly as each function needs.
Normally, `r28 = 0` will be a `r29` implementation check, behaving as a `xorr r31, r31` in case `r29`'s feature is not implmemented.

**`pcall -1` functions**

- `r29 = 0`: [interrupt vector functions](#interrupt-vector-interrupt-space)
- - `r28 = 0`: [interrupt vector check](#interrupt-vector-check)
- - `r28 = 1`: [interrupt vector enable](#interrupt-vector-enable)
- `r29 = 1`: [paging functions](#paging-interrupt-space)
- - `r28 = 0`: [paging check](#paging-check)
- - `r28 = 1`: [paging enable](#paging-enable)
- `r29 = 2`: [model information](#model-information-interrupt-space)
- - `r28 = 0`: [model check](#model-check)
- - `r28 = 1`: []
- `r29 = 3`: [hyper functions](#hyper-function-interrupt-space)
- - `r28

---

### interrupt vector interrupt space

#### interrupt vector check

input registers: none

output registers:

- `r31`: `0` if no interrupts are possible, `pcall 0:0` is just a shadow to `add r31, r0, r0`,  
         `1` if interrupts are possible, but only in the address specified by `r29`,  
         `2` if interrupts are possible anywhere defined by the program,  

- `r30`: in case `r31 != 0`, defines the amount of interrupts the processor is able to handle

- `r29`: in case `r31 == 1`, sets bit flags to which hardware interrupts are supported

trashed registers: none

#### interrupt vector enable

input registers:

- `r31` (possibly): in case where `pcall 0:0` returned `2`, set the interrupt vector register to the specified pointer, else value is just ignored

output registers: none

trashed registers: none

---

### paging interrupt space

#### paging check

input registers: none

output registers:

- `r31`: set to the processor's amount of page level reach, can assume either `0` or `≥ 2`
- `r30`: in case `r31` is not zero, returns the processor's page size

trashed registers: none

#### paging enable

input registers:

- `r30`:

#### `r28 = 1`, `r30 = page root directory`

Enable paging, with `r30` set to the memory location of the first page root directory location

---

### model information interrupt space

#### information check

input registers: none

output registers:

- `r31`: set if the processor is able to give more information about itself

trashed registers: none
