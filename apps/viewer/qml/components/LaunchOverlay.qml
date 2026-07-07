import QtQuick
import CuteSim.Viewer
import QtQuick.Layouts
import QtQuick.Dialogs

/* LaunchOverlay — configure and launch rr-feedback.
Two modes: a fully parameterised random workload, or a .scn scenario file
(a bundled preset, a file picked from disk, or one produced by the
ScenarioEditor). Validation of scenario files goes through ScenarioBridge —
the same C parser the binary runs.

Form layout convention: every parameter group is a SectionHead (title +
description, full width) with a single control row underneath — labels,
spins and buttons never share a line with the group title. */
Item {
    id: root
    anchors.fill: parent
    z: 100

    property bool launching: false
    property var bridge: null            // ScenarioBridge (null in unit tests)

    /* ── Launch mode ────────────────────────────────────────────────── */
    property string mode: "random"       // "random" | "scenario"

    /* ── Random workload params ─────────────────────────────────────── */
    property int processes: 5
    property int quantumHi: 3
    property int quantumLo: 6
    property int pIo: 0
    property int serviceMin: 5
    property int serviceMax: 15
    property int seed: 42

    property string arrivalMode: "batch" // batch | bernoulli | poisson | uniform
    property int arrivalRate: 20    // %/tick        (bernoulli)
    property int arrivalLambdaPct: 50    // λ×0.01/tick   (poisson)
    property int arrivalInterval: 3     // ticks         (uniform)

    property int pDisk: 34
    property int pTape: 33
    readonly property int pPrinter: Math.max(0, 100 - pDisk - pTape)

    property int diskMin: 5
    property int diskMax: 5
    property int tapeMin: 8
    property int tapeMax: 8
    property int printerMin: 12
    property int printerMax: 12
    property string diskMode: "concurrent"
    property string tapeMode: "concurrent"
    property string printerMode: "concurrent"

    /* ── Scenario mode ──────────────────────────────────────────────── */
    property string scenarioPath: ""
    property var scenarioSummary: null
    readonly property bool scenarioOk: scenarioSummary !== null && scenarioSummary.ok === true

    /* Presets shipped with the app (scenarios/ next to the binary). */
    property var bundled: []
    onBridgeChanged: bundled = bridge ? bridge.bundledScenarios() : []
    Component.onCompleted: if (bridge)
                               bundled = bridge.bundledScenarios()

    readonly property bool canLaunch: !launching && (mode === "random" || (scenarioPath.length > 0
                                                                           && scenarioOk))

    signal launchRequested(var params)
    signal editRequested(string path)

    /* Group header: title + description stacked, full width. The control
    row always goes on the line below. */
    component SectionHead: ColumnLayout {
        property string title: ""
        property string hint: ""
        Layout.fillWidth: true
        spacing: 3
        Text {
            text: title
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
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }

    /* Small inline label placed next to a control inside a control row. */
    component FieldTag: Text {
        color: Theme.textDim
        font.family: Theme.fontFamily
        font.pixelSize: 10
        font.letterSpacing: 0.6
    }

    /* ── API ────────────────────────────────────────────────────────── */
    function setScenario(path) {
        mode = "scenario";
        scenarioPath = path;
        refreshSummary();
    }
    function refreshSummary() {
        scenarioSummary = (bridge && scenarioPath.length > 0) ? bridge.summarize(scenarioPath) :
                                                                null;
    }
    function applyParams(p) {
        if (!p)
            return;
        mode = p.scenarioFile ? "scenario" : "random";
        scenarioPath = p.scenarioFile ?? scenarioPath;
        processes = p.processes ?? processes;
        quantumHi = p.quantumHi ?? quantumHi;
        quantumLo = p.quantumLo ?? quantumLo;
        pIo = p.pIo ?? pIo;
        serviceMin = p.serviceMin ?? serviceMin;
        serviceMax = p.serviceMax ?? serviceMax;
        seed = p.seed ?? seed;
        arrivalMode = p.arrivalMode ?? arrivalMode;
        arrivalRate = p.arrivalRate ?? arrivalRate;
        arrivalInterval = p.arrivalInterval ?? arrivalInterval;
        arrivalLambdaPct = p.arrivalLambda !== undefined ? Math.round(p.arrivalLambda * 100) :
                                                           arrivalLambdaPct;
        pDisk = p.pDisk ?? pDisk;
        pTape = p.pTape ?? pTape;
        diskMin = p.diskMin ?? diskMin;
        diskMax = p.diskMax ?? diskMax;
        tapeMin = p.tapeMin ?? tapeMin;
        tapeMax = p.tapeMax ?? tapeMax;
        printerMin = p.printerMin ?? printerMin;
        printerMax = p.printerMax ?? printerMax;
        diskMode = p.diskMode ?? diskMode;
        tapeMode = p.tapeMode ?? tapeMode;
        printerMode = p.printerMode ?? printerMode;
        if (mode === "scenario")
            refreshSummary();
    }
    function buildParams() {
        if (mode === "scenario")
            return {
                scenarioFile: scenarioPath
            };
        return {
            processes: processes,
            quantumHi: quantumHi,
            quantumLo: quantumLo,
            pIo: pIo,
            serviceMin: serviceMin,
            serviceMax: serviceMax,
            seed: seed,
            arrivalMode: arrivalMode,
            arrivalRate: arrivalRate,
            arrivalLambda: arrivalLambdaPct / 100.0,
            arrivalInterval: arrivalInterval,
            pDisk: pDisk,
            pTape: pTape,
            diskMin: diskMin,
            diskMax: diskMax,
            tapeMin: tapeMin,
            tapeMax: tapeMax,
            printerMin: printerMin,
            printerMax: printerMax,
            diskMode: diskMode,
            tapeMode: tapeMode,
            printerMode: printerMode
        };
    }

    /* ── Backdrop ───────────────────────────────────────────────────── */
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.78)
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop {
                    position: 0.0
                    color: Theme.accentSoft
                }
                GradientStop {
                    position: 0.5
                    color: "transparent"
                }
                GradientStop {
                    position: 1.0
                    color: "transparent"
                }
            }
            opacity: 0.8
        }
        MouseArea {
            anchors.fill: parent
        }
    }

    FileDialog {
        id: openDialog
        nameFilters: ["Scenario files (*.scn)", "All files (*)"]
        onAccepted: root.setScenario(root.bridge ? root.bridge.toLocalPath(selectedFile.toString()) :
                                                   selectedFile.toString())
    }

    /* ── Card ───────────────────────────────────────────────────────── */
    Rectangle {
        id: card
        // Random mode spreads over two columns; scenario mode is a single
        // narrower list. Never wider than the window.
        width: Math.min(root.mode === "random" ? 1100 : 640, root.width - 48)
        Behavior on width {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.cardBgSolid
        border.width: 1
        border.color: Theme.cardBorderHi
        radius: Theme.radiusLarge
        // Never taller than the window: content scrolls instead of clipping.
        height: Math.min(inner.implicitHeight + 56, root.height - 48)
        clip: true

        transform: Scale {
            id: scaleT
            origin.x: card.width / 2
            origin.y: card.height / 2
            xScale: 1.0
            yScale: 1.0
        }
        opacity: 1.0
        Component.onCompleted: {
            scaleT.xScale = 0.985;
            scaleT.yScale = 0.985;
            opacity = 0;
            entranceAnim.start();
        }
        ParallelAnimation {
            id: entranceAnim
            NumberAnimation {
                target: scaleT
                property: "xScale"
                from: 0.985
                to: 1.0
                duration: 280
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: scaleT
                property: "yScale"
                from: 0.985
                to: 1.0
                duration: 280
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: card
                property: "opacity"
                from: 0
                to: 1
                duration: 220
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: parent.height * 0.30
            radius: parent.radius
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop {
                    position: 0.0
                    color: Theme.accentSoft
                }
                GradientStop {
                    position: 1.0
                    color: "transparent"
                }
            }
        }

        Flickable {
            id: flick
            anchors.fill: parent
            anchors.margins: 28
            contentWidth: width
            contentHeight: inner.implicitHeight
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            ColumnLayout {
                id: inner
                width: flick.width
                spacing: 0

                /* ── Header ─────────────────────────────────────────── */
                RowLayout {
                    Layout.fillWidth: true
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
                    Item {
                        Layout.fillWidth: true
                    }
                    SegmentControl {
                        id: modeCtl
                        options: ["random", "scenario"]
                        value: root.mode
                        onSelected: v => {
                                        root.mode = v;
                                        if (v === "scenario")
                                        root.refreshSummary();
                                    }
                    }
                }

                Item {
                    Layout.preferredHeight: 18
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.divider
                }

                /* ── Random workload — two columns: workload/scheduling
                on the left, everything I/O on the right ─────────── */
                RowLayout {
                    visible: root.mode === "random"
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.bottomMargin: 16
                    spacing: 20

                    /* Left column — workload & scheduling */
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 100
                        Layout.alignment: Qt.AlignTop
                        spacing: 16

                        Text {
                            text: "WORKLOAD & SCHEDULING"
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            font.letterSpacing: 2.0
                        }

                        /* Processes + Seed side by side (single spins) */
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 20
                            rowSpacing: 16

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 100
                                Layout.alignment: Qt.AlignTop
                                spacing: 8
                                SectionHead {
                                    title: "Processes"
                                    hint: "how many processes enter the system"
                                }
                                SpinBox {
                                    boxWidth: 110
                                    minimumValue: 1
                                    maximumValue: 20
                                    value: root.processes
                                    onValueChanged: root.processes = value
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 100
                                Layout.alignment: Qt.AlignTop
                                spacing: 8
                                SectionHead {
                                    title: "Seed"
                                    hint: "RNG seed — same seed, same run"
                                }
                                SpinBox {
                                    boxWidth: 110
                                    minimumValue: 0
                                    maximumValue: 99999
                                    value: root.seed
                                    onValueChanged: root.seed = value
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            SectionHead {
                                title: "Quantum"
                                hint: "time slice per queue, in ticks"
                            }
                            RowLayout {
                                spacing: 8
                                FieldTag {
                                    text: "high"
                                }
                                SpinBox {
                                    boxWidth: 96
                                    minimumValue: 1
                                    maximumValue: 20
                                    value: root.quantumHi
                                    onValueChanged: root.quantumHi = value
                                }
                                FieldTag {
                                    text: "low"
                                    Layout.leftMargin: 8
                                }
                                SpinBox {
                                    boxWidth: 96
                                    minimumValue: 1
                                    maximumValue: 20
                                    value: root.quantumLo
                                    onValueChanged: root.quantumLo = value
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            SectionHead {
                                title: "Service"
                                hint: "CPU burst per process, sampled in [min, max]"
                            }
                            RowLayout {
                                spacing: 8
                                FieldTag {
                                    text: "min"
                                }
                                SpinBox {
                                    boxWidth: 96
                                    minimumValue: 1
                                    maximumValue: 100
                                    value: root.serviceMin
                                    onValueChanged: root.serviceMin = value
                                }
                                FieldTag {
                                    text: "max"
                                    Layout.leftMargin: 8
                                }
                                SpinBox {
                                    boxWidth: 96
                                    minimumValue: root.serviceMin
                                    maximumValue: 100
                                    value: root.serviceMax
                                    onValueChanged: root.serviceMax = value
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            SectionHead {
                                title: "Arrival"
                                hint: root.arrivalMode === "batch"
                                      ? "all processes arrive together at tick 0" :
                                        root.arrivalMode === "bernoulli"
                                        ? "each tick has an N% chance of one arrival" :
                                          root.arrivalMode === "poisson"
                                          ? "λ arrivals per tick on average (value ×0.01)" :
                                            "exactly one arrival every N ticks"
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                SegmentControl {
                                    options: ["batch", "bernoulli", "poisson", "uniform"]
                                    value: root.arrivalMode
                                    onSelected: v => root.arrivalMode = v
                                }
                                Item {
                                    Layout.fillWidth: true
                                }
                                FieldTag {
                                    visible: root.arrivalMode !== "batch"
                                    text: root.arrivalMode === "bernoulli" ? "% per tick" :
                                                                             root.arrivalMode
                                                                             === "poisson"
                                                                             ? "λ ×0.01" : "ticks"
                                }
                                SpinBox {
                                    visible: root.arrivalMode === "bernoulli"
                                    boxWidth: 96
                                    minimumValue: 1
                                    maximumValue: 100
                                    value: root.arrivalRate
                                    onValueChanged: root.arrivalRate = value
                                }
                                SpinBox {
                                    visible: root.arrivalMode === "poisson"
                                    boxWidth: 96
                                    minimumValue: 1
                                    maximumValue: 300
                                    value: root.arrivalLambdaPct
                                    onValueChanged: root.arrivalLambdaPct = value
                                }
                                SpinBox {
                                    visible: root.arrivalMode === "uniform"
                                    boxWidth: 96
                                    minimumValue: 1
                                    maximumValue: 50
                                    value: root.arrivalInterval
                                    onValueChanged: root.arrivalInterval = value
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 1
                        color: Theme.divider
                    }

                    /* Right column — I/O */
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 100
                        Layout.alignment: Qt.AlignTop
                        spacing: 16

                        Text {
                            text: "I/O"
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            font.letterSpacing: 2.0
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            SectionHead {
                                title: "I/O probability"
                                hint: "chance that the running process fires an I/O request on each CPU tick — 0 keeps the workload CPU-only"
                            }
                            RowLayout {
                                spacing: 8
                                SpinBox {
                                    boxWidth: 96
                                    minimumValue: 0
                                    maximumValue: 100
                                    value: root.pIo
                                    onValueChanged: root.pIo = value
                                }
                                FieldTag {
                                    text: "% per tick"
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            SectionHead {
                                title: "Device split"
                                hint: "which device each I/O request goes to — the printer takes whatever is left"
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                FieldTag {
                                    text: "disk"
                                }
                                SpinBox {
                                    boxWidth: 96
                                    minimumValue: 0
                                    maximumValue: 100
                                    value: root.pDisk
                                    onValueChanged: root.pDisk = value
                                }
                                FieldTag {
                                    text: "tape"
                                    Layout.leftMargin: 10
                                }
                                SpinBox {
                                    boxWidth: 96
                                    minimumValue: 0
                                    maximumValue: 100 - root.pDisk
                                    value: root.pTape
                                    onValueChanged: root.pTape = value
                                }
                                FieldTag {
                                    text: "printer"
                                    Layout.leftMargin: 10
                                }
                                Text {
                                    text: root.pPrinter + " %"
                                    color: Theme.text
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSizeMed
                                    font.weight: Font.Bold
                                }
                                Item {
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            SectionHead {
                                title: "I/O duration & mode"
                                hint: "how long a request holds each device (ticks in [min, max]) and how it serves — concurrent: all advance · queue: head only"
                            }
                            /* One aligned line per device: label · min/max spins ·
                            mode buttons (fixed column widths keep rows aligned). */
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Repeater {
                                    model: [
                                        {
                                            name: "Disk",
                                            minKey: "diskMin",
                                            maxKey: "diskMax",
                                            modeKey: "diskMode"
                                        },
                                        {
                                            name: "Tape",
                                            minKey: "tapeMin",
                                            maxKey: "tapeMax",
                                            modeKey: "tapeMode"
                                        },
                                        {
                                            name: "Printer",
                                            minKey: "printerMin",
                                            maxKey: "printerMax",
                                            modeKey: "printerMode"
                                        }
                                    ]
                                    delegate: RowLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        spacing: 8
                                        Text {
                                            text: modelData.name
                                            color: Theme.text
                                            font.family: Theme.fontFamily
                                            font.pixelSize: 11
                                            font.weight: Font.Medium
                                            Layout.preferredWidth: 52
                                        }
                                        FieldTag {
                                            text: "min"
                                        }
                                        SpinBox {
                                            boxWidth: 96
                                            minimumValue: 1
                                            maximumValue: 100
                                            value: root[modelData.minKey]
                                            onValueChanged: root[modelData.minKey] = value
                                        }
                                        FieldTag {
                                            text: "max"
                                            Layout.leftMargin: 8
                                        }
                                        SpinBox {
                                            boxWidth: 96
                                            minimumValue: root[modelData.minKey]
                                            maximumValue: 100
                                            value: root[modelData.maxKey]
                                            onValueChanged: root[modelData.maxKey] = value
                                        }
                                        Item {
                                            Layout.fillWidth: true
                                        }
                                        SegmentControl {
                                            options: ["concurrent", "queue"]
                                            value: root[modelData.modeKey]
                                            onSelected: v => root[modelData.modeKey] = v
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                /* ── Scenario file ──────────────────────────────────── */
                ColumnLayout {
                    visible: root.mode === "scenario"
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.bottomMargin: 16
                    spacing: 16

                    /* Bundled presets */
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        SectionHead {
                            title: "Bundled scenarios"
                            hint: "ready-made presets that ship with CuteSim — click one to select it"
                        }
                        Repeater {
                            model: root.bundled
                            delegate: Rectangle {
                                id: bundledItem
                                required property var modelData
                                readonly property bool active: root.scenarioPath === modelData.path
                                Layout.fillWidth: true
                                implicitHeight: bundledCol.implicitHeight + 18
                                radius: 8
                                color: active ? Qt.rgba(Theme.accent.r, Theme.accent.g,
                                                        Theme.accent.b, 0.10) : (bundledHov.hovered
                                                                                 ? Theme.hover :
                                                                                   Theme.cardBg)
                                border.width: 1
                                border.color: active ? Theme.accentGlow : Theme.divider
                                Behavior on color {
                                    ColorAnimation {
                                        duration: Theme.durFast
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 10
                                    ColumnLayout {
                                        id: bundledCol
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Text {
                                            text: bundledItem.modelData.title
                                            color: bundledItem.active ? Theme.accent : Theme.text
                                            font.family: Theme.fontFamily
                                            font.pixelSize: 12
                                            font.weight: Font.Medium
                                        }
                                        Text {
                                            text: bundledItem.modelData.description
                                            visible: text.length > 0
                                            color: Theme.textDim
                                            font.family: Theme.fontFamily
                                            font.pixelSize: 10
                                            wrapMode: Text.Wrap
                                            maximumLineCount: 2
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                    Text {
                                        text: bundledItem.active ? "✓" : "▸"
                                        color: bundledItem.active ? Theme.accent : Theme.textDim
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 13
                                    }
                                }

                                HoverHandler {
                                    id: bundledHov
                                    cursorShape: Qt.PointingHandCursor
                                }
                                TapHandler {
                                    onTapped: root.setScenario(bundledItem.modelData.path)
                                }
                            }
                        }
                        Text {
                            visible: root.bundled.length === 0
                            text: "no bundled scenarios found next to the app"
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            font.italic: true
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.divider
                    }

                    /* Custom file */
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        SectionHead {
                            title: "Custom file"
                            hint: "pick a .scn from disk or build one in the editor — the file supplies the whole configuration"
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Rectangle {
                                Layout.fillWidth: true
                                height: 32
                                radius: 8
                                color: Theme.hover
                                border.width: 1
                                border.color: Theme.divider
                                Text {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    verticalAlignment: Text.AlignVCenter
                                    text: root.scenarioPath.length > 0 ? root.scenarioPath :
                                                                         "(no file selected)"
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
                    }

                    Text {
                        visible: root.scenarioPath.length > 0
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: {
                            if (!root.scenarioSummary)
                                return "…";
                            if (!root.scenarioOk)
                                return "✗ " + root.scenarioSummary.error;
                            var s = root.scenarioSummary;
                            return "✓ valid — " + (s.scripted ? s.processCount
                                                                + " scripted process" + (
                                                                    s.processCount !== 1 ? "es" :
                                                                                           "") : "random workload, "
                                                                + s.processCount + " processes")
                                    + " · quantum " + s.quantumHi + "/" + s.quantumLo + " · seed "
                                    + s.seed;
                        }
                        color: root.scenarioOk ? Theme.accentAlt : Theme.danger
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.divider
                }

                /* ── Launch button ──────────────────────────────────── */
                Item {
                    Layout.fillWidth: true
                    Layout.topMargin: 18
                    Layout.preferredHeight: 46

                    Rectangle {
                        anchors.fill: parent
                        radius: 10
                        color: !root.canLaunch ? Theme.hoverStrong : (launchMouse.containsMouse
                                                                      ? Theme.accent : Qt.rgba(
                                                                            Theme.accent.r,
                                                                            Theme.accent.g,
                                                                            Theme.accent.b, 0.10))
                        border.width: 1
                        border.color: !root.canLaunch ? "transparent" : Theme.accentGlow
                        Behavior on color {
                            ColorAnimation {
                                duration: Theme.durFast
                            }
                        }
                    }

                    Row {
                        anchors.centerIn: parent
                        spacing: 10
                        Text {
                            visible: root.canLaunch
                            text: "▶"
                            color: launchMouse.containsMouse ? "#000" : Theme.accent
                            font.family: Theme.fontFamily
                            font.pixelSize: 13
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: root.launching ? "LAUNCHING…" : "LAUNCH SIMULATOR"
                            color: !root.canLaunch ? Theme.textDim : (launchMouse.containsMouse
                                                                      ? "#000" : Theme.accent)
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
                                root.launchRequested(root.buildParams());
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
}
