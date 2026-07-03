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

    // Process entry in allProcesses for the selected pid (ProcessDetail input).
    readonly property var selectedProcess: {
        if (selectedPid < 0 || !controller) return null
        var list = controller.allProcesses
        for (var i = 0; i < list.length; i++)
            if (list[i].pid === selectedPid) return list[i]
        return null
    }

    // PIDs inserted into each queue this tick, derived from the event log.
    // A process leaving the CPU (preempted or departing for I/O) stays on the
    // CPU display during that tick and only lands in its destination queue on
    // the next tick, so those highlights come from the PREVIOUS tick's events.
    readonly property var newlyQueued: {
        var r = { high: [], low: [], disk: [], tape: [], printer: [] }
        if (!controller) return r
        var evts = controller.events
        for (var i = 0; i < evts.length; i++) {
            var e = evts[i]
            if (e.type === "arrived")   r.high.push(e.pid)
            if (e.type === "io_return") {
                if (e.queue === "low") r.low.push(e.pid)
                else                  r.high.push(e.pid)
            }
        }
        var prev = controller.prevEvents
        for (var j = 0; j < prev.length; j++) {
            if (prev[j].type === "preempted") r.low.push(prev[j].pid)
            if (prev[j].type === "io_start") {
                if      (prev[j].device === "disk")    r.disk.push(prev[j].pid)
                else if (prev[j].device === "tape")    r.tape.push(prev[j].pid)
                else if (prev[j].device === "printer") r.printer.push(prev[j].pid)
            }
        }
        return r
    }

    // "" | "preempted" | "completed" | "io_disk" | "io_tape" | "io_printer" —
    // whether the process shown on the CPU left it this tick (the snapshot
    // keeps it visible so the viewer can tag where it went).
    readonly property string cpuGhost: {
        if (!controller || !controller.cpu || controller.cpu.pid === undefined) return ""
        var g = controller.cpu.ghost
        if (g === "io_start") return "io_" + controller.cpu.device
        return g || ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Header {
            id: header
            Layout.fillWidth: true
            tick:      controller ? controller.tick      : 0
            status:    controller && controller.done      ? "done"
                     : controller && controller.connected ? "connected"
                                                          : "disconnected"
            connected: controller ? controller.connected : false
            simDone:   controller ? controller.done      : false
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
                            ghost:   window.cpuGhost
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
                                    highlightPids: window.newlyQueued.high
                                }
                                QueueView {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    label: "low queue"
                                    qColor: Theme.qLow
                                    items: controller ? controller.lowQueue : []
                                    highlightPids: window.newlyQueued.low
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
                                    highlightPids: window.newlyQueued.disk
                                }
                                QueueView {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    label: "i/o tape"; kind: "io"; qColor: Theme.qTape
                                    items: controller ? controller.tapeQueue : []
                                    highlightPids: window.newlyQueued.tape
                                }
                                QueueView {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                    label: "i/o printer"; kind: "io"; qColor: Theme.qPrint
                                    items: controller ? controller.printerQueue : []
                                    highlightPids: window.newlyQueued.printer
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

                        ProcessDetail {
                            Layout.preferredWidth: 320
                            Layout.fillHeight: true
                            visible: window.selectedPid >= 0
                            selectedPid:   window.selectedPid
                            process:       window.selectedProcess
                            eventsHistory: controller ? controller.eventsHistory : []
                            onCloseRequested: window.selectedPid = -1
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
                    params:      controller ? controller.lastParams  : null
                    bridge:      (typeof scenarioBridge !== "undefined") ? scenarioBridge : null
                }
            }
        }
    }

    LaunchOverlay {
        id: launchOverlay
        visible:   (controller ? controller.needsLaunch : true) && !scenarioEditor.visible
        launching: controller ? controller.launching   : false
        bridge:    (typeof scenarioBridge !== "undefined") ? scenarioBridge : null
        onLaunchRequested: (params) => { if (controller) controller.launch(params) }
        onEditRequested: (path) => {
            scenarioEditor.openWith(path)
            scenarioEditor.visible = true
        }
    }

    ScenarioEditor {
        id: scenarioEditor
        visible: false
        bridge: (typeof scenarioBridge !== "undefined") ? scenarioBridge : null
        onClosed: visible = false
        onUseScenario: (path) => {
            launchOverlay.setScenario(path)
            visible = false
        }
    }

    Connections {
        target: controller
        ignoreUnknownSignals: true
        function onLaunchStateChanged() {
            if (!controller || !controller.needsLaunch) return
            var p = controller.lastParams
            if (!p || Object.keys(p).length === 0) return
            launchOverlay.applyParams(p)
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
