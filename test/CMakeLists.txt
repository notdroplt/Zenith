include(CTest)
enable_testing()

add_executable(test_ihex ihex.c)
target_link_libraries(test_ihex PRIVATE output)

add_executable(test_vm vm.c)
target_link_libraries(test_vm PRIVATE vm)

add_executable(test_lex lex.c)
target_link_libraries(test_lex PRIVATE parse)

add_executable(test_array array.c)
target_link_libraries(test_array PRIVATE types)

add_test(NAME test_ihex COMMAND $<TARGET_FILE:test_ihex>)
add_test(NAME test_vm COMMAND $<TARGET_FILE:test_vm>)
add_test(NAME test_lex_int COMMAND $<TARGET_FILE:test_lex> int)
add_test(NAME test_lex_float COMMAND $<TARGET_FILE:test_lex> float)
add_test(NAME test_lex_string COMMAND $<TARGET_FILE:test_lex> str)
add_test(NAME test_array_type COMMAND $<TARGET_FILE:test_array>)