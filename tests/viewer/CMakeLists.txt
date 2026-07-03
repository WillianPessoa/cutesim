find_package(Qt6 REQUIRED COMPONENTS Test QuickTest Quick)
qt_standard_project_setup(REQUIRES 6.2)

# ── C++ unit tests for SimController ──────────────────────────────────────
add_executable(test_sim_controller
    cpp/test_sim_controller.cpp
)
target_link_libraries(test_sim_controller PRIVATE
    cutesim_viewer_lib
    GTest::gtest
    Qt6::Test
)
add_test(NAME test_sim_controller COMMAND test_sim_controller)

# ── C++ unit tests for ScenarioBridge ──────────────────────────────────────
add_executable(test_scenario_bridge
    cpp/test_scenario_bridge.cpp
)
target_link_libraries(test_scenario_bridge PRIVATE
    cutesim_viewer_lib
    GTest::gtest
    Qt6::Test
)
add_test(NAME test_scenario_bridge COMMAND test_scenario_bridge)

# ── QML component tests ────────────────────────────────────────────────────
qt_add_executable(tst_viewer_qml
    qml/main.cpp
    qml/ViewerTestSetup.h
)
target_compile_definitions(tst_viewer_qml PRIVATE
    VIEWER_QML_BUILD_DIR="${CMAKE_BINARY_DIR}/apps/viewer"
)
target_link_libraries(tst_viewer_qml PRIVATE
    cutesim_viewer_lib
    cutesim_viewer_qml
    cutesim_viewer_qmlplugin
    cutesim_viewer_qmlplugin_init
    Qt6::QuickTest
)
add_test(NAME tst_viewer_qml
    COMMAND tst_viewer_qml -input ${CMAKE_CURRENT_SOURCE_DIR}/qml
)
