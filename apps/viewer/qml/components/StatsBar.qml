import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts

GlassCard {
    id: root

    // stats: { cpu_utilization, avg_turnaround, avg_waiting, avg_response, throughput }
    property var stats: ({})

    property var utilHistory: []
    property var turnaroundHistory: []
    property var throughputHistory: []

    function fmtNum(v, digits) {
        if (v === undefined || v === null || isNaN(v))
            return "—";
        return Number(v).toFixed(digits);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 12
            Layout.bottomMargin: 8
            Text {
                text: "STATISTICS"
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.4
            }
            Item {
                Layout.fillWidth: true
            }
            Text {
                text: "live"
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 1.0
            }
        }

        StatItem {
            Layout.fillWidth: true
            Layout.fillHeight: true
            label: "cpu utilization"
            // snapshot sends a 0..1 fraction — display as NN.00 %
            valueText: root.fmtNum(root.stats.cpu_utilization * 100, 2)
            unitText: "%"
            valueColor: Theme.accent
            sparkData: root.utilHistory
            sparkColor: Theme.accent
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.divider
        }

        StatItem {
            Layout.fillWidth: true
            Layout.fillHeight: true
            label: "avg waiting"
            valueText: root.fmtNum(root.stats.avg_waiting, 2)
            unitText: "ticks"
            valueColor: Theme.warning
            sparkData: []
            sparkColor: Theme.warning
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.divider
        }

        StatItem {
            Layout.fillWidth: true
            Layout.fillHeight: true
            label: "avg turnaround"
            valueText: root.fmtNum(root.stats.avg_turnaround, 2)
            unitText: "ticks"
            valueColor: Theme.accentAlt
            sparkData: root.turnaroundHistory
            sparkColor: Theme.accentAlt
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.divider
        }

        StatItem {
            Layout.fillWidth: true
            Layout.fillHeight: true
            label: "throughput"
            valueText: root.fmtNum(root.stats.throughput, 4)
            unitText: "proc/tick"
            valueColor: Theme.warning
            sparkData: root.throughputHistory
            sparkColor: Theme.warning
        }
    }
}
