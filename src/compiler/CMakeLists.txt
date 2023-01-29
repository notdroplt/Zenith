project(libzvm VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(zcompiler compiler.cc)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
