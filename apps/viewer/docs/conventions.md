# CuteSim Viewer — QML Conventions

Settled before writing any UI code. All T5+ work follows these rules.

---

## Directory layout

```
apps/viewer/
├── CMakeLists.txt
├── main.cpp
├── src/
│   ├── SimController.h/.cpp    ← Q_PROPERTY surface for QML; owns SimClient
│   └── SimClient.h/.cpp        ← QTcpSocket + JSON Lines; optionally reads file
├── qml/
│   ├── Main.qml                ← window root, layout
│   ├── theme/
│   │   └── Theme.qml           ← pragma Singleton, all design tokens
│   └── components/
│       ├── GlassCard.qml
│       ├── Header.qml
│       ├── CPUView.qml
│       ├── QueueView.qml
│       ├── QueueChip.qml
│       ├── TimelineChart.qml
│       ├── StatsBar.qml
│       ├── StatItem.qml
│       ├── FinishedTable.qml
│       ├── Sparkline.qml
│       ├── LaunchOverlay.qml
│       ├── InspectorPanel.qml  ← T7b: raw JSON + event log, toggleable
│       ├── Toast.qml
│       ├── StatusChip.qml
│       ├── TickChip.qml
│       ├── CtrlButton.qml
│       ├── SpinBox.qml
│       └── Logo.qml
└── docs/
    └── conventions.md          ← this file
```

---

## CMake module declaration

Use `qt_add_qml_module` (Qt 6.2+). Single URI `CuteSim.Viewer`.

```cmake
find_package(Qt6 REQUIRED COMPONENTS Quick Network)
qt_standard_project_setup(REQUIRES 6.2)

qt_add_executable(cutesim_viewer main.cpp src/SimController.cpp src/SimClient.cpp)

qt_add_qml_module(cutesim_viewer
    URI CuteSim.Viewer
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/components/GlassCard.qml
        qml/components/Header.qml
        # ... all component files
    RESOURCES
        # fonts, assets
)

target_link_libraries(cutesim_viewer PRIVATE cutesim_lib Qt6::Quick Qt6::Network)
```

`Theme.qml` keeps `pragma Singleton` in QML — no need for `QML_SINGLETON` on a C++ type. The QML engine handles it automatically when the file is listed in `QML_FILES` and has the pragma.

**No `.qrc` files.** `qt_add_qml_module` and `qt_add_resources` handle embedding automatically via CMake.

---

## Singleton Theme

```qml
// Theme.qml
pragma Singleton
import QtQuick

QtObject {
    id: theme
    // ... tokens
}
```

Import in any component:

```qml
import CuteSim.Viewer
// or for same-module files, just use Theme directly — it's auto-visible
```

No `qmldir` needed. `qt_add_qml_module` generates it from `QML_FILES`.

---

## C++↔QML boundary

`main.cpp` creates one `SimController` instance and exposes it via context property:

```cpp
SimController controller;
engine.rootContext()->setContextProperty("controller", &controller);
```

This is simpler than `QML_SINGLETON` for a single-binary app with one controller
instance. Context properties are deprecated in Qt 7 but fine for Qt 6.

`SimController` exposes `Q_PROPERTY` values and `Q_INVOKABLE` methods. No
business logic in QML — QML only reads properties and calls invokables.

---

## Controller surface (Q_PROPERTYs + invokables)

All properties notify via a single `stateUpdated()` signal (batch update per
tick). This is what the old `SchedulerController` did; keep the same pattern.

```cpp
// Properties (all NOTIFY stateUpdated)
int          tick
QVariantMap  cpu          // {pid, remaining, quantum_used, quantum_max, queue}
QVariantList highQueue    // [{pid, remaining}]
QVariantList lowQueue
QVariantList diskQueue    // [{pid, io_remaining}]
QVariantList tapeQueue
QVariantList printerQueue
QVariantList finished     // [{pid, arrival_tick, finish_tick, ...stats}]
QVariantMap  stats        // {cpu_utilization, throughput, avg_turnaround, ...}
QVariantList events       // [{type, pid, ...}]  — from snapshot.events
QVariantList prevEvents   // previous tick's events[] — a preempted process only
                          // lands in the low queue one tick after its event,
                          // so its queue highlight is driven by prevEvents
QString      rawSnapshot  // last raw JSON line — for InspectorPanel
bool         done         // snapshot.done — the sim finished (independent of the socket)

// Histories (for sparklines and Gantt)
QVariantList cpuHistory        // [pid] per tick (0 = idle)
QVariantList ganttBlocks       // [{pid, start, end}]
QVariantList allProcesses      // [{pid, label}] in stable order
int          totalTicks
QVariantList utilHistory
QVariantList turnaroundHistory
QVariantList throughputHistory

// Connection state (NOTIFY connectionChanged)
// Socket state only — "sim finished" is the `done` property above. Conflating
// the two (the old simDone) broke the status chip and the reset flow.
bool connected

// Launch state (NOTIFY launchStateChanged)
bool        needsLaunch
bool        launching
```

Key difference from old controller: **no `m_prev*` diff fields**. The
`events[]` array in the CuteSim snapshot already encodes all transitions
(`arrived`, `scheduled`, `preempted`, `io_start`, `io_return`, `completed`).
`SimController` reads them directly — no heuristic diffs.

Invokables:

