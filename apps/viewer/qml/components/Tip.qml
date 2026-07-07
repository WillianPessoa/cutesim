import QtQuick
import QtQuick.Controls.Basic
import CuteSim.Viewer

/* Tip — themed tooltip. The ToolTip attached properties render through the
application style's shared instance, which can come up with no background
depending on which style wins; this explicit component always draws the
themed card behind the text. */
ToolTip {
    id: control

    background: Rectangle {
        color: Theme.cardBgSolid
        border.width: 1
        border.color: Theme.cardBorderHi
        radius: 6
    }

    contentItem: Text {
        text: control.text
        color: Theme.text
        font.family: Theme.fontFamily
        font.pixelSize: 10
        wrapMode: Text.Wrap
    }
}
