import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts
import QtQuick.Controls.Basic

/* ProcessDetail — side panel with the full history of a selected process.
   Ported from scheduler-ui; unlike the old version (heuristic state diffs)
   the strip and event list are derived exactly from the per-tick events[]
   arrays exposed by SimController.eventsHistory. */
GlassCard {
    id: root

    /* ── Inputs ─────────────────────────────────────────────────────── */
    property int selectedPid: -1
    property var process: null       // allProcesses entry for the pid
    property var eventsHistory: []   // events[] per recorded tick (index = tick-1)

    signal closeRequested()

    readonly property bool hasPid: selectedPid >= 0 && process !== null
    readonly property color pidColor: hasPid ? Theme.pidColor(selectedPid) : Theme.textDim

    /* ── Status chip ────────────────────────────────────────────────── */
    function statusLabel(s) {
        return ({ "cpu_high": "cpu high", "cpu_low": "cpu low",
                  "queue_high": "high queue", "queue_low": "low queue",
                  "io_disk": "i/o disk", "io_tape": "i/o tape",
                  "io_printer": "i/o printer", "done": "done" })[s] || s
    }
    function statusColor(s) {
        if (s === "done")       return Theme.accentAlt
        if (s === "cpu_high")   return Theme.qHigh
        if (s === "cpu_low")    return Theme.qLow
        if (s === "queue_high") return Qt.rgba(Theme.qHigh.r, Theme.qHigh.g, Theme.qHigh.b, 0.55)
        if (s === "queue_low")  return Qt.rgba(Theme.qLow.r,  Theme.qLow.g,  Theme.qLow.b,  0.55)
        if (s === "io_disk")    return Theme.qDisk
        if (s === "io_tape")    return Theme.qTape
        if (s === "io_printer") return Theme.qPrint
        return Theme.warning
    }

    /* ── stripData ──────────────────────────────────────────────────── */
    // Per-tick state of the selected process, reconstructed from events:
    //   0 = not arrived / finished (transparent)
    //   1 = on CPU from high queue      2 = on CPU from low queue
    //   3 = waiting in high queue       4 = waiting in low queue
    //   5 = i/o disk    6 = i/o tape    7 = i/o printer
    // A "preempted"/"completed" tick keeps the CPU state (the process ran);
    // an "io_start" tick shows the device (Model A: no CPU tick consumed).
    readonly property var stripData: {
        if (!hasPid) return []
        var pid   = selectedPid
        var arr   = []
        var carry = 0
        for (var i = 0; i < eventsHistory.length; i++) {
            var evts = eventsHistory[i] || []
            var st   = carry
            for (var j = 0; j < evts.length; j++) {
                var e = evts[j]
                if (e.pid !== pid || e.type === "io_tick") continue
                if (e.type === "arrived") {
                    st = 3; carry = 3
                } else if (e.type === "scheduled") {
                    st = (e.queue === "high") ? 1 : 2; carry = st
                } else if (e.type === "preempted") {
                    st = carry; carry = 4
                } else if (e.type === "io_start") {
                    st = (e.device === "disk") ? 5 : (e.device === "tape") ? 6 : 7
                    carry = st
                } else if (e.type === "io_return") {
                    st = (e.queue === "high") ? 3 : 4; carry = st
                } else if (e.type === "completed") {
                    st = carry; carry = 0
                }
            }
            arr.push(st)
        }
        return arr
    }

    /* ── pidEvents ──────────────────────────────────────────────────── */
    // Flat list of this process's events with their tick numbers (io_tick
    // noise excluded).
    readonly property var pidEvents: {
        if (!hasPid) return []
        var pid = selectedPid
        var out = []
        for (var i = 0; i < eventsHistory.length; i++) {
            var evts = eventsHistory[i] || []
            for (var j = 0; j < evts.length; j++) {
                var e = evts[j]
                if (e.pid !== pid || e.type === "io_tick") continue
                out.push({ tick: i + 1, type: e.type,
                           queue: e.queue, device: e.device,
                           io_remaining: e.io_remaining,
                           quantum_used: e.quantum_used,
                           quantum_max: e.quantum_max })
            }
        }
        return out
    }

    function eventLabel(e) {
        if (e.type === "arrived")   return "arrived → high queue"
        if (e.type === "scheduled") return "scheduled ← " + e.queue + " queue"
        if (e.type === "preempted")
            return "preempted " + e.quantum_used + "/" + e.quantum_max + " → low queue"
        if (e.type === "io_start")  return "→ i/o " + e.device + " (" + e.io_remaining + "t)"
        if (e.type === "io_return") return "← i/o " + e.device + " → " + e.queue + " queue"
        if (e.type === "completed") return "completed ✓"
        return e.type
    }
    function eventColor(e) {
        if (e.type === "completed") return Theme.accentAlt
        if (e.type === "preempted") return Theme.warning
        if (e.type === "io_start" || e.type === "io_return") {
            if (e.device === "disk") return Theme.qDisk
            if (e.device === "tape") return Theme.qTape
            return Theme.qPrint
        }
        if (e.type === "scheduled") return Theme.text
        return Theme.textDim
    }

    /* ── Strip colors ───────────────────────────────────────────────── */
    function stripColor(state) {
        if (state === 1) return Theme.qHigh
        if (state === 2) return Theme.qLow
        if (state === 3) return Qt.rgba(Theme.qHigh.r, Theme.qHigh.g, Theme.qHigh.b, 0.35)
        if (state === 4) return Qt.rgba(Theme.qLow.r,  Theme.qLow.g,  Theme.qLow.b,  0.35)
        if (state === 5) return Theme.qDisk
        if (state === 6) return Theme.qTape
        if (state === 7) return Theme.qPrint
        return "transparent"
    }
    function stripTip(state, tick) {
        var labels = ["", "cpu (high)", "cpu (low)", "high queue", "low queue",
                      "i/o disk", "i/o tape", "i/o printer"]
        return "tick " + tick + " — " + (labels[state] || "")
    }

    /* ── Layout ─────────────────────────────────────────────────────── */
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle { width: 10; height: 10; radius: 3; color: root.pidColor }
            Text {
                text: root.hasPid ? "P" + root.selectedPid : "—"
                color: root.pidColor
                font.family: Theme.fontFamily; font.pixelSize: 16; font.weight: Font.Bold
            }
            Rectangle {
                visible: root.hasPid
                height: 18
                width: statusTxt.implicitWidth + 12
                radius: 4
                color: {
                    var c = root.statusColor(root.process ? root.process.status : "")
                    return Qt.rgba(c.r, c.g, c.b, 0.2)
                }
                Text {
                    id: statusTxt
                    anchors.centerIn: parent
                    text: root.process ? root.statusLabel(root.process.status) : ""
                    color: root.statusColor(root.process ? root.process.status : "")
                    font.family: Theme.fontFamily; font.pixelSize: 10; font.weight: Font.Medium
                }
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                width: 20; height: 20; radius: 4
                color: closeHov.containsMouse ? Theme.hover : "transparent"
                Text { anchors.centerIn: parent; text: "✕"; color: Theme.textDim; font.pixelSize: 11 }
                HoverHandler { id: closeHov }
                TapHandler { onTapped: root.closeRequested() }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

        /* ── Execution strip ────────────────────────────────────────── */
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: "EXECUTION"
                color: Theme.textDim
                font.family: Theme.fontFamily; font.pixelSize: 10
                font.weight: Font.Bold; font.letterSpacing: 1.4
            }

            Flickable {
                id: stripFlick
                Layout.fillWidth: true
                height: 20
                clip: true
                contentWidth: Math.max(stripRow.implicitWidth, width)
                boundsBehavior: Flickable.StopAtBounds

                Row {
                    id: stripRow
                    spacing: 1
                    readonly property int blockW: Math.max(4,
                        Math.floor((stripFlick.width - (root.stripData.length - 1))
                                   / Math.max(1, root.stripData.length)))

                    Repeater {
                        model: root.stripData
                        delegate: Rectangle {
                            width:  stripRow.blockW
                            height: 20
                            radius: 2
                            color:   modelData === 0 ? "transparent" : root.stripColor(modelData)
                            opacity: modelData === 0 ? 0
                                   : (modelData >= 3 && modelData <= 4) ? 0.55
                                   : 0.88
                            HoverHandler { id: blkHov }
                            ToolTip.visible: blkHov.hovered && modelData > 0
                            ToolTip.delay:   150
                            ToolTip.text:    root.stripTip(modelData, index + 1)
                        }
                    }
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8
                Repeater {
                    model: [
                        { color: Theme.qHigh,  label: "cpu high" },
                        { color: Theme.qLow,   label: "cpu low"  },
                        { color: Qt.rgba(Theme.qHigh.r, Theme.qHigh.g, Theme.qHigh.b, 0.35), label: "queued" },
                        { color: Theme.qDisk,  label: "disk"     },
                        { color: Theme.qTape,  label: "tape"     },
                        { color: Theme.qPrint, label: "printer"  }
                    ]
                    delegate: Row {
                        spacing: 3
                        Rectangle {
                            width: 8; height: 8; radius: 2
                            color: modelData.color
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: modelData.label
                            color: Theme.textDim
                            font.family: Theme.fontFamily; font.pixelSize: 9
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

        /* ── Event log ──────────────────────────────────────────────── */
        Text {
            text: "EVENTS"
            color: Theme.textDim
            font.family: Theme.fontFamily; font.pixelSize: 10
            font.weight: Font.Bold; font.letterSpacing: 1.4
        }

        ListView {
            id: eventList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.pidEvents
            spacing: 2
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Row {
                width: eventList.width
                height: 20
                spacing: 6

                Text {
                    width: 36
                    text: "t" + modelData.tick
                    color: Theme.textDim
                    font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.Bold
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                }

                Text {
                    text: root.eventLabel(modelData)
                    color: root.eventColor(modelData)
                    font.family: Theme.fontFamily; font.pixelSize: 11
                    font.weight: modelData.type === "completed" ? Font.Bold : Font.Normal
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Text {
                visible: root.pidEvents.length === 0 && root.hasPid
                anchors.centerIn: parent
                text: "no events recorded"
                color: Theme.textDim
                font.family: Theme.fontFamily; font.pixelSize: 11
                opacity: 0.5
            }
        }
    }
}