```cpp
Q_INVOKABLE void step();
Q_INVOKABLE void run(int n);
Q_INVOKABLE void runAll();
Q_INVOKABLE void reconfigure();          // back to launch overlay
Q_INVOKABLE void launch(QVariantMap);   // LaunchOverlay → controller
Q_INVOKABLE void openTrace(QString);    // T9: load --emit-file output
```

Two connection modes:
- **TCP** (`--serve`): `SimClient` holds a `QTcpSocket`.
- **File** (`openTrace`): reads JSON Lines from disk tick by tick. Same
  parsing path — `SimController` doesn't know which mode.

---

## Naming

| Thing | Convention | Example |
|---|---|---|
| QML files | PascalCase | `CPUView.qml`, `GlassCard.qml` |
| QML ids | camelCase | `id: cpuView` |
| C++ classes | PascalCase | `SimController`, `SimClient` |
| C++ members | `m_camelCase` | `m_tick`, `m_highQueue` |
| JSON field names | snake_case | matches snapshot schema verbatim |
| All code | **English** | comments, labels, strings, signals |

Labels in QML that are user-visible in the UI (queue names, button text) are
in English. Portuguese strings from the old `scheduler-ui` are translated.

---

## F0a audit summary — what comes from the old scheduler-ui

Source: `PCB Scheduler CPU-3.zip` (redesign, already has `qml/components/`
structure) + `scheduler-ui/src/` (C++ side).

| File | Action | Note |
|---|---|---|
| `Theme.qml` | **Port 1:1** | Change import to `CuteSim.Viewer`; nothing else changes |
| `GlassCard.qml` | **Port 1:1** | Pure visual, no data |
| `Logo.qml` | **Port 1:1** | |
| `StatusChip.qml` | **Port 1:1** | |
| `TickChip.qml` | **Port 1:1** | |
| `CtrlButton.qml` | **Port 1:1** | |
| `SpinBox.qml` | **Port 1:1** | |
| `Sparkline.qml` | **Port 1:1** | |
| `Toast.qml` | **Port 1:1** | |
| `StatItem.qml` | **Port 1:1** | |
| `Header.qml` | **Port + translate** | English labels ("SCHEDULER" stays; "ROUND ROBIN · FEEDBACK" stays) |
| `CPUView.qml` | **Port + adapt** | Field names match new snapshot (`pid`, `remaining`, `quantum_used`, `quantum_max`, `queue`) |
| `QueueChip.qml` | **Port + adapt** | `io_remaining` field name |
| `QueueView.qml` | **Port + adapt** | `kind: "io"` → item field is `io_remaining`; CPU queues use `remaining` |
| `TimelineChart.qml` | **Port ~1:1** | Same interface: `blocks`, `processes`, `currentTick`, `totalTicks` |
| `StatsBar.qml` | **Port + adapt** | Stats field names: `cpu_utilization`, `throughput`, `avg_turnaround`, `avg_waiting`, `avg_response` |
| `FinishedTable.qml` | **Port + adapt** | Match snapshot `finished[]` fields |
| `LaunchOverlay.qml` | **Port + extend** | Translate labels; add `arrival_mode` selector (batch/bernoulli/poisson/uniform/geometric) + mode-specific field; add `.scn` file picker button |
| `Main.qml` | **Rewrite structure** | Queue binding names change; add `InspectorPanel` slot; new controller surface |
| `InspectorPanel.qml` | **New** | T7b: shows `controller.rawSnapshot` + `controller.events`; toggleable |
| `SchedulerClient` | **Port** | Rename to `SimClient`; same protocol (JSON Lines TCP) |
| `SchedulerController` | **Port + simplify** | Rename to `SimController`; drop `m_prev*`; add `rawSnapshot`; add file-mode path |

**Discarded from old scheduler-ui:**
- `ProcessDetail.qml` — not in CPU-3 redesign; not needed
- `mock_scheduler.py` — not needed (real binary)
- `setContextProperty` + old `addImportPath` wiring — replaced by `qt_add_qml_module`
- `.qrc` resource files — handled by CMake module
- Flat `qml/` (no subfolder) — already fixed in CPU-3

---

## F0b — Modern Qt6 QML module patterns (reference)

Key sources: Qt6 official examples, Qt Quick Controls, KDE/Kirigami.

**`qt_add_qml_module`** is the canonical way since Qt 6.2:
- Generates `qmldir` automatically from `QML_FILES`
- Embeds QML into the binary as resources (no loose files at runtime)
- Enables AOT compilation (`qmlcachegen`) without extra setup
- URI determines the import statement used in QML

**`QML_ELEMENT` / `QML_SINGLETON`** on C++ types:
- `QML_ELEMENT` — register a C++ class as a QML type automatically (no `qmlRegisterType` call needed)
- `QML_SINGLETON` — for C++ singleton types
- For **pure-QML singletons** (like Theme): `pragma Singleton` in the `.qml` file, list the file in `qt_add_qml_module` → engine handles it. No C++ needed.

**KDE/Kirigami patterns** (informational; not adopted now):
- ECM (Extra CMake Modules) for KDE-style builds — too heavy for a single app
- `contents/ui/` layout — convention for Plasma applets; not relevant here
- Relevant: KDE consistently uses `qt_add_qml_module` + `QML_ELEMENT` for their C++ types
- **Kirigami Addons Onboarding** (Sandro/KDE) — declarative guided-tour steps,
  per-widget highlight + blur, lifecycle hooks. API in refinement for an upcoming
  Kirigami Addons release. Tracked as TD3: add after the module ships.
  Ref: https://lnkd.in/dREnDYdj
