cmake_minimum_required(VERSION 3.13.0)

project(zenith)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

include_directories(include)

add_subdirectory(src/debugger)
add_subdirectory(src/parsing)
add_subdirectory(src/virtualmachine)
add_subdirectory(src/compiler)

add_executable(zenith src/main.cc)

target_link_libraries(zenith PRIVATE zvm)
target_link_libraries(zenith PRIVATE zdb)
target_link_libraries(zenith PRIVATE zparser)
target_link_libraries(zenith PRIVATE zcompiler)