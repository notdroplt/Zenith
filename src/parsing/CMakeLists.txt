add_library(parse STATIC parser.cc lex.cc)
target_include_directories(parse PUBLIC ../)

target_compile_features(parse PUBLIC cxx_std_23)
target_compile_options(parse PUBLIC -std=c++23)