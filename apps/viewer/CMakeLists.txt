find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

add_executable(cutesim_viewer)
target_sources(cutesim_viewer PRIVATE
    main.cpp
    MainWindow.cpp
)
target_link_libraries(cutesim_viewer PRIVATE
    cutesim_lib
    Qt6::Widgets
)
