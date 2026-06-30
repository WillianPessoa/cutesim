add_executable(test_scenario
    test_scenario.cpp
)
target_include_directories(test_scenario PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(test_scenario PRIVATE cutesim_lib GTest::gtest_main)
gtest_discover_tests(test_scenario)
