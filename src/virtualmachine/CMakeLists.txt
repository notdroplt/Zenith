project(libvm VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(vm zenithvm.c fetch_memory.c)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CPack)