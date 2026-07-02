import QtQuick
import CuteSim.Viewer

Item {
    id: root
    property color chipColor: Theme.accent
    property int pid: 0
    property string meta: ""

    implicitWidth: row.implicitWidth + 14
    implicitHeight: 22

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusSmall
        color: Qt.rgba(root.chipColor.r, root.chipColor.g, root.chipColor.b, 0.14)
        border.width: 1
        border.color: root.chipColor
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
