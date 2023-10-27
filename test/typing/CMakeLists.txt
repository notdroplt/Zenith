create_test_sourcelist(TypingTests
    common.cc
    different.cc
    same.cc
    subtype.cc
    supertype.cc
)

add_executable(TypeTests ${TypingTests})

target_link_libraries(TypeTests typing)

set(TestsToRun ${TypingTests})
remove(TestsToRun common.cc)

foreach(test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    add_test (NAME ${TName} COMMAND TypeTests ${TName})
endforeach()

