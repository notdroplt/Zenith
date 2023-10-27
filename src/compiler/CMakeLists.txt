add_library(compiler generator.cc)
target_include_directories(compiler PUBLIC ../)

target_compile_features(compiler PUBLIC cxx_std_23)
target_compile_options(compiler PUBLIC -std=c++23)