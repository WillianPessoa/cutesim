import QtQuick
import QtQuick.Window
import CuteSim.Viewer

Window {
    id: root
    visible: true
    width: 1440
    height: 900
    title: "CuteSim"
    color: Theme.bg

    Text {
        anchors.centerIn: parent
        text: "CuteSim"
        color: Theme.accent
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeHero
        font.weight: Font.Light
        font.letterSpacing: 8
    }
}
