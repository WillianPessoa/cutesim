import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts

Item {
    id: root
    property string label: ""
    property string valueText: "—"
    property string unitText: ""
    property color valueColor: Theme.text
    property bool alert: false
    property var sparkData: []
    property color sparkColor: valueColor

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 2

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: root.label.toUpperCase()
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.4
            }
            Item { Layout.fillWidth: true }
            Text {
                visible: root.alert
                text: "● ALERT"
                color: Theme.danger
                font.family: Theme.fontFamily
                font.pixelSize: 9
                font.weight: Font.Bold
                font.letterSpacing: 1.0
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            RowLayout {
                spacing: 4
                Layout.alignment: Qt.AlignVCenter
                Text {
                    text: root.valueText
                    color: root.valueColor
                    font.family: Theme.fontFamily
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    font.letterSpacing: -0.3
                }
                Text {
                    visible: root.unitText.length > 0
                    text: root.unitText
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                    Layout.maximumWidth: 72
                }
            }

            Item { Layout.fillWidth: true }

            Sparkline {
                visible: root.sparkData.length > 1
                chartData: root.sparkData
                lineColor: root.sparkColor
                Layout.preferredWidth: 70
                Layout.preferredHeight: 20
            }
        }
    }
}
