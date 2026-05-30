add_executable(test_simulation
    test_round_robin.cpp
    test_feedback.cpp
    test_io.cpp
    test_completion.cpp
)
target_include_directories(test_simulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(test_simulation PRIVATE cutesim_lib GTest::gtest_main)
gtest_discover_tests(test_simulation)
