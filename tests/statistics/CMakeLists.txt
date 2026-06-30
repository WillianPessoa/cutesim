add_executable(test_statistics
    test_statistics.cpp
)
target_include_directories(test_statistics PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(test_statistics PRIVATE cutesim_lib GTest::gtest_main)
gtest_discover_tests(test_statistics)
