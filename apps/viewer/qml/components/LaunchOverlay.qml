import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent
    z: 100

    property bool launching: false

    property int processes:   5
    property int quantumHi:   3
    property int quantumLo:   6
    property int pIo:         0
    property int serviceMin:  5
    property int serviceMax:  15
    property int seed:        42

    signal launchRequested(var params)

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.78)
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: Theme.accentSoft }
                GradientStop { position: 0.5; color: "transparent" }
                GradientStop { position: 1.0; color: "transparent" }
            }
            opacity: 0.8
        }
        MouseArea { anchors.fill: parent }
    }

    Rectangle {
        id: card
        width: 460
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.cardBgSolid
        border.width: 1
        border.color: Theme.cardBorderHi
        radius: Theme.radiusLarge
        height: inner.implicitHeight + 56
        clip: true

        transform: Scale {
            id: scaleT
            origin.x: card.width / 2
            origin.y: card.height / 2
            xScale: 1.0; yScale: 1.0
        }
        opacity: 1.0
        Component.onCompleted: {
            scaleT.xScale = 0.985; scaleT.yScale = 0.985; opacity = 0
            entranceAnim.start()
        }
        ParallelAnimation {
            id: entranceAnim
            NumberAnimation { target: scaleT; property: "xScale"; from: 0.985; to: 1.0; duration: 280; easing.type: Easing.OutCubic }
            NumberAnimation { target: scaleT; property: "yScale"; from: 0.985; to: 1.0; duration: 280; easing.type: Easing.OutCubic }
            NumberAnimation { target: card;   property: "opacity"; from: 0;     to: 1;   duration: 220 }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: parent.height * 0.30
            radius: parent.radius
            opacity: 1.0
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: Theme.accentSoft }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        ColumnLayout {
            id: inner
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 28
            spacing: 0

            RowLayout {
                spacing: 14
                Logo {}
                ColumnLayout {
                    spacing: 4
                    Text {
                        text: "LAUNCH SIMULATOR"
                        color: Theme.accent
                        font.family: Theme.fontFamily
                        font.pixelSize: 14
                        font.weight: Font.Bold
                        font.letterSpacing: 2.8
                    }
                    Text {
                        text: "ROUND ROBIN · FEEDBACK"
                        color: Theme.textDim
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.letterSpacing: 1.6
                    }
                }
            }

            Text {
                text: "No simulator found on port 9000. Configure the parameters\nbelow and launch rr-feedback."
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 11
                wrapMode: Text.Wrap
                lineHeight: 1.5
                Layout.fillWidth: true
                Layout.topMargin: 14
                Layout.bottomMargin: 18
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 18
                Layout.bottomMargin: 18
                spacing: 12

                Repeater {
                    model: [
                        { key: "processes",  label: "Processes",   hint: "total process count",              min: 1,  max: 20  },
                        { key: "quantumHi",  label: "Quantum hi",  hint: "time slices in high-priority queue",min: 1, max: 20  },
                        { key: "quantumLo",  label: "Quantum lo",  hint: "time slices in low-priority queue", min: 1, max: 20  },
                        { key: "pIo",        label: "I/O prob %",  hint: "probability of I/O request per tick",min: 0,max: 100 },
                        { key: "serviceMin", label: "Service min", hint: "minimum CPU burst duration",        min: 1,  max: 50  },
                        { key: "serviceMax", label: "Service max", hint: "maximum CPU burst duration",        min: 2,  max: 100 },
                        { key: "seed",       label: "Seed",        hint: "RNG seed (0 = random)",            min: 0,  max: 99999 }
                    ]
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: 14
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: modelData.label
                                color: Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                                font.weight: Font.Medium
                            }
                            Text {
                                text: modelData.hint
                                color: Theme.textDim
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                            }
                        }
                        SpinBox {
                            boxWidth: 120
                            minimumValue: modelData.min
                            maximumValue: modelData.max
                            value: root[modelData.key]
                            onValueChanged: root[modelData.key] = value
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

            Item {
                Layout.fillWidth: true
                Layout.topMargin: 18
                Layout.preferredHeight: 46

                Rectangle {
                    id: btnBg
                    anchors.fill: parent
                    radius: 10
                    color: root.launching
                            ? Theme.hoverStrong
                            : (launchMouse.containsMouse
                                ? Theme.accent
                                : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.10))
                    border.width: 1
                    border.color: root.launching ? "transparent" : Theme.accentGlow
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                }

                Row {
                    anchors.centerIn: parent
                    spacing: 10
                    Text {
                        visible: !root.launching
                        text: "▶"
                        color: launchMouse.containsMouse ? "#000" : Theme.accent
                        font.family: Theme.fontFamily; font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: root.launching ? "LAUNCHING…" : "LAUNCH SIMULATOR"
                        color: root.launching
                                ? Theme.textDim
                                : (launchMouse.containsMouse ? "#000" : Theme.accent)
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        font.weight: Font.Bold
                        font.letterSpacing: 2.0
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: launchMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: root.launching ? Qt.WaitCursor : Qt.PointingHandCursor
                    onClicked: {
                        if (!root.launching) {
                            root.launchRequested({
                                processes:  root.processes,
                                quantumHi:  root.quantumHi,
                                quantumLo:  root.quantumLo,
                                pIo:        root.pIo,
                                serviceMin: root.serviceMin,
                                serviceMax: root.serviceMax,
                                seed:       root.seed
                            })
                        }
                    }
                }
            }

            Text {
                text: "Binary must be built:  cmake --build build"
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.italic: true
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                Layout.topMargin: 14
            }
        }
    }
}
