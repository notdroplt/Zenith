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

install(TARGETS zenith DESTINATION bin)