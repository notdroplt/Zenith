add_subdirectory(types)

add_library(vm STATIC virtualmachine/zenithvm.c virtualmachine/fetch_memory.c)

add_library(parse STATIC parsing/lex.c parsing/node.c parsing/parse.c parsing/optimizing.c)

add_library(output STATIC output/zrf.c output/ihex.c)

add_library(dotenv STATIC dotenv/dotenv.c)

add_library(debugger STATIC debugger/debugger.c debugger/disassembler.c )

add_library(compiler STATIC compiler/compiler.c)

target_link_libraries(compiler vm)