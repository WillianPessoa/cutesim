import QtQuick
import CuteSim.Viewer

Item {
    id: root
    property int tick: 0
    implicitWidth: contentRow.implicitWidth + 24
    implicitHeight: 30

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: Theme.cardBg
        border.width: 1
        border.color: Theme.cardBorder
    }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 6

        Text {
            text: "TICK"
            color: Theme.textDim
            font.family: Theme.fontFamily
            font.pixelSize: 10
            font.weight: Font.Medium
            font.letterSpacing: 1.6
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: num
            objectName: "num"
            text: String(root.tick).padStart ? String(root.tick).padStart(3, "0") : ("000" + root.tick).slice(
                                                   -3)
            color: Theme.textStrong
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMed
            font.weight: Font.Bold
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    SequentialAnimation {
        id: popAnim
        NumberAnimation {
            target: num
            property: "scale"
            from: 1.0
            to: 1.08
            duration: 100
        }
        NumberAnimation {
            target: num
            property: "scale"
            from: 1.08
            to: 1.0
            duration: 140
        }
    }
    Connections {
        target: root
        function onTickChanged() {
            popAnim.restart();
        }
    }
}
