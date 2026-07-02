import QtQuick
import CuteSim.Viewer

Item {
    id: root
    anchors.fill: parent
    z: 150
    visible: opacity > 0
    opacity: 0

    property string message: ""
    property color toastColor: Theme.danger

    function show(msg, color) {
        message = msg
        if (color !== undefined) toastColor = color
        opacity = 1
        hideTimer.restart()
    }

    Timer {
        id: hideTimer
        interval: 4000
        onTriggered: root.opacity = 0
    }

    Behavior on opacity { NumberAnimation { duration: 240 } }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        radius: 10
        color: root.toastColor
        opacity: 0.92
        implicitWidth: txt.implicitWidth + 32
        implicitHeight: 38

        Text {
            id: txt
            anchors.centerIn: parent
            text: root.message
            color: {
                var c = root.toastColor
                var lum = 0.299 * c.r + 0.587 * c.g + 0.114 * c.b
                return lum > 0.55 ? "#0d0f1a" : "#ffffff"
            }
            font.family: Theme.fontFamily
            font.pixelSize: 12
            font.weight: Font.Medium
        }
    }
}
