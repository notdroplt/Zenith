cmake_minimum_required(VERSION 3.21.0)

project(zenith LANGUAGES C)

# default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# add compile options to all types of build and 
if (CMAKE_BUILD_TYPE EQUAL "Release")
  add_compile_options(-D_FORTIFY_SOURCE=1)
endif()
add_compile_options(-Wall -Wextra -pedantic -Werror -Wunused -Wdeclaration-after-statement -fstack-protector-strong -pedantic-errors -Wformat=2 -Wstack-usage=384 -fanalyzer) 

include_directories(src/ include/ src/types/)

# include actual code
add_subdirectory(src)

# add test suits
enable_testing()
add_subdirectory(test)

add_executable(zenith src/main.c)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)
set_property(TARGET zenith PROPERTY C_STANDARD 99) 

# link libraries
target_include_directories(zenith PUBLIC src/types/)

target_link_libraries(zenith PRIVATE vm debugger parse compiler output dotenv types)
