# Unit tests — parse_args and display
add_executable(test_cli test_cli.cpp)
target_link_libraries(test_cli PRIVATE rr_feedback_args GTest::gtest_main)
gtest_discover_tests(test_cli)

# Integration tests — invoke the real binary as a subprocess
add_executable(test_binary test_binary.cpp)
target_link_libraries(test_binary PRIVATE GTest::gtest_main)
target_compile_definitions(test_binary PRIVATE
    RR_FEEDBACK_BIN="$<TARGET_FILE:rr-feedback>"
)
add_dependencies(test_binary rr-feedback)
gtest_discover_tests(test_binary)
