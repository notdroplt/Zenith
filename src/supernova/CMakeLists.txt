include(GNUInstallDirs)

set(supernovasrc supernova.c group0.c)

add_library(supernova_obj OBJECT ${supernovasrc})

set_property(TARGET supernova_obj PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(supernova_shared SHARED $<TARGET_OBJECTS:supernova_obj>)
add_library(supernova_static STATIC $<TARGET_OBJECTS:supernova_obj>)


