# add_library(vm STATIC virtualmachine/zenithvm.c virtualmachine/fetch_memory.c)

add_subdirectory(parsing)

add_subdirectory(compiler)

add_subdirectory(runners)

add_subdirectory(runasm)

add_library(output STATIC output/zrf.c output/ihex.c)

# add_library(debugger STATIC debugger/debugger.c debugger/disassembler.c )

# add_library(compiler STATIC compiler/compiler.c)

add_library(types STATIC types/list.c types/array.c types/hashtable.c types/hash.c)