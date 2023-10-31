cmake_minimum_required(VERSION 3.18.0)
project(Zenith LANGUAGES C CXX VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -ggdb")
add_compile_options(-Wall -Wextra -pedantic -Werror -pedantic-errors -Wunused -fstack-protector-strong -Wformat=2 -fno-exceptions)

add_executable(zenith src/main.cc)

target_compile_features(zenith PUBLIC cxx_std_23)
target_compile_options(zenith PUBLIC -std=c++23)

target_include_directories(zenith PUBLIC src)
add_subdirectory(src)

target_link_libraries(zenith PRIVATE parse typing ${CMAKE_DL_LIBS})

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)
add_subdirectory(test)

enable_testing()

set(CPACK_PACKAGE_VENDOR "notdroplt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Zenith development kit")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_BINARY_STGZ "ON")
set(CPACK_BINARY_TBZ2 "ON")
set(CPACK_BINARY_TGZ "ON")
set(CPACK_BINARY_TXZ "ON")
set(CPACK_BINARY_TZ "ON")
include(CPack)