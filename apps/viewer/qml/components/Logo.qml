import QtQuick
import CuteSim.Viewer

Item {
    id: root
    implicitWidth: 22
    implicitHeight: 22

    Grid {
        anchors.centerIn: parent
        rows: 3; columns: 3
        rowSpacing: 3
        columnSpacing: 3
        Repeater {
            model: 9
            delegate: Rectangle {
                width: 4; height: 4
                radius: 1
                color: Theme.accent
                opacity: (index === 0 || index === 2 || index === 4 ||
                           index === 6 || index === 8) ? 1.0 : 0.35
            }
        }
    }
}
