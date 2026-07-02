import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts

Rectangle {
    id: root
    color: Theme.headerBg
    height: 64

    property int tick: 0
    property string status: "disconnected"
    property bool connected: false
    property bool simDone: false

    signal stepRequested()
    signal resetRequested()
    signal reconfigureRequested()
    signal themeToggleRequested()
    signal inspectorToggleRequested()

    readonly property bool canRun:      connected && !simDone
    readonly property bool canReconfig: connected || simDone

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.divider
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        spacing: 14

        RowLayout {
            spacing: 10
            Logo {}
            ColumnLayout {
                spacing: 2
                Text {
                    text: "CUTESIM"
                    color: Theme.accent
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    font.letterSpacing: 2.8
                }
                Text {
                    text: "ROUND ROBIN · FEEDBACK"
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 9
                    font.weight: Font.Medium
                    font.letterSpacing: 1.6
                }
            }
        }

        Rectangle {
            width: 1; Layout.preferredHeight: 28
            color: Theme.divider
            Layout.leftMargin: 6
        }

        TickChip   { tick: root.tick }
        StatusChip { status: root.status }

        Item { Layout.fillWidth: true }

        CtrlButton {
            iconText: "⏵"
            label: "Step"
            enabledState: root.canRun
            onClicked: root.stepRequested()
        }

        CtrlButton {
            iconText: "↺"
            iconOnly: true
            enabledState: root.canReconfig
            onClicked: root.resetRequested()
        }

        CtrlButton {
            iconText: "⚙"
            iconOnly: true
            enabledState: root.canReconfig
            onClicked: root.reconfigureRequested()
        }

        Rectangle {
            width: 1; Layout.preferredHeight: 28
            color: Theme.divider
        }

        CtrlButton {
            iconText: "⊞"
            iconOnly: true
            implicitWidth: 36
            enabledState: true
            onClicked: root.inspectorToggleRequested()
        }

        CtrlButton {
            iconText: Theme.isDark ? "☀" : "🌙"
            iconOnly: true
            implicitWidth: 36
            enabledState: true
            onClicked: root.themeToggleRequested()
        }
    }
}
