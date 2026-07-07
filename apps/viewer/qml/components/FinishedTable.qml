import QtQuick
import CuteSim.Viewer
import QtQuick.Controls
import QtQuick.Layouts

GlassCard {
    id: root
    clip: true   // BUG-21: never let column labels escape the card

    property var processes: []
    property int selectedPid: -1

    signal processSelected(int pid)

    /* BUG-21 (round 2): the PID column stays pinned on the left; every other
    column lives in a clipped area that scrolls horizontally, shared by the
    header and all rows (bottom scroll bar + horizontal wheel). */
    readonly property int pidColWidth: 48
    readonly property int colSpacing: 8
    readonly property real scrollWidth: {
        var w = 0;
        for (var i = 0; i < cols.length; i++)
            w += cols[i].width;
        return w + colSpacing * (cols.length - 1);
    }
    // room for the scrollable cells: card minus margins, paddings and pid col
    readonly property real hViewport: Math.max(0, width - 48 - pidColWidth - colSpacing)
    readonly property bool needsHScroll: scrollWidth > hViewport
    readonly property real hx: hbar.position * scrollWidth

    WheelHandler {
        orientation: Qt.Horizontal
        target: null
        onWheel: event => {
                     if (!root.needsHScroll)
                     return;
                     var maxPos = 1 - hbar.size;
                     hbar.position = Math.max(0, Math.min(maxPos, hbar.position - (
                                                              event.angleDelta.x / 2)
                                                          / root.scrollWidth));
                 }
    }

    readonly property int doneCount: {
        var n = 0;
        for (var i = 0; i < processes.length; i++)
            if (processes[i].status === "done")
                n++;
        return n;
    }

    function statusLabel(s) {
        return ({
                    "cpu_high": "CPU high",
                    "cpu_low": "CPU low",
                    "queue_high": "Queue high",
                    "queue_low": "Queue low",
                    "io_disk": "I/O disk",
                    "io_tape": "I/O tape",
                    "io_printer": "I/O printer",
                    "done": "Done"
                })[s] || s;
    }
    function statusColor(s) {
        if (s === "done")
            return Theme.accentAlt;
        if (s === "cpu_high")
            return Theme.qHigh;
        if (s === "cpu_low")
            return Theme.qLow;
        if (s === "queue_high")
            return Qt.rgba(Theme.qHigh.r, Theme.qHigh.g, Theme.qHigh.b, 0.55);
        if (s === "queue_low")
            return Qt.rgba(Theme.qLow.r, Theme.qLow.g, Theme.qLow.b, 0.55);
        return Theme.warning;
    }

    /* Scrollable columns — PID is pinned and lives outside this list */
    readonly property var cols: [
        {
            label: "STATUS",
            width: 100
        },
        {
            label: "ARRIVAL",
            width: 60
        },
        {
            label: "DONE",
            width: 52
        },
        {
            label: "SERVICE",
            width: 60
        },
        {
            label: "CPU",
            width: 52
        },
        {
            label: "DISK",
            width: 44
        },
        {
            label: "TAPE",
            width: 44
        },
        {
            label: "PRINT",
            width: 44
        },
        {
            label: "I/O",
            width: 44
        },
        {
            label: "WAIT",
            width: 56
        },
        {
            label: "TURNAROUND",
            width: 80
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Text {
                text: "PROCESSES"
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.4
            }
            Item {
                Layout.fillWidth: true
            }
            Text {
                text: root.doneCount + " / " + root.processes.length
                color: Theme.accentAlt
                font.family: Theme.fontFamily
                font.pixelSize: 11
                font.weight: Font.Bold
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 16

            Text {
                x: 12
                width: root.pidColWidth
                text: "PID"
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.3
            }

            Item {
                x: 12 + root.pidColWidth + root.colSpacing
                width: Math.max(0, parent.width - x - 12)
                height: parent.height
                clip: true

                Row {
                    x: -root.hx
                    spacing: root.colSpacing
                    Repeater {
                        model: root.cols
                        delegate: Text {
                            width: modelData.width
                            text: modelData.label
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            font.letterSpacing: 1.3
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.hoverStrong
            opacity: 0.5
        }

        ListView {
            id: rows
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.processes
            spacing: 0
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Item {
                width: rows.width
                height: 28

                readonly property var p: modelData
                readonly property bool done: p.status === "done"
                readonly property bool sel: p.pid === root.selectedPid

                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    radius: 6
                    color: sel ? Qt.rgba(Theme.pidColor(p.pid).r, Theme.pidColor(p.pid).g, Theme.pidColor(
                                             p.pid).b, 0.12) : rowHov.hovered ? Theme.hover :
                                                                                "transparent"
                    border.width: sel ? 1 : 0
                    border.color: Qt.rgba(Theme.pidColor(p.pid).r, Theme.pidColor(p.pid).g, Theme.pidColor(
                                              p.pid).b, 0.35)
                }

                TapHandler {
                    onTapped: root.processSelected(sel ? -1 : p.pid)
                }

                /* Pinned PID cell */
                Row {
                    x: 12
                    width: root.pidColWidth
                    spacing: 5
                    anchors.verticalCenter: parent.verticalCenter
                    readonly property color pidCol: Theme.pidColor(p.pid)
                    Rectangle {
                        width: 7
                        height: 7
                        radius: 2
                        color: parent.pidCol
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "P" + p.pid
                        color: parent.pidCol
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                /* Scrollable cells, offset shared with the header */
                Item {
                    x: 12 + root.pidColWidth + root.colSpacing
                    width: Math.max(0, parent.width - x - 12)
                    height: parent.height
                    clip: true

                    Row {
                        x: -root.hx
                        height: parent.height
                        spacing: root.colSpacing

                        Rectangle {
                            width: 100
                            height: 18
                            radius: 4
                            color: Qt.rgba(root.statusColor(p.status).r, root.statusColor(p.status).g,
                                           root.statusColor(p.status).b, 0.18)
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                anchors.centerIn: parent
                                text: root.statusLabel(p.status)
                                color: root.statusColor(p.status)
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                                font.weight: Font.Medium
                                font.letterSpacing: 0.5
                            }
                        }

                        // ARRIVAL
                        Text {
                            width: 60
                            text: "t" + (done ? p.arrival_tick : p.first_seen_tick)
                            color: done ? Theme.textDim : Qt.rgba(Theme.textDim.r, Theme.textDim.g,
                                                                  Theme.textDim.b, 0.5)
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // DONE tick
                        Text {
                            width: 52
                            text: done ? ("t" + p.finish_tick) : "—"
                            color: done ? Theme.accent : Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // SERVICE
                        Text {
                            width: 60
                            text: done ? (p.service_time + "t") : "—"
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // CPU used
                        Text {
                            width: 52
                            text: done ? (p.cpu_time_used + "t") : "—"
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // DISK
                        Text {
                            width: 44
                            text: done ? (p.io_disk + "t") : "—"
                            color: (done && p.io_disk > 0) ? Theme.warning : Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // TAPE
                        Text {
                            width: 44
                            text: done ? (p.io_tape + "t") : "—"
                            color: (done && p.io_tape > 0) ? Theme.warning : Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // PRINTER
                        Text {
                            width: 44
                            text: done ? (p.io_printer + "t") : "—"
                            color: (done && p.io_printer > 0) ? Theme.warning : Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // I/O total
                        Text {
                            width: 44
                            text: done ? (p.io_total + "t") : "—"
                            color: (done && p.io_total > 0) ? Theme.warning : Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            font.weight: done && p.io_total > 0 ? Font.Bold : Font.Normal
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // WAIT
                        Text {
                            width: 56
                            text: done ? (p.wait_time + "t") : "—"
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // TURNAROUND
                        Text {
                            width: 80
                            text: done ? (p.turnaround + "t") : "—"
                            color: done ? Theme.accentAlt : Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            font.weight: done ? Font.Bold : Font.Normal
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                HoverHandler {
                    id: rowHov
                }
            }

            Text {
                visible: root.processes.length === 0
                anchors.centerIn: parent
                text: "no processes yet"
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 11
                opacity: 0.6
            }
        }

        ScrollBar {
            id: hbar
            Layout.fillWidth: true
            Layout.leftMargin: 12 + root.pidColWidth + root.colSpacing
            Layout.rightMargin: 12
            Layout.preferredHeight: root.needsHScroll ? implicitHeight : 0
            visible: root.needsHScroll
            orientation: Qt.Horizontal
            policy: ScrollBar.AlwaysOn
            size: root.scrollWidth > 0 ? Math.min(1, root.hViewport / root.scrollWidth) : 1
            onSizeChanged: position = Math.max(0, Math.min(position, 1 - size))
        }
    }
}
