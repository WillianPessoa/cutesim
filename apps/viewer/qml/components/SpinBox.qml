import QtQuick
import CuteSim.Viewer

Item {
    id: root
    property int value: 0
    property int minimumValue: 0
    property int maximumValue: 9999
    property bool enabledState: true
    property int boxWidth: 86

    implicitWidth: boxWidth
    implicitHeight: 32
    opacity: enabledState ? 1.0 : 0.4

    onValueChanged: {
        if (!input.activeFocus)
            input.text = root.value.toString();
    }

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: 8
        color: Theme.cardBg
        border.width: 1
        border.color: input.activeFocus ? Theme.accent : Theme.cardBorder
        Behavior on border.color {
            ColorAnimation {
                duration: Theme.durFast
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -3
        radius: bg.radius + 3
        color: "transparent"
        border.width: 3
        border.color: Theme.accentSoft
        visible: input.activeFocus
    }

    Row {
        anchors.fill: parent
        spacing: 0

        Item {
            width: 28
            height: parent.height
            Rectangle {
                anchors.fill: parent
                color: minusArea.containsMouse ? Theme.hoverStrong : "transparent"
                Behavior on color {
                    ColorAnimation {
                        duration: Theme.durFast
                    }
                }
            }
            Text {
                anchors.centerIn: parent
                text: "−"
                color: (root.value <= root.minimumValue) ? Theme.textDim : (minusArea.containsMouse
                                                                            ? Theme.accent :
                                                                              Theme.text)
                font.family: Theme.fontFamily
                font.pixelSize: 14
                font.weight: Font.Bold
            }
            MouseArea {
                id: minusArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: (root.enabledState && root.value > root.minimumValue)
                             ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                onClicked: {
                    if (root.enabledState && root.value > root.minimumValue)
                        root.value = root.value - 1;
                }
            }
        }

        TextInput {
            id: input
            width: parent.width - 56
            height: parent.height
            Component.onCompleted: text = root.value.toString()
            color: Theme.textStrong
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMed
            font.weight: Font.Bold
            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter
            selectByMouse: true
            enabled: root.enabledState
            validator: IntValidator {
                bottom: root.minimumValue
                top: root.maximumValue
            }
            onEditingFinished: {
                var v = parseInt(text);
                if (isNaN(v))
                    v = root.minimumValue;
                v = Math.max(root.minimumValue, Math.min(root.maximumValue, v));
                root.value = v;
                text = v.toString();
            }
        }

        Item {
            width: 28
            height: parent.height
            Rectangle {
                anchors.fill: parent
                color: plusArea.containsMouse ? Theme.hoverStrong : "transparent"
                Behavior on color {
                    ColorAnimation {
                        duration: Theme.durFast
                    }
                }
            }
            Text {
                anchors.centerIn: parent
                text: "+"
                color: (root.value >= root.maximumValue) ? Theme.textDim : (plusArea.containsMouse
                                                                            ? Theme.accent :
                                                                              Theme.text)
                font.family: Theme.fontFamily
                font.pixelSize: 14
                font.weight: Font.Bold
            }
            MouseArea {
                id: plusArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: (root.enabledState && root.value < root.maximumValue)
                             ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                onClicked: {
                    if (root.enabledState && root.value < root.maximumValue)
                        root.value = root.value + 1;
                }
            }
        }
    }
}
