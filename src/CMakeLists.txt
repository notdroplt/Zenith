# add_library(vm STATIC virtualmachine/zenithvm.c virtualmachine/fetch_memory.c)

add_subdirectory(parsing)

add_subdirectory(compiler)

add_subdirectory(runners)

add_subdirectory(runasm)

add_library(types STATIC types/list.c types/array.c types/hashtable.c types/hash.c)
