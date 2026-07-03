import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts
import QtQuick.Dialogs

/* LaunchOverlay — configure and launch rr-feedback.
   Two modes: a fully parameterised random workload, or a .scn scenario file
   (picked from disk or produced by the ScenarioEditor). Validation of
   scenario files goes through ScenarioBridge — the same C parser the binary
   runs. */
Item {
    id: root
    anchors.fill: parent
    z: 100

    property bool launching: false
    property var bridge: null            // ScenarioBridge (null in unit tests)

    /* ── Launch mode ────────────────────────────────────────────────── */
    property string mode: "random"       // "random" | "scenario"

    /* ── Random workload params ─────────────────────────────────────── */
    property int processes:  5
    property int quantumHi:  3
    property int quantumLo:  6
    property int pIo:        0
    property int serviceMin: 5
    property int serviceMax: 15
    property int seed:       42

    property string arrivalMode: "batch" // batch | bernoulli | poisson | uniform
    property int arrivalRate:      20    // %/tick        (bernoulli)
    property int arrivalLambdaPct: 50    // λ×0.01/tick   (poisson)
    property int arrivalInterval:  3     // ticks         (uniform)

    property int pDisk: 34
    property int pTape: 33
    readonly property int pPrinter: Math.max(0, 100 - pDisk - pTape)

    property int diskMin: 5;    property int diskMax: 5
    property int tapeMin: 8;    property int tapeMax: 8
    property int printerMin: 12; property int printerMax: 12
    property string diskMode:    "concurrent"
    property string tapeMode:    "concurrent"
    property string printerMode: "concurrent"

    /* ── Scenario mode ──────────────────────────────────────────────── */
    property string scenarioPath: ""
    property var    scenarioSummary: null
    readonly property bool scenarioOk: scenarioSummary !== null
                                       && scenarioSummary.ok === true

    readonly property bool canLaunch: !launching &&
        (mode === "random" || (scenarioPath.length > 0 && scenarioOk))

    signal launchRequested(var params)
    signal editRequested(string path)

    /* Reusable label + hint column for parameter rows */
    component ParamLabel: ColumnLayout {
        property string label: ""
        property string hint: ""
        Layout.fillWidth: true
        spacing: 2
        Text {
            text: label
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: 12
            font.weight: Font.Medium
        }
        Text {
            visible: hint.length > 0
            text: hint
            color: Theme.textDim
            font.family: Theme.fontFamily
            font.pixelSize: 10
        }
    }

    /* ── API ────────────────────────────────────────────────────────── */
    function setScenario(path) {
        mode = "scenario"
        scenarioPath = path
        refreshSummary()
    }
    function refreshSummary() {
        scenarioSummary = (bridge && scenarioPath.length > 0)
                          ? bridge.summarize(scenarioPath) : null
    }
    function applyParams(p) {
        if (!p) return
        mode             = p.scenarioFile ? "scenario" : "random"
        scenarioPath     = p.scenarioFile      ?? scenarioPath
        processes        = p.processes         ?? processes
        quantumHi        = p.quantumHi         ?? quantumHi
        quantumLo        = p.quantumLo         ?? quantumLo
        pIo              = p.pIo               ?? pIo
        serviceMin       = p.serviceMin        ?? serviceMin
        serviceMax       = p.serviceMax        ?? serviceMax
        seed             = p.seed              ?? seed
        arrivalMode      = p.arrivalMode       ?? arrivalMode
        arrivalRate      = p.arrivalRate       ?? arrivalRate
        arrivalInterval  = p.arrivalInterval   ?? arrivalInterval
        arrivalLambdaPct = p.arrivalLambda !== undefined
                           ? Math.round(p.arrivalLambda * 100) : arrivalLambdaPct
        pDisk            = p.pDisk             ?? pDisk
        pTape            = p.pTape             ?? pTape
        diskMin          = p.diskMin           ?? diskMin
        diskMax          = p.diskMax           ?? diskMax
        tapeMin          = p.tapeMin           ?? tapeMin
        tapeMax          = p.tapeMax           ?? tapeMax
        printerMin       = p.printerMin        ?? printerMin
        printerMax       = p.printerMax        ?? printerMax
        diskMode         = p.diskMode          ?? diskMode
        tapeMode         = p.tapeMode          ?? tapeMode
        printerMode      = p.printerMode       ?? printerMode
        if (mode === "scenario") refreshSummary()
    }
    function buildParams() {
        if (mode === "scenario")
            return { scenarioFile: scenarioPath }
        return {
            processes: processes, quantumHi: quantumHi, quantumLo: quantumLo,
            pIo: pIo, serviceMin: serviceMin, serviceMax: serviceMax, seed: seed,
            arrivalMode: arrivalMode, arrivalRate: arrivalRate,
            arrivalLambda: arrivalLambdaPct / 100.0, arrivalInterval: arrivalInterval,
            pDisk: pDisk, pTape: pTape,
            diskMin: diskMin, diskMax: diskMax,
            tapeMin: tapeMin, tapeMax: tapeMax,
            printerMin: printerMin, printerMax: printerMax,
            diskMode: diskMode, tapeMode: tapeMode, printerMode: printerMode
        }
    }

    /* ── Backdrop ───────────────────────────────────────────────────── */
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

    FileDialog {
        id: openDialog
        nameFilters: ["Scenario files (*.scn)", "All files (*)"]
        onAccepted: root.setScenario(
            root.bridge ? root.bridge.toLocalPath(selectedFile.toString())
                        : selectedFile.toString())
    }

    /* ── Card ───────────────────────────────────────────────────────── */
    Rectangle {
        id: card
        width: 560
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

            /* ── Header ─────────────────────────────────────────────── */
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
                Item { Layout.fillWidth: true }
                SegmentControl {
                    id: modeCtl
                    options: ["random", "scenario"]
                    value: root.mode
                    onSelected: (v) => { root.mode = v; if (v === "scenario") root.refreshSummary() }
                }
            }

            Item { Layout.preferredHeight: 18 }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

            /* ── Random workload ────────────────────────────────────── */
            ColumnLayout {
                visible: root.mode === "random"
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.bottomMargin: 16
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14
                    ParamLabel { label: "Processes"; hint: "total process count" }
                    SpinBox {
                        boxWidth: 110; minimumValue: 1; maximumValue: 20
                        value: root.processes
                        onValueChanged: root.processes = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14
                    ParamLabel { label: "Quantum hi / lo"; hint: "time slices per queue" }
                    SpinBox {
                        boxWidth: 110; minimumValue: 1; maximumValue: 20
                        value: root.quantumHi
                        onValueChanged: root.quantumHi = value
                    }
                    SpinBox {
                        boxWidth: 110; minimumValue: 1; maximumValue: 20
                        value: root.quantumLo
                        onValueChanged: root.quantumLo = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14
                    ParamLabel { label: "Service min / max"; hint: "CPU burst duration range" }
                    SpinBox {
                        boxWidth: 110; minimumValue: 1; maximumValue: 100
                        value: root.serviceMin
                        onValueChanged: root.serviceMin = value
                    }
                    SpinBox {
                        boxWidth: 110; minimumValue: root.serviceMin; maximumValue: 100
                        value: root.serviceMax
                        onValueChanged: root.serviceMax = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14
                    ParamLabel { label: "Seed"; hint: "RNG seed" }
                    SpinBox {
                        boxWidth: 110; minimumValue: 0; maximumValue: 99999
                        value: root.seed
                        onValueChanged: root.seed = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14
                    ParamLabel {
                        label: "Arrival"
                        hint: root.arrivalMode === "batch"     ? "all processes at tick 0"
                            : root.arrivalMode === "bernoulli" ? "% chance per tick"
                            : root.arrivalMode === "poisson"   ? "λ ×0.01 arrivals per tick"
                                                               : "one arrival every N ticks"
                    }
                    SegmentControl {
                        options: ["batch", "bernoulli", "poisson", "uniform"]
                        value: root.arrivalMode
                        onSelected: (v) => root.arrivalMode = v
                    }
                    SpinBox {
                        visible: root.arrivalMode === "bernoulli"
                        boxWidth: 96; minimumValue: 1; maximumValue: 100
                        value: root.arrivalRate
                        onValueChanged: root.arrivalRate = value
                    }
                    SpinBox {
                        visible: root.arrivalMode === "poisson"
                        boxWidth: 96; minimumValue: 1; maximumValue: 300
                        value: root.arrivalLambdaPct
                        onValueChanged: root.arrivalLambdaPct = value
                    }
                    SpinBox {
                        visible: root.arrivalMode === "uniform"
                        boxWidth: 96; minimumValue: 1; maximumValue: 50
                        value: root.arrivalInterval
                        onValueChanged: root.arrivalInterval = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14
                    ParamLabel {
                        label: "I/O prob %  ·  split disk / tape"
                        hint: "printer gets the remainder: " + root.pPrinter + "%"
                    }
                    SpinBox {
                        boxWidth: 96; minimumValue: 0; maximumValue: 100
                        value: root.pIo
                        onValueChanged: root.pIo = value
                    }
                    SpinBox {
                        boxWidth: 96; minimumValue: 0; maximumValue: 100
                        value: root.pDisk
                        onValueChanged: root.pDisk = value
                    }
                    SpinBox {
                        boxWidth: 96; minimumValue: 0; maximumValue: 100 - root.pDisk
                        value: root.pTape
                        onValueChanged: root.pTape = value
                    }
                }

                /* Per-device duration + execution mode */
                Repeater {
                    model: [
                        { name: "Disk",    minKey: "diskMin",    maxKey: "diskMax",    modeKey: "diskMode"    },
                        { name: "Tape",    minKey: "tapeMin",    maxKey: "tapeMax",    modeKey: "tapeMode"    },
                        { name: "Printer", minKey: "printerMin", maxKey: "printerMax", modeKey: "printerMode" }
                    ]
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: 14
                        ParamLabel { label: modelData.name; hint: "I/O duration min / max · mode" }
                        SpinBox {
                            boxWidth: 96; minimumValue: 1; maximumValue: 100
                            value: root[modelData.minKey]
                            onValueChanged: root[modelData.minKey] = value
                        }
                        SpinBox {
                            boxWidth: 96; minimumValue: root[modelData.minKey]; maximumValue: 100
                            value: root[modelData.maxKey]
                            onValueChanged: root[modelData.maxKey] = value
                        }
                        SegmentControl {
                            options: ["concurrent", "queue"]
                            value: root[modelData.modeKey]
                            onSelected: (v) => root[modelData.modeKey] = v
                        }
                    }
                }
            }

            /* ── Scenario file ──────────────────────────────────────── */
            ColumnLayout {
                visible: root.mode === "scenario"
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.bottomMargin: 16
                spacing: 12

                Text {
                    text: "Pick a .scn scenario file, or build one in the editor.\nScenario values take precedence over defaults."
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                    lineHeight: 1.5
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Rectangle {
                        Layout.fillWidth: true
                        height: 34
                        radius: 8
                        color: Theme.hover
                        border.width: 1
                        border.color: Theme.divider
                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            verticalAlignment: Text.AlignVCenter
                            text: root.scenarioPath.length > 0 ? root.scenarioPath : "(no file selected)"
                            color: root.scenarioPath.length > 0 ? Theme.text : Theme.textDim
                            font.family: "Menlo, Monaco, Courier New, monospace"
                            font.pixelSize: 10
                            elide: Text.ElideLeft
                        }
                    }

                    CtrlButton {
                        label: "BROWSE…"
                        onClicked: openDialog.open()
                    }
                    CtrlButton {
                        label: root.scenarioPath.length > 0 ? "EDIT" : "NEW"
                        onClicked: root.editRequested(root.scenarioPath)
                    }
                }

                Text {
                    visible: root.scenarioPath.length > 0
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: {
                        if (!root.scenarioSummary) return "…"
                        if (!root.scenarioOk) return "✗ " + root.scenarioSummary.error
                        var s = root.scenarioSummary
                        return "✓ valid — " + (s.scripted
                                ? s.processCount + " scripted process" + (s.processCount !== 1 ? "es" : "")
                                : "random workload, " + s.processCount + " processes")
                             + " · quantum " + s.quantumHi + "/" + s.quantumLo
                             + " · seed " + s.seed
                    }
                    color: root.scenarioOk ? Theme.accentAlt : Theme.danger
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

            /* ── Launch button ──────────────────────────────────────── */
            Item {
                Layout.fillWidth: true
                Layout.topMargin: 18
                Layout.preferredHeight: 46

                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: !root.canLaunch
                            ? Theme.hoverStrong
                            : (launchMouse.containsMouse
                                ? Theme.accent
                                : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.10))
                    border.width: 1
                    border.color: !root.canLaunch ? "transparent" : Theme.accentGlow
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                }

                Row {
                    anchors.centerIn: parent
                    spacing: 10
                    Text {
                        visible: root.canLaunch
                        text: "▶"
                        color: launchMouse.containsMouse ? "#000" : Theme.accent
                        font.family: Theme.fontFamily; font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: root.launching ? "LAUNCHING…" : "LAUNCH SIMULATOR"
                        color: !root.canLaunch
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
                    cursorShape: root.canLaunch ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (root.canLaunch)
                            root.launchRequested(root.buildParams())
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
