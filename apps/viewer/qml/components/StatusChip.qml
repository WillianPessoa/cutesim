import QtQuick
import CuteSim.Viewer

Item {
    id: root
    property string status: "disconnected"  // "connected" | "done" | "disconnected"

    readonly property color statusColor:
        status === "connected" ? Theme.accentAlt :
        status === "done"      ? Theme.textDim :
                                 Theme.danger

    readonly property color statusBg:
        status === "connected" ? Qt.rgba(Theme.accentAlt.r, Theme.accentAlt.g, Theme.accentAlt.b, 0.14) :
        status === "done"      ? Theme.hoverStrong :
                                 Qt.rgba(Theme.danger.r, Theme.danger.g, Theme.danger.b, 0.14)

    readonly property string statusLabel:
        status === "connected" ? "connected" :
        status === "done"      ? "finished" :
                                 "disconnected"

    readonly property bool breathe: status !== "done"

    implicitWidth: contentRow.implicitWidth + 24
    implicitHeight: 26

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: root.statusBg
    }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 8

        Rectangle {
            id: dot
            width: 6; height: 6; radius: 3
            color: root.statusColor
            anchors.verticalCenter: parent.verticalCenter
            SequentialAnimation on opacity {
                running: root.breathe
                loops: Animation.Infinite
                NumberAnimation { from: 1.0; to: 0.5; duration: 1000; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 0.5; to: 1.0; duration: 1000; easing.type: Easing.InOutQuad }
            }
        }

        Text {
            text: root.statusLabel
            color: root.statusColor
            font.family: Theme.fontFamily
            font.pixelSize: 11
            font.weight: Font.Medium
            font.letterSpacing: 0.5
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
