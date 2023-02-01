project(libelf VERSION 0.1.0)


include(CTest)
enable_testing()

add_library(elf elfcreator.c)
include_directories(../../include)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
