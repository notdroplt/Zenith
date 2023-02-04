cmake_minimum_required(VERSION 3.21.0)

project(zenith LANGUAGES C CXX)

set(CMAKE_C_FLAGS_DEBUG   "-O0 -g3")
set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g3")

# remove flags after -O3 if releasing online
set(CMAKE_C_FLAGS_RELEASE "-O3 -flto -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -march=native -mtune=native")


add_compile_options(-Wall -Wextra -pedantic -Werror)

include(CMakePrintHelpers)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

enable_testing()

include_directories(src)
include_directories(include)

set_property(GLOBAL PROPERTY C_STANDARD 17)
set_property(GLOBAL PROPERTY CXX_STANDARD 14)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_PARALLEL_LEVEL 4)

add_subdirectory(src/debugger)
add_subdirectory(src/parsing)
add_subdirectory(src/virtualmachine)
add_subdirectory(src/compiler)
add_subdirectory(src/output)

add_subdirectory(test)

add_executable(zenith src/main.cc)

target_link_libraries(zenith PRIVATE vm)
target_link_libraries(zenith PRIVATE debugger)
target_link_libraries(zenith PRIVATE parse)
target_link_libraries(zenith PRIVATE compiler)
target_link_libraries(zenith PRIVATE output)
