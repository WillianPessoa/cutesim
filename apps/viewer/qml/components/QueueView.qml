import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts

GlassCard {
    id: root

    property string label: "QUEUE"
    property color qColor: Theme.accent
    property string kind: "cpu"        // "cpu" → {pid, remaining}; "io" → {pid, io_remaining}
    property var items: []
    property var highlightPids: []     // PIDs inserted into this queue this tick

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                width: 6; height: 6; radius: 1
                color: root.qColor
                Layout.alignment: Qt.AlignVCenter
            }
            Text {
                text: root.label.toUpperCase()
                color: root.qColor
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.4
                Layout.alignment: Qt.AlignVCenter
            }
            Item { Layout.fillWidth: true }
            RowLayout {
                spacing: 4
                Text {
                    text: root.items.length
                    color: root.qColor
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    font.weight: Font.Bold
                }
                Text {
                    text: root.items.length === 1 ? "proc" : "procs"
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.letterSpacing: 1.0
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Text {
                visible: root.items.length === 0
                anchors.left: parent.left
                anchors.top: parent.top
                text: "— empty —"
                color: Theme.textDim
                opacity: 0.6
                font.family: Theme.fontFamily
                font.pixelSize: 11
                font.weight: Font.Medium
            }

            Flow {
                anchors.fill: parent
                spacing: 6
                visible: root.items.length > 0
                Repeater {
                    model: root.items
                    delegate: QueueChip {
                        chipColor: root.qColor
                        pid: modelData.pid
                        highlighted: root.highlightPids.indexOf(modelData.pid) >= 0
                        meta: root.kind === "io"
                              ? ("io:" + (modelData.io_remaining !== undefined
                                          ? modelData.io_remaining : 0) + "t")
                              : ((modelData.remaining !== undefined
                                  ? modelData.remaining : 0) + "t")
                    }
                }
            }
        }
    }
}
