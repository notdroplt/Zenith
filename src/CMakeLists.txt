add_library(vm virtualmachine/zenithvm.c virtualmachine/fetch_memory.c)

add_library(parse parsing/lex.c parsing/node.c parsing/parse.c parsing/optimizing.c)

add_library(output output/zrf.c output/ihex.c)

add_library(dotenv dotenv/dotenv.c)

add_library(debugger debugger/debugger.c debugger/disassembler.c )

add_library(compiler compiler/compiler.c)
target_link_libraries(compiler vm)