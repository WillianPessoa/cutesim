add_executable(test_process test_process.cpp)
target_link_libraries(test_process PRIVATE cutesim_lib GTest::gtest_main)
gtest_discover_tests(test_process)
