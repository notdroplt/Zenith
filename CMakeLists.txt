cmake_minimum_required(VERSION 3.21.0)

SET (CMAKE_C_COMPILER             "/usr/bin/gcc")

project(zenith LANGUAGES C CXX)

# default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

find_program(MOLD_LINKER "mold")

# use mold if present
if(MOLD_LINKER)
  add_compile_options(-fuse-ld=mold)
endif() 

# add compile options to all types of build and 
if (CMAKE_BUILD_TYPE EQUAL "Release")
add_compile_options(-D_FORTIFY_SOURCE=1)
endif()
add_compile_options(-Wall -Wextra -pedantic -Werror -fanalyzer) 

enable_testing()

include_directories(src include)



# include actual code
add_subdirectory(src/debugger)
add_subdirectory(src/parsing)
add_subdirectory(src/virtualmachine)
add_subdirectory(src/compiler)
add_subdirectory(src/output)
add_subdirectory(src/dotenv)
add_subdirectory(src/types)

# add test suits
add_subdirectory(test)

add_executable(zenith src/main.c)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)
set_property(TARGET zenith PROPERTY C_STANDARD 99) 

# link libraries
target_link_libraries(zenith PRIVATE vm debugger parse compiler output dotenv types)