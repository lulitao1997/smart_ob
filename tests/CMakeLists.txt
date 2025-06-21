add_executable(unit_tests test.cpp test_generate.cpp)
target_link_libraries(unit_tests
    PRIVATE
    gtest_main
    smart_ob_lib)
add_test(NAME unit_tests COMMAND unit_tests)

# Enable testing
enable_testing()