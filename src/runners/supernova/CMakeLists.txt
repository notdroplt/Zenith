cmake_minimum_required(VERSION 3.9)

project(supernova VERSION 1.0.0 DESCRIPTION "shared library for supernova-vm")

include(GNUInstallDirs)

add_library(supernova SHARED supernova.c)

set_target_properties(supernova PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER supernova.h)

target_include_directories(zenith PRIVATE .)