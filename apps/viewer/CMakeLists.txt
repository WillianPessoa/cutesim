find_package(Qt6 REQUIRED COMPONENTS Quick Network)
qt_standard_project_setup(REQUIRES 6.2)

# Theme.qml must be marked as a singleton so qt_add_qml_module emits the
# correct qmldir entry; the QML file itself also carries `pragma Singleton`.
set_source_files_properties(qml/theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(cutesim_viewer main.cpp)

qt_add_qml_module(cutesim_viewer
    URI CuteSim.Viewer
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
)

target_link_libraries(cutesim_viewer PRIVATE cutesim_lib Qt6::Quick Qt6::Network)
