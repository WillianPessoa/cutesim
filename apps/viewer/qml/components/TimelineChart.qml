import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts
import QtQuick.Controls.Basic

Rectangle {
    id: root

    property var history: []
    property int totalProcessCount: 0
    property int finishedCount: 0
    property int blockWidth: 14
    property int rowHeight: 24
    property int labelWidth: 56
    property int axisHeight: 22

    readonly property int remainingCount: totalProcessCount - finishedCount

    readonly property var pids: {
        var seen = {};
        var out = [];
        for (var i = 0; i < history.length; i++) {
            var p = history[i];
            if (p >= 0 && !seen[p]) {
                seen[p] = true;
                out.push(p);
            }
        }
        out.sort(function (a, b) {
            return a - b;
        });
        return out;
    }

    color: Theme.cardBgSolid
    radius: Theme.radius
    implicitHeight: 240

    ColumnLayout {
        anchors {
            fill: parent
            margins: Theme.padding
        }
        spacing: Theme.gap

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "GANTT — BY PROCESS"
                color: Theme.textDim
                font.pixelSize: Theme.fontSizeSmall
                font.family: Theme.fontFamily
                font.bold: true
                font.letterSpacing: 1.5
            }

            Text {
                text: root.history.length + " ticks · " + root.pids.length + " processes"
                color: Theme.textDim
                font.pixelSize: Theme.fontSizeSmall
                font.family: Theme.fontFamily
            }

            Item {
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 6
                visible: root.totalProcessCount > 0
                Text {
                    text: root.remainingCount > 0 ? root.remainingCount + " remaining" : "all done"
                    color: root.remainingCount > 0 ? Theme.accent : Theme.accentAlt
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: Theme.fontFamily
                    font.weight: Font.Bold
                }
            }
        }

        Item {
            id: chartArea
            Layout.fillWidth: true
            Layout.fillHeight: true

            readonly property int contentW: root.history.length * root.blockWidth
            readonly property int contentH: (Math.max(root.pids.length, 1) + 1) * root.rowHeight

            Flickable {
                id: flick
                x: root.labelWidth
                y: 0
                width: chartArea.width - root.labelWidth
                height: chartArea.height - root.axisHeight
                contentWidth: chartArea.contentW
                contentHeight: chartArea.contentH
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                onContentWidthChanged: contentX = Math.max(0, contentWidth - width)
                onContentHeightChanged: contentY = Math.max(0, contentHeight - height)

                ScrollBar.horizontal: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                Repeater {
                    model: Math.floor(root.history.length / 5) + 1
                    Rectangle {
                        x: index * 5 * root.blockWidth
                        y: 0
                        width: 1
                        height: chartArea.contentH
                        color: Theme.hoverStrong
                        opacity: 0.35
                    }
                }

                Repeater {
                    model: root.history.length
                    Rectangle {
                        x: index * root.blockWidth + 1
                        y: root.pids.length * root.rowHeight + 4
                        width: root.blockWidth - 2
                        height: root.rowHeight - 8
                        radius: 2
                        visible: root.history[index] === -1
                        color: Theme.danger
                        opacity: 0.55
                        HoverHandler {
                            id: idleHov
                        }
                        Tip {
                            visible: idleHov.hovered
                            delay: 300
                            text: "tick " + index + " — idle"
                        }
                    }
                }

                Repeater {
                    model: root.pids
                    Item {
                        id: lane
                        property int pid: modelData
                        x: 0
                        y: (root.pids.length - 1 - index) * root.rowHeight
                        width: chartArea.contentW
                        height: root.rowHeight

                        Rectangle {
                            anchors.fill: parent
                            color: index % 2 === 0 ? "transparent" : Qt.rgba(1, 1, 1, Theme.isDark
                                                                             ? 0.02 : 0.04)
                        }

                        Rectangle {
                            y: root.rowHeight / 2
                            width: parent.width
                            height: 1
                            color: Theme.hoverStrong
                            opacity: 0.3
                        }

                        Repeater {
                            model: root.history.length
                            Rectangle {
                                x: index * root.blockWidth + 1
                                y: 4
                                width: root.blockWidth - 2
                                height: root.rowHeight - 8
                                radius: 2
                                visible: root.history[index] === lane.pid
                                color: Theme.pidColor(lane.pid)
                                HoverHandler {
                                    id: blockHov
                                }
                                Tip {
                                    visible: blockHov.hovered
                                    delay: 300
                                    text: "tick " + index + " — P" + lane.pid
                                }
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: root.pids.length === 0
                    text: "Waiting for first tick…"
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSizeNormal
                    font.family: Theme.fontFamily
                    font.italic: true
                }
            }

            Item {
                id: yPanel
                x: 0
                y: 0
                width: root.labelWidth
                height: chartArea.height - root.axisHeight
                clip: true
                z: 3

                Rectangle {
                    anchors.fill: parent
                    color: root.color
                }

                Item {
                    y: -flick.contentY
                    width: parent.width

                    Item {
                        y: root.pids.length * root.rowHeight
                        height: root.rowHeight
                        width: parent.width
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 4
                            spacing: 6
                            Rectangle {
                                width: 10
                                height: 10
                                radius: 2
                                anchors.verticalCenter: parent.verticalCenter
                                color: Theme.danger
                                opacity: 0.7
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: "IDLE"
                                color: Theme.danger
                                font.pixelSize: Theme.fontSizeSmall
                                font.family: Theme.fontFamily
                                font.bold: true
                            }
                        }
                    }

                    Repeater {
                        model: root.pids
                        Item {
                            y: (root.pids.length - 1 - index) * root.rowHeight
                            height: root.rowHeight
                            width: parent.width
                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 4
                                spacing: 6
                                Rectangle {
                                    width: 10
                                    height: 10
                                    radius: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: Theme.pidColor(modelData)
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "P" + modelData
                                    color: Theme.text
                                    font.pixelSize: Theme.fontSizeNormal
                                    font.family: Theme.fontFamily
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }

            Item {
                id: xPanel
                x: root.labelWidth
                y: chartArea.height - root.axisHeight
                width: chartArea.width - root.labelWidth
                height: root.axisHeight
                clip: true
                z: 3

                Rectangle {
                    anchors.fill: parent
                    color: root.color
                }

                Item {
                    x: -flick.contentX
                    height: parent.height
                    width: root.history.length * root.blockWidth

                    Repeater {
                        model: Math.floor(root.history.length / 5) + 1
                        Item {
                            x: index * 5 * root.blockWidth
                            y: 0
                            width: 40
                            height: parent.height

                            Rectangle {
                                width: 1
                                height: 4
                                color: Theme.textDim
                                opacity: 0.5
                            }
                            Text {
                                anchors.top: parent.top
                                anchors.topMargin: 6
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: (index * 5).toString()
                                color: Theme.textDim
                                font.pixelSize: Theme.fontSizeSmall - 1
                                font.family: Theme.fontFamily
                            }
                        }
                    }
                }
            }

            Rectangle {
                x: 0
                y: chartArea.height - root.axisHeight
                width: root.labelWidth
                height: root.axisHeight
                color: root.color
                z: 4
            }
        }
    }
}
