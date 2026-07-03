import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts
import QtQuick.Controls.Basic

GlassCard {
    id: root

    property string rawSnapshot: ""
    property var events: []
    property var params: null            // SimController.lastParams
    property var bridge: null            // ScenarioBridge (to read .scn files)

    property string view: "snapshot"     // "snapshot" | "config"

    // rawSnapshot pretty-printed with 2-space indentation; falls back to the
    // raw string when it is not valid JSON.
    readonly property string prettySnapshot: {
        if (!rawSnapshot) return ""
        try { return JSON.stringify(JSON.parse(rawSnapshot), null, 2) }
        catch (e) { return rawSnapshot }
    }

    // What the running simulation was launched with: the scenario file
    // (path + contents) or the random-workload options.
    readonly property string configText: {
        var p = params
        if (!p || Object.keys(p).length === 0)
            return ""
        if (p.scenarioFile) {
            var body = bridge ? bridge.readFile(p.scenarioFile) : ""
            return "# scenario file\n" + p.scenarioFile + "\n\n"
                 + (body.length > 0 ? body : "(cannot read file)")
        }
        function row(k, v) { return (k + "            ").slice(0, 12) + v }
        function dur(lo, hi) { return lo === hi ? lo + "t" : lo + "-" + hi + "t" }
        var arrival = p.arrivalMode === "bernoulli"
                        ? "bernoulli — " + p.arrivalRate + "% per tick"
                    : p.arrivalMode === "poisson"
                        ? "poisson — λ " + p.arrivalLambda + " per tick"
                    : p.arrivalMode === "uniform"
                        ? "uniform — every " + p.arrivalInterval + " ticks"
                        : "batch — all at tick 0"
        return [
            "# random workload",
            "",
            row("processes",  p.processes),
            row("seed",       p.seed),
            row("quantum",    p.quantumHi + " / " + p.quantumLo),
            row("service",    dur(p.serviceMin, p.serviceMax)),
            row("arrival",    arrival),
            row("i/o prob",   p.pIo + "% per tick"),
            row("split",      "disk " + p.pDisk + " · tape " + p.pTape
                              + " · printer " + Math.max(0, 100 - p.pDisk - p.pTape)),
            row("disk",       dur(p.diskMin, p.diskMax) + " · " + p.diskMode),
            row("tape",       dur(p.tapeMin, p.tapeMax) + " · " + p.tapeMode),
            row("printer",    dur(p.printerMin, p.printerMax) + " · " + p.printerMode)
        ].join("\n")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "INSPECTOR"
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1.4
            }
            Item { Layout.fillWidth: true }
            Text {
                text: root.events.length + " event" + (root.events.length !== 1 ? "s" : "")
                color: root.events.length > 0 ? Theme.accent : Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.weight: Font.Medium
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

        // Events this tick
        Column {
            Layout.fillWidth: true
            spacing: 4
            visible: root.events.length > 0

            Repeater {
                model: root.events
                delegate: RowLayout {
                    width: parent.width
                    spacing: 8

                    Rectangle {
                        width: 6; height: 6; radius: 1
                        color: {
                            var t = modelData.type
                            if (t === "arrived")   return Theme.accentAlt
                            if (t === "scheduled") return Theme.qHigh
                            if (t === "preempted") return Theme.warning
                            if (t === "io_start")  return Theme.qDisk
                            if (t === "io_return") return Theme.qTape
                            if (t === "completed") return Theme.accentAlt
                            return Theme.textDim
                        }
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        text: modelData.type
                        color: Theme.accent
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.6
                    }

                    Text {
                        text: "P" + modelData.pid
                        color: Theme.pidColor(modelData.pid)
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.weight: Font.Bold
                    }

                    Text {
                        text: {
                            var d = modelData
                            if (d.type === "preempted")
                                return "quantum " + d.quantum_used + "/" + d.quantum_max
                            if (d.type === "io_start" || d.type === "io_return")
                                return d.device || ""
                            if (d.type === "scheduled" && d.queue)
                                return "→ " + d.queue
                            return ""
                        }
                        color: Theme.textDim
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        Layout.fillWidth: true
                    }
                }
            }
        }

        Text {
            visible: root.events.length === 0
            text: "no events this tick"
            color: Theme.textDim
            font.family: Theme.fontFamily
            font.pixelSize: 11
            opacity: 0.6
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

        // Bottom area: raw JSON snapshot or the launch configuration
        RowLayout {
            Layout.fillWidth: true
            SegmentControl {
                options: ["snapshot", "config"]
                value: root.view
                segPixelSize: 9
                onSelected: (v) => root.view = v
            }
            Item { Layout.fillWidth: true }
            Text {
                visible: root.view === "config"
                text: (root.params && root.params.scenarioFile) ? "scenario file"
                    : (root.params && Object.keys(root.params).length > 0) ? "random workload"
                                                                           : ""
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 10
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            TextArea {
                readOnly: true
                text: root.view === "config"
                      ? (root.configText || "(not launched yet)")
                      : (root.prettySnapshot || "(none)")
                color: Theme.textDim
                font.family: "Menlo, Monaco, Courier New, monospace"
                font.pixelSize: 9
                wrapMode: TextEdit.WrapAnywhere
                selectByMouse: true
                background: null
                topPadding: 0
                leftPadding: 0
            }
        }
    }
}
