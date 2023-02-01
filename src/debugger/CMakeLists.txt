project(libdebugger VERSION 0.1.0)


include(CTest)
enable_testing()

add_library(debugger debugger.c disassembler.c )
include_directories(../../include)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
