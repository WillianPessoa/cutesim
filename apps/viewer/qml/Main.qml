import QtQuick
import CuteSim.Viewer
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: window
    visible: true
    visibility: Window.FullScreen
    width: 1440
    height: 900
    color: Theme.bg
    title: "CuteSim"

    Rectangle {
        anchors.fill: parent
        color: Theme.bg
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Theme.accentSoft }
            GradientStop { position: 0.4; color: "transparent" }
            GradientStop { position: 1.0; color: "transparent" }
        }
        opacity: 0.5
    }

    property bool inspectorOpen: false
    property int selectedPid: -1

    readonly property var cpuFlash: {
        if (!controller) return null
        var evts = controller.events
        for (var i = 0; i < evts.length; i++) {
            if (evts[i].type === "preempted")
                return { pid: evts[i].pid, used: evts[i].quantum_used, max: evts[i].quantum_max }
        }
        return null
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Header {
            id: header
            Layout.fillWidth: true
            tick:      controller ? controller.tick      : 0
            status:    !controller || !controller.connected
                        ? (controller && controller.simDone ? "done" : "disconnected")
                        : "connected"
            connected: controller ? controller.connected : false
            simDone:   controller ? controller.simDone   : false
            onStepRequested:            if (controller) controller.step()
            onResetRequested:           if (controller) controller.reset()
            onReconfigureRequested:     if (controller) controller.reconfigure()
            onThemeToggleRequested:     Theme.toggle()
            onInspectorToggleRequested: window.inspectorOpen = !window.inspectorOpen
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.gap
                spacing: Theme.gap

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Theme.gap

                    // Row 1: CPU + queues
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 188
                        spacing: Theme.gap

                        CPUView {
                            Layout.preferredWidth: 340
                            Layout.fillHeight: true
                            cpu:     controller ? controller.cpu        : null
                            flash:   window.cpuFlash
                            history: controller ? controller.cpuHistory : []
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: Theme.gap

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: Theme.gap
                                QueueView {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    label: "high queue"
                                    qColor: Theme.qHigh
                                    items: controller ? controller.highQueue : []
                                }
                                QueueView {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    label: "low queue"
                                    qColor: Theme.qLow
                                    items: controller ? controller.lowQueue : []
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: Theme.gap
                                QueueView {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    label: "i/o disk"; kind: "io"; qColor: Theme.qDisk
                                    items: controller ? controller.diskQueue : []
                                }
                                QueueView {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    label: "i/o tape"; kind: "io"; qColor: Theme.qTape
                                    items: controller ? controller.tapeQueue : []
                                }
                                QueueView {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    label: "i/o printer"; kind: "io"; qColor: Theme.qPrint
                                    items: controller ? controller.printerQueue : []
                                }
                            }
                        }
                    }

                    // Row 2: Gantt
                    TimelineChart {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 260
                        history:           controller ? controller.ganttHistory      : []
                        totalProcessCount: controller ? controller.totalProcessCount : 0
                        finishedCount:     controller ? controller.finished.length   : 0
                    }

                    // Row 3: Process table + Stats
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 240
                        spacing: Theme.gap

                        FinishedTable {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            processes:   controller ? controller.allProcesses : []
                            selectedPid: window.selectedPid
                            onProcessSelected: (pid) => { window.selectedPid = pid }
                        }

                        StatsBar {
                            Layout.preferredWidth: 320
                            Layout.fillHeight: true
                            stats:             controller ? controller.stats             : ({})
                            utilHistory:       controller ? controller.utilHistory       : []
                            turnaroundHistory: controller ? controller.turnaroundHistory : []
                            throughputHistory: controller ? controller.throughputHistory : []
                        }
                    }
                }

                // Inspector panel (toggleable)
                InspectorPanel {
                    Layout.preferredWidth: 320
                    Layout.fillHeight: true
                    visible: window.inspectorOpen
                    rawSnapshot: controller ? controller.rawSnapshot : ""
                    events:      controller ? controller.events      : []
                }
            }
        }
    }

    LaunchOverlay {
        id: launchOverlay
        visible:   controller ? controller.needsLaunch : true
        launching: controller ? controller.launching   : false
        onLaunchRequested: (params) => { if (controller) controller.launch(params) }
    }

    Connections {
        target: controller
        ignoreUnknownSignals: true
        function onLaunchStateChanged() {
            if (!controller || !controller.needsLaunch) return
            var p = controller.lastParams
            if (!p || Object.keys(p).length === 0) return
            launchOverlay.processes   = p.processes   ?? launchOverlay.processes
            launchOverlay.quantumHi   = p.quantumHi   ?? launchOverlay.quantumHi
            launchOverlay.quantumLo   = p.quantumLo   ?? launchOverlay.quantumLo
            launchOverlay.pIo         = p.pIo         ?? launchOverlay.pIo
            launchOverlay.serviceMin  = p.serviceMin  ?? launchOverlay.serviceMin
            launchOverlay.serviceMax  = p.serviceMax  ?? launchOverlay.serviceMax
            launchOverlay.seed        = p.seed        ?? launchOverlay.seed
        }
    }

    Toast { id: toast }

    Connections {
        target: controller
        ignoreUnknownSignals: true
        function onErrorOccurred(msg) { toast.show(msg, Theme.danger) }
        function onInfoOccurred(msg)  { toast.show(msg, Theme.accentAlt) }
    }

    Shortcut {
        sequence: "Esc"
        onActivated: Qt.quit()
    }
}
