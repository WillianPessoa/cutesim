import QtQuick
import CuteSim.Viewer

/* SegmentControl — compact mutually-exclusive option selector. */
Row {
    id: root

    property var options: []       // list of strings
    property string value: ""
    property int segPixelSize: 10

    signal selected(string value)

    spacing: 4

    Repeater {
        model: root.options
        delegate: Rectangle {
            readonly property bool active: modelData === root.value
            width: segTxt.implicitWidth + 18
            height: 24
            radius: 6
            color: active ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.16) : (
                                segHov.hovered ? Theme.hover : "transparent")
            border.width: 1
            border.color: active ? Theme.accentGlow : Theme.divider

            Text {
                id: segTxt
                anchors.centerIn: parent
                text: modelData
                color: parent.active ? Theme.accent : Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: root.segPixelSize
                font.weight: parent.active ? Font.Bold : Font.Medium
                font.letterSpacing: 1.0
            }

            HoverHandler {
                id: segHov
                cursorShape: Qt.PointingHandCursor
            }
            TapHandler {
                onTapped: {
                    if (root.value !== modelData) {
                        root.value = modelData;
                        root.selected(modelData);
                    }
                }
            }
        }
    }
}
