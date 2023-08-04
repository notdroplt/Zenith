# runasm

runasm is the way i found to balance work with multiple computer architectures without building a different compiler for each one of them and not degrading performance, by making a 

a runasm file is a representation of an entire cpu from the compiler's perspective, which is basically available registers, (possibly) memory mapped areas and things instruction formatting

the main structure consists of 

```runasm 
ISA [instruction set name]

manual [link to manual if it exists]

registers
    [definition of registers and their sizes]
registers

ext_registers
    [in case some registers are shadows from other already existing registers, they are defined here]
ext_registers

flags
    [if present, define compiler flags]
flags

format [format name]

instruction [instruction definition]
```

the file does need to follow that order, as some places are optional and others required, all of those will be talked more about later in the readme file

### why does it compile to a bytecode

the runner consists of two phases

### `ISA` directive

A required directive and can occur only once per `.runasm` file, it tells the runner which instruction set the following file defines constraints for.

```
ISA name[:variant[:variants]...]
```

### `manual` directive

An optional directive, can occur multiple times per file, which makes the work of a copyright notice in some way, completely ignored by the runner

```
manual link_to_manual
```

### `width` directive

A required directive that tells the runner the default bus width of the target

```
width [bit count]bit
```

the bytecode compiler will emit an error if the `bit count` is not on the standard $2^n$ 

### `registers` block-directive

Even if memory mapped, the `registers` block-directive is required, marks available registers for the compiler to use, every register written there must not be a register that
have parts mapped into other registers (like `ax`'s values are shadowed into ah:al on Intel x86)

every register entry consists of `register_name:bitsize`

the bytecode compiler will emit an error if the 
