find_package(Qt6 REQUIRED COMPONENTS Quick Network)
qt_standard_project_setup(REQUIRES 6.2)

set_source_files_properties(qml/theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(cutesim_viewer
    main.cpp
    src/SimClient.cpp
    src/SimClient.h
    src/SimController.cpp
    src/SimController.h
)

qt_add_qml_module(cutesim_viewer
    URI CuteSim.Viewer
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/components/GlassCard.qml
        qml/components/Logo.qml
        qml/components/StatusChip.qml
        qml/components/TickChip.qml
        qml/components/CtrlButton.qml
        qml/components/SpinBox.qml
        qml/components/Toast.qml
        qml/components/Sparkline.qml
        qml/components/StatItem.qml
        qml/components/QueueChip.qml
        qml/components/QueueView.qml
        qml/components/Header.qml
        qml/components/CPUView.qml
        qml/components/TimelineChart.qml
        qml/components/StatsBar.qml
        qml/components/FinishedTable.qml
        qml/components/LaunchOverlay.qml
        qml/components/InspectorPanel.qml
)

target_include_directories(cutesim_viewer PRIVATE src)
target_link_libraries(cutesim_viewer PRIVATE cutesim_lib Qt6::Quick Qt6::Network)
