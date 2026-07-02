import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts
import QtQuick.Controls.Basic

GlassCard {
    id: root
    implicitWidth: 340
    implicitHeight: 188

    // cpu: { pid, remaining, quantum_used, quantum_max, queue } or empty/null when idle
    property var cpu: null
    property bool isIdle: !cpu || !cpu.pid

    // flash: { pid, used, max } — final state of preempted process this tick
    property var flash: null

    // Last N CPU pids (0 = idle), used by the history strip
    property var history: []

    readonly property bool fromHigh: {
        if (!isIdle)            return cpu.queue   === "high"
        if (flash && flash.pid) return true          // preempted from high or low; treat as last queue
        return false
    }
    readonly property color queueColor: fromHigh ? Theme.qHigh : Theme.qLow
    accentStripe: isIdle ? "transparent" : queueColor

    readonly property int displayQuantumUsed: {
        if (!isIdle) return cpu.quantum_used
        if (flash && flash.pid) return flash.used
        return 0
    }
    readonly property int displayQuantumMax: {
        if (!isIdle) return cpu.quantum_max
        if (flash && flash.pid) return flash.max
        return 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        anchors.topMargin: 18
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "CPU"
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.6
            }
            Item { Layout.fillWidth: true }
            Text {
                visible: !root.isIdle
                text: root.fromHigh ? "HIGH QUEUE" : "LOW QUEUE"
                color: root.queueColor
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Medium
                font.letterSpacing: 1.8
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                visible: root.isIdle
                text: "— idle —"
                color: Theme.idle
                font.family: Theme.fontFamily
                font.pixelSize: 30
                font.weight: Font.Bold
                font.letterSpacing: -0.6
            }
            Text {
                visible: !root.isIdle
                text: root.isIdle ? "" : ("P" + root.cpu.pid)
                color: Theme.textStrong
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeHero
                font.weight: Font.Bold
                font.letterSpacing: -1.6
            }
            Item { Layout.fillWidth: true }
            ColumnLayout {
                visible: !root.isIdle
                spacing: 2
                Text {
                    text: "remaining"
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 0.6
                }
                Text {
                    text: root.isIdle ? "" : (root.cpu.remaining + " ticks")
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.Bold
                }
            }
        }

        ColumnLayout {
            visible: !root.isIdle || (root.flash !== null && root.flash !== undefined && root.flash.pid > 0)
            Layout.fillWidth: true
            spacing: 6
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "QUANTUM"
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    font.letterSpacing: 1.6
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: root.displayQuantumUsed + " / " + root.displayQuantumMax
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.Bold
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 6
                radius: 3
                color: Theme.hoverStrong
                clip: true

                Rectangle {
                    height: parent.height
                    width: parent.width
                           * Math.max(0, root.displayQuantumUsed - 1)
                           / Math.max(1, root.displayQuantumMax)
                    color: root.queueColor
                    opacity: 0.6
                    Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                }

                Rectangle {
                    x: parent.width
                       * Math.max(0, root.displayQuantumUsed - 1)
                       / Math.max(1, root.displayQuantumMax)
                    height: parent.height
                    width: parent.width / Math.max(1, root.displayQuantumMax)
                    color: root.queueColor
                    Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                }
                Repeater {
                    model: Math.max(0, root.displayQuantumMax - 1)
                    delegate: Rectangle {
                        x: (index + 1) * (parent.width / root.displayQuantumMax) - 0.5
                        y: 0; width: 1; height: parent.height
                        color: Qt.rgba(0, 0, 0, 0.35)
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                text: "LAST 24 TICKS"
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Medium
                font.letterSpacing: 1.6
            }
            Row {
                id: stripRow
                Layout.fillWidth: true
                spacing: 2

                readonly property var slice24: root.history.slice(-24)

                Repeater {
                    model: stripRow.slice24
                    delegate: Rectangle {
                        width: (stripRow.width - 23 * 2) / 24
                        height: 14
                        radius: 2

                        property int pid: modelData

                        color:   pid === 0 ? Theme.idle : Theme.pidColor(pid)
                        opacity: pid === 0 ? 0.5 : 0.85

                        HoverHandler { id: stripHov }
                        ToolTip.visible: stripHov.hovered
                        ToolTip.delay:   200
                        ToolTip.text: {
                            var t = root.history.length - stripRow.slice24.length + index
                            return pid === 0
                                ? "tick " + t + " — idle"
                                : "tick " + t + " — P" + pid
                        }
                    }
                }
            }
        }
    }
}
