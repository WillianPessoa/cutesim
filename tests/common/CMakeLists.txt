add_executable(test_snapshot test_snapshot.cpp)
target_link_libraries(test_snapshot PRIVATE cutesim_common GTest::gtest_main)
gtest_discover_tests(test_snapshot)

add_executable(test_emit_file test_emit_file.cpp)
target_link_libraries(test_emit_file PRIVATE cutesim_common GTest::gtest_main)
gtest_discover_tests(test_emit_file)

add_executable(test_emit_tcp test_emit_tcp.cpp)
target_link_libraries(test_emit_tcp PRIVATE cutesim_common GTest::gtest_main)
gtest_discover_tests(test_emit_tcp)
