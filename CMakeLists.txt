cmake_minimum_required(VERSION 3.21.0)

project(zenith)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(src)
include_directories(include)

set_property(GLOBAL PROPERTY C_STANDARD 17)
set_property(GLOBAL PROPERTY CXX_STANDARD 14)

add_subdirectory(src/debugger)
add_subdirectory(src/parsing)
add_subdirectory(src/virtualmachine)
add_subdirectory(src/compiler)
add_subdirectory(src/elf_formatter)

add_executable(zenith src/main.cc)


target_link_libraries(zenith PRIVATE vm)
target_link_libraries(zenith PRIVATE debugger)
target_link_libraries(zenith PRIVATE parse)
target_link_libraries(zenith PRIVATE compiler)
target_link_libraries(zenith PRIVATE elf)
