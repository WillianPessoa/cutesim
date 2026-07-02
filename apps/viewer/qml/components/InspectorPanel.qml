import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts
import QtQuick.Controls.Basic

GlassCard {
    id: root

    property string rawSnapshot: ""
    property var events: []

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "INSPECTOR"
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.4
            }
            Item { Layout.fillWidth: true }
            Text {
                text: root.events.length + " event" + (root.events.length !== 1 ? "s" : "")
                color: root.events.length > 0 ? Theme.accent : Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Medium
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

        // Events this tick
        Column {
            Layout.fillWidth: true
            spacing: 4
            visible: root.events.length > 0

            Repeater {
                model: root.events
                delegate: RowLayout {
                    width: parent.width
                    spacing: 8

                    Rectangle {
                        width: 6; height: 6; radius: 1
                        color: {
                            var t = modelData.type
                            if (t === "arrived")   return Theme.accentAlt
                            if (t === "scheduled") return Theme.qHigh
                            if (t === "preempted") return Theme.warning
                            if (t === "io_start")  return Theme.qDisk
                            if (t === "io_return") return Theme.qTape
                            if (t === "completed") return Theme.accentAlt
                            return Theme.textDim
                        }
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        text: modelData.type
                        color: Theme.accent
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.6
                    }

                    Text {
                        text: "P" + modelData.pid
                        color: Theme.pidColor(modelData.pid)
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.weight: Font.Bold
                    }

                    Text {
                        text: {
                            var d = modelData
                            if (d.type === "preempted")
                                return "quantum " + d.quantum_used + "/" + d.quantum_max
                            if (d.type === "io_start" || d.type === "io_return")
                                return d.device || ""
                            if (d.type === "scheduled" && d.queue)
                                return "→ " + d.queue
                            return ""
                        }
                        color: Theme.textDim
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        Layout.fillWidth: true
                    }
                }
            }
        }

        Text {
            visible: root.events.length === 0
            text: "no events this tick"
            color: Theme.textDim
            font.family: Theme.fontFamily
            font.pixelSize: 11
            opacity: 0.6
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

        // Raw JSON snapshot
        Text {
            text: "RAW SNAPSHOT"
            color: Theme.textDim
            font.family: Theme.fontFamily
            font.pixelSize: 10
            font.weight: Font.Bold
            font.letterSpacing: 1.4
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            Text {
                width: parent.width
                text: root.rawSnapshot || "(none)"
                color: Theme.textDim
                font.family: "Menlo, Monaco, Courier New, monospace"
                font.pixelSize: 9
                wrapMode: Text.WrapAnywhere
                lineHeight: 1.4
            }
        }
    }
}
