include(CTest)
enable_testing()

add_executable(test_ihex ihex.c)
target_link_libraries(test_ihex PRIVATE output)

add_executable(test_vm vm.c)
target_link_libraries(test_vm PRIVATE vm)

add_test(NAME test_ihex COMMAND $<TARGET_FILE:test_ihex>)
add_test(NAME test_vm COMMAND $<TARGET_FILE:test_vm>)
