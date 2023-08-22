cmake_minimum_required(VERSION 3.18.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(RELENV $ENV{ReleaseEnvironment})

if(DEFINED RELENV)
  message(NOTICE $ENV{ReleaseEnvironment})
  message(NOTICE "-- Docker or Release detected!" )
  set(CMAKE_BUILD_TYPE "Release")
endif()

project(zenith LANGUAGES C CXX)

add_compile_options(-Wall -Wextra -pedantic -Werror -pedantic-errors -Wunused -fstack-protector-strong -Wformat=2 -fno-exceptions )


set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)


include_directories(src)

add_executable(zenith src/main.cc)

include_directories(src/common/)

add_subdirectory(src)
target_link_libraries(zenith PRIVATE parse compile output types frontend loader ${CMAKE_DL_LIBS})
target_link_options(zenith PRIVATE -flto)

set(CMAKE_C_CLANG_TIDY "clang-tidy;-checks=*")
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)
enable_testing()

#  add_executable(test_ihex test/test_ihex.c)
#  add_executable(test_vm test/test_vm.c)
#  add_executable(test_lex test/test_lex.c)
#  add_executable(test_array test/test_array.c)
#  add_executable(test_list test/test_list.c)

#  target_link_libraries(test_ihex PRIVATE output)
#  target_link_libraries(test_vm PRIVATE vm compiler)
#  target_link_libraries(test_lex PRIVATE parse)
#  target_link_libraries(test_array PRIVATE types) 
#  target_link_libraries(test_list PRIVATE types)

#  add_test(NAME test_ihex COMMAND $<TARGET_FILE:test_ihex>)
#  add_test(NAME test_vm COMMAND $<TARGET_FILE:test_vm>)
#  add_test(NAME test_lex_int COMMAND $<TARGET_FILE:test_lex> int)
#  add_test(NAME test_lex_float COMMAND $<TARGET_FILE:test_lex> float)
#  add_test(NAME test_lex_string COMMAND $<TARGET_FILE:test_lex> str)
#  add_test(NAME test_array_type COMMAND $<TARGET_FILE:test_array>)
#  add_test(NAME test_list_type COMMAND $<TARGET_FILE:test_list>)

install(TARGETS zenith DESTINATION bin)