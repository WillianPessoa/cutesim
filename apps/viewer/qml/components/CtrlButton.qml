import QtQuick
import CuteSim.Viewer

Item {
    id: root
    implicitWidth: contentRow.implicitWidth + 24
    implicitHeight: 32

    property string label: ""
    property string iconText: ""
    property bool enabledState: true
    property bool primary: false
    property bool iconOnly: false
    signal clicked()

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 8
        antialiasing: true
        color: !root.enabledState
                ? Theme.hover
                : (mouseArea.containsMouse
                    ? Theme.accent
                    : (root.primary ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                                    : Theme.cardBg))
        border.width: 1
        border.color: !root.enabledState
                       ? "transparent"
                       : (mouseArea.containsMouse
                           ? Theme.accent
                           : (root.primary ? Theme.accentGlow : Theme.cardBorder))
        Behavior on color        { ColorAnimation { duration: Theme.durFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 6

        Text {
            text: root.iconText
            visible: root.iconText.length > 0
            color: !root.enabledState
                    ? Theme.textDim
                    : (mouseArea.containsMouse ? "#000" : (root.primary ? Theme.accent : Theme.text))
            font.family: Theme.fontFamily
            font.pixelSize: 13
            font.weight: Font.Medium
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.label
            visible: root.label.length > 0 && !root.iconOnly
            color: !root.enabledState
                    ? Theme.textDim
                    : (mouseArea.containsMouse ? "#000" : (root.primary ? Theme.accent : Theme.text))
            font.family: Theme.fontFamily
            font.pixelSize: 12
            font.weight: Font.Medium
            font.letterSpacing: 0.4
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.enabledState ? Qt.PointingHandCursor : Qt.ForbiddenCursor
        onClicked: if (root.enabledState) root.clicked()
    }
}
