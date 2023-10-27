include(CheckCXXSymbolExists)


add_library(typing STATIC typing.cc)
target_include_directories(typing PUBLIC ../)

target_compile_features(typing PUBLIC cxx_std_23)
target_compile_options(typing PUBLIC -std=c++23)