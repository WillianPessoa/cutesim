import QtQuick
import CuteSim.Viewer

Item {
    id: root
    property color chipColor: Theme.accent
    property int pid: 0
    property string meta: ""
    property bool highlighted: false  // process was inserted into this queue this tick

    implicitWidth: row.implicitWidth + 14
    implicitHeight: 22

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusSmall
        color: Qt.rgba(root.chipColor.r, root.chipColor.g, root.chipColor.b, root.highlighted
                       ? 0.32 : 0.14)
        border.width: root.highlighted ? 2 : 1
        border.color: root.chipColor

        Behavior on color {
            ColorAnimation {
                duration: 300
            }
        }
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 4
        Text {
            text: "P" + root.pid
            color: root.chipColor
            font.family: Theme.fontFamily
            font.pixelSize: 11
            font.weight: Font.Bold
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.meta
            color: Theme.textDim
            font.family: Theme.fontFamily
            font.pixelSize: 11
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
