import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
/* Last so the CuteSim SpinBox wins over the Controls.Basic one */
import CuteSim.Viewer

/* ScenarioEditor — form-based editor for .scn scenario files.
   Globals on the left, scripted processes on the right; the I/O timeline is
   edited in the documented "t:dev[:dur], …" text form. Every change is
   validated through ScenarioBridge — the same C parser the binary runs — so
   a scenario that validates here always launches. */
Item {
    id: root
    anchors.fill: parent
    z: 120

    property var bridge: null          // ScenarioBridge (null in unit tests)
    property string filePath: ""
    property var validation: null      // last bridge.parse() result

    /* ── Globals ────────────────────────────────────────────────────── */
    property int quantumHi: 3
    property int quantumLo: 6
    property int seed:      42

    property int diskMin: 5;     property int diskMax: 5
    property int tapeMin: 8;     property int tapeMax: 8
    property int printerMin: 12; property int printerMax: 12
    property string diskMode:    "concurrent"
    property string tapeMode:    "concurrent"
    property string printerMode: "concurrent"

    /* Random workload keys — written only when no [process] blocks exist */
    property int processCount: 5
    property int pIo:          20
    property int pDisk:        34
    property int pTape:        33
    readonly property int pPrinter: Math.max(0, 100 - pDisk - pTape)
    property int serviceMin:   5
    property int serviceMax:   15

    readonly property bool valid: validation !== null && validation.ok === true

    signal closed()
    signal useScenario(string path)

    ListModel { id: procModel }

    /* ── Serialization ──────────────────────────────────────────────── */
    function durStr(min, max) {
        return min === max ? String(min) : min + "-" + max
    }
    function toScn() {
        var l = []
        l.push("# scenario written by the CuteSim scenario editor")
        l.push("quantum-hi = " + quantumHi)
        l.push("quantum-lo = " + quantumLo)
        l.push("seed = " + seed)
        l.push("disk-duration = "    + durStr(diskMin, diskMax))
        l.push("tape-duration = "    + durStr(tapeMin, tapeMax))
        l.push("printer-duration = " + durStr(printerMin, printerMax))
        l.push("disk-mode = "    + diskMode)
        l.push("tape-mode = "    + tapeMode)
        l.push("printer-mode = " + printerMode)
        if (procModel.count === 0) {
            l.push("")
            l.push("# random workload")
            l.push("process-count = " + processCount)
            l.push("p-io = " + pIo)
            l.push("p-disk = " + pDisk)
            l.push("p-tape = " + pTape)
            l.push("p-printer = " + pPrinter)
            l.push("service-duration = " + durStr(serviceMin, serviceMax))
        }
        for (var i = 0; i < procModel.count; i++) {
            var p = procModel.get(i)
            l.push("")
            l.push("[process]")
            l.push("arrival = " + p.arrival)
            l.push("burst = " + p.burst)
            if (p.io && p.io.trim().length > 0)
                l.push("io = " + p.io.trim())
        }
        return l.join("\n") + "\n"
    }

    function revalidate() {
        validation = bridge ? bridge.parse(toScn()) : null
    }

    /* ── Open / reset / save ────────────────────────────────────────── */
    function openWith(path) {
        if (path && path.length > 0 && bridge) {
            var m = bridge.summarize(path)
            if (m.ok) {
                filePath    = m.path
                quantumHi   = m.quantumHi;   quantumLo   = m.quantumLo
                seed        = m.seed
                diskMin     = m.diskMin;     diskMax     = m.diskMax
                tapeMin     = m.tapeMin;     tapeMax     = m.tapeMax
                printerMin  = m.printerMin;  printerMax  = m.printerMax
                diskMode    = m.diskMode
                tapeMode    = m.tapeMode
                printerMode = m.printerMode
                pIo         = m.pIo
                pDisk       = m.pDisk;       pTape       = m.pTape
                serviceMin  = m.serviceMin;  serviceMax  = m.serviceMax
                if (!m.scripted) processCount = m.processCount
                procModel.clear()
                var procs = m.processes || []
                for (var i = 0; i < procs.length; i++)
                    procModel.append({ arrival: procs[i].arrival,
                                       burst:   procs[i].burst,
                                       io:      procs[i].io })
                revalidate()
                return
            }
        }
        reset()
    }
    function reset() {
        filePath = ""
        quantumHi = 3; quantumLo = 6; seed = 42
        diskMin = 5; diskMax = 5; tapeMin = 8; tapeMax = 8
        printerMin = 12; printerMax = 12
        diskMode = "concurrent"; tapeMode = "concurrent"; printerMode = "concurrent"
        processCount = 5; pIo = 20; pDisk = 34; pTape = 33
        serviceMin = 5; serviceMax = 15
        procModel.clear()
        revalidate()
    }
    function addProcess() {
        procModel.append({ arrival: 0, burst: 10, io: "" })
        revalidate()
    }
    function saveTo(path) {
        if (!bridge) return false
        if (!bridge.writeFile(path, toScn())) return false
        filePath = bridge.toLocalPath(path)
        return true
    }

    property bool pendingUse: false
    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        nameFilters: ["Scenario files (*.scn)"]
        defaultSuffix: "scn"
        onAccepted: {
            if (root.saveTo(selectedFile.toString()) && root.pendingUse) {
                root.pendingUse = false
                root.useScenario(root.filePath)
            }
        }
        onRejected: root.pendingUse = false
    }

    /* ── Backdrop ───────────────────────────────────────────────────── */
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.82)
        MouseArea { anchors.fill: parent }
    }

    /* ── Card ───────────────────────────────────────────────────────── */
    GlassCard {
        width: Math.min(920, parent.width - 80)
        height: Math.min(660, parent.height - 60)
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            /* Header */
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text {
                    text: "SCENARIO EDITOR"
                    color: Theme.accent
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    font.letterSpacing: 2.4
                }
                Text {
                    text: root.filePath.length > 0 ? root.filePath : "(unsaved)"
                    color: Theme.textDim
                    font.family: "Menlo, Monaco, Courier New, monospace"
                    font.pixelSize: 10
                    elide: Text.ElideLeft
                    Layout.fillWidth: true
                }
                Rectangle {
                    width: 22; height: 22; radius: 4
                    color: closeHov.hovered ? Theme.hover : "transparent"
                    Text { anchors.centerIn: parent; text: "✕"; color: Theme.textDim; font.pixelSize: 12 }
                    HoverHandler { id: closeHov }
                    TapHandler { onTapped: root.closed() }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 20

                /* ── Left: globals ──────────────────────────────────── */
                ColumnLayout {
                    Layout.preferredWidth: 360
                    Layout.fillHeight: true
                    spacing: 8

                    Text {
                        text: "GLOBALS"
                        color: Theme.textDim
                        font.family: Theme.fontFamily; font.pixelSize: 10
                        font.weight: Font.Bold; font.letterSpacing: 1.4
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: 10
                        rowSpacing: 8

                        Text { text: "quantum hi / lo"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 11 }
                        SpinBox {
                            boxWidth: 88; minimumValue: 1; maximumValue: 50
                            value: root.quantumHi
                            onValueChanged: { root.quantumHi = value; root.revalidate() }
                        }
                        SpinBox {
                            boxWidth: 88; minimumValue: 1; maximumValue: 50
                            value: root.quantumLo
                            onValueChanged: { root.quantumLo = value; root.revalidate() }
                        }

                        Text { text: "seed"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 11 }
                        SpinBox {
                            boxWidth: 88; minimumValue: 0; maximumValue: 99999
                            value: root.seed
                            onValueChanged: { root.seed = value; root.revalidate() }
                        }
                        Item { width: 1; height: 1 }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider; opacity: 0.6 }

                    Repeater {
                        model: [
                            { name: "disk",    minKey: "diskMin",    maxKey: "diskMax",    modeKey: "diskMode"    },
                            { name: "tape",    minKey: "tapeMin",    maxKey: "tapeMax",    modeKey: "tapeMode"    },
                            { name: "printer", minKey: "printerMin", maxKey: "printerMax", modeKey: "printerMode" }
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                text: modelData.name
                                color: Theme.text
                                font.family: Theme.fontFamily; font.pixelSize: 11
                                Layout.preferredWidth: 52
                            }
                            SpinBox {
                                boxWidth: 80; minimumValue: 1; maximumValue: 100
                                value: root[modelData.minKey]
                                onValueChanged: { root[modelData.minKey] = value; root.revalidate() }
                            }
                            SpinBox {
                                boxWidth: 80; minimumValue: root[modelData.minKey]; maximumValue: 100
                                value: root[modelData.maxKey]
                                onValueChanged: { root[modelData.maxKey] = value; root.revalidate() }
                            }
                            SegmentControl {
                                options: ["concurrent", "queue"]
                                value: root[modelData.modeKey]
                                onSelected: (v) => { root[modelData.modeKey] = v; root.revalidate() }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider; opacity: 0.6 }

                    Text {
                        text: "RANDOM WORKLOAD (no scripted processes)"
                        color: Theme.textDim
                        font.family: Theme.fontFamily; font.pixelSize: 10
                        font.weight: Font.Bold; font.letterSpacing: 1.4
                        opacity: procModel.count === 0 ? 1.0 : 0.4
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: 10
                        rowSpacing: 8
                        enabled: procModel.count === 0
                        opacity: procModel.count === 0 ? 1.0 : 0.4

                        Text { text: "processes · p-io %"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 11 }
                        SpinBox {
                            boxWidth: 88; minimumValue: 1; maximumValue: 50
                            value: root.processCount
                            onValueChanged: { root.processCount = value; root.revalidate() }
                        }
                        SpinBox {
                            boxWidth: 88; minimumValue: 0; maximumValue: 100
                            value: root.pIo
                            onValueChanged: { root.pIo = value; root.revalidate() }
                        }

                        Text {
                            text: "split d / t (p: " + root.pPrinter + ")"
                            color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 11
                        }
                        SpinBox {
                            boxWidth: 88; minimumValue: 0; maximumValue: 100
                            value: root.pDisk
                            onValueChanged: { root.pDisk = value; root.revalidate() }
                        }
                        SpinBox {
                            boxWidth: 88; minimumValue: 0; maximumValue: 100 - root.pDisk
                            value: root.pTape
                            onValueChanged: { root.pTape = value; root.revalidate() }
                        }

                        Text { text: "service min / max"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 11 }
                        SpinBox {
                            boxWidth: 88; minimumValue: 1; maximumValue: 100
                            value: root.serviceMin
                            onValueChanged: { root.serviceMin = value; root.revalidate() }
                        }
                        SpinBox {
                            boxWidth: 88; minimumValue: root.serviceMin; maximumValue: 100
                            value: root.serviceMax
                            onValueChanged: { root.serviceMax = value; root.revalidate() }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                Rectangle { width: 1; Layout.fillHeight: true; color: Theme.divider }

                /* ── Right: scripted processes ──────────────────────── */
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "SCRIPTED PROCESSES"
                            color: Theme.textDim
                            font.family: Theme.fontFamily; font.pixelSize: 10
                            font.weight: Font.Bold; font.letterSpacing: 1.4
                        }
                        Item { Layout.fillWidth: true }
                        CtrlButton {
                            label: "+ ADD"
                            onClicked: root.addProcess()
                        }
                    }

                    Text {
                        text: "io timeline: service_tick:device[:duration], …   e.g.  2:disk, 4:tape:6, 7:printer:4-8"
                        color: Theme.textDim
                        font.family: "Menlo, Monaco, Courier New, monospace"
                        font.pixelSize: 9
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    ListView {
                        id: procList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: procModel
                        spacing: 6
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        delegate: Rectangle {
                            width: procList.width
                            height: 44
                            radius: 8
                            color: Theme.hover
                            border.width: 1
                            border.color: Theme.divider

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                Text {
                                    text: "P" + (index + 1)
                                    color: Theme.pidColor(index + 1)
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 12; font.weight: Font.Bold
                                    Layout.preferredWidth: 28
                                }

                                Text { text: "arrival"; color: Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 10 }
                                SpinBox {
                                    boxWidth: 76; minimumValue: 0; maximumValue: 9999
                                    value: model.arrival
                                    onValueChanged: {
                                        if (model.arrival !== value) {
                                            procModel.setProperty(index, "arrival", value)
                                            root.revalidate()
                                        }
                                    }
                                }

                                Text { text: "burst"; color: Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 10 }
                                SpinBox {
                                    boxWidth: 76; minimumValue: 0; maximumValue: 9999
                                    value: model.burst
                                    onValueChanged: {
                                        if (model.burst !== value) {
                                            procModel.setProperty(index, "burst", value)
                                            root.revalidate()
                                        }
                                    }
                                }

                                TextField {
                                    Layout.fillWidth: true
                                    text: model.io
                                    placeholderText: "io timeline (optional)"
                                    color: Theme.text
                                    placeholderTextColor: Qt.rgba(Theme.textDim.r, Theme.textDim.g, Theme.textDim.b, 0.5)
                                    font.family: "Menlo, Monaco, Courier New, monospace"
                                    font.pixelSize: 10
                                    background: Rectangle {
                                        radius: 6
                                        color: Theme.cardBgSolid
                                        border.width: 1
                                        border.color: parent.activeFocus ? Theme.accentGlow : Theme.divider
                                    }
                                    onTextEdited: {
                                        procModel.setProperty(index, "io", text)
                                        root.revalidate()
                                    }
                                }

                                Rectangle {
                                    width: 20; height: 20; radius: 4
                                    color: rmHov.hovered ? Theme.hover : "transparent"
                                    Text { anchors.centerIn: parent; text: "✕"; color: Theme.textDim; font.pixelSize: 10 }
                                    HoverHandler { id: rmHov }
                                    TapHandler {
                                        onTapped: {
                                            procModel.remove(index)
                                            root.revalidate()
                                        }
                                    }
                                }
                            }
                        }

                        Text {
                            visible: procModel.count === 0
                            anchors.centerIn: parent
                            text: "no scripted processes — the scenario runs a random workload"
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            opacity: 0.6
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

            /* ── Footer: validation + actions ───────────────────────── */
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: {
                        if (!root.validation) return "validation unavailable"
                        if (!root.valid) return "✗ " + root.validation.error
                        return "✓ valid — " + (root.validation.scripted
                                ? root.validation.processCount + " scripted process"
                                  + (root.validation.processCount !== 1 ? "es" : "")
                                : "random workload, " + root.validation.processCount + " processes")
                    }
                    color: root.valid ? Theme.accentAlt
                         : root.validation ? Theme.danger : Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                }

                CtrlButton {
                    label: "SAVE AS…"
                    enabledState: root.valid
                    onClicked: if (root.valid) saveDialog.open()
                }
                CtrlButton {
                    label: "SAVE & USE"
                    enabledState: root.valid
                    onClicked: {
                        if (!root.valid) return
                        if (root.filePath.length > 0) {
                            if (root.saveTo(root.filePath))
                                root.useScenario(root.filePath)
                        } else {
                            root.pendingUse = true
                            saveDialog.open()
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: revalidate()
}
