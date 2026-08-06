# CuteSim

[![CI](https://github.com/WillianPessoa/cutesim/actions/workflows/ci.yml/badge.svg)](https://github.com/WillianPessoa/cutesim/actions/workflows/ci.yml)

Queue-based process scheduler simulator: a Round Robin with Feedback engine
written as a C11 library, driven by a CLI app and visualized live by a
Qt 6 / QML viewer.

The scheduler runs two CPU queues (HIGH / LOW) with per-queue quantums:
a process that exhausts its quantum is demoted to LOW, and I/O completion
routes it back by device — disk returns to LOW, tape and printer to HIGH.
Workloads are either random (five arrival modes, sampled bursts) or fully
scripted through scenario files.

## A tour of CuteSim

<!--
  TODO: capture the images referenced below and drop them into
  screenshots/ (create the folder at the repo root — it is tracked,
  docs/ is not). Suggested size: full window, dark theme.
  Delete this comment when done.
-->

### 1. Launch a simulation

Opening the viewer lands on the launch screen. Pick one of two modes:

- **Random** — describe a workload: process count, RNG seed, the quantum
  of each CPU queue, service duration range, one of five arrival modes
  (batch, bernoulli, geometric, poisson, uniform), I/O probability per
  CPU tick, how I/O splits across disk/tape/printer, and each device's
  duration and service mode (concurrent or queued).
- **Scenario** — run a `.scn` file instead: one of the bundled presets
  shipped with the app, or your own file picked from disk. The file is
  validated inline by the same C parser the engine uses.

Pressing **LAUNCH** spawns a fresh `rr-feedback --serve` child on a
random TCP port and connects the viewer to it — the UI is a pure client;
every tick is computed by the C engine.

<!-- screenshots/launch-random.png: LaunchOverlay in random mode (the
     two-column WORKLOAD & SCHEDULING / IO form) -->
![Launch — random workload](screenshots/launch-random.png)

<!-- screenshots/launch-scenario.png: LaunchOverlay in scenario mode with
     the bundled presets listed -->
![Launch — scenario mode](screenshots/launch-scenario.png)

### 2. Watch the scheduler work

The dashboard shows one engine tick at a time, advanced with the step
control. Reading it top to bottom:

- **CPU** — the running process, its quantum usage (`N/M`) and remaining
  service time. When a process is preempted, finishes, or leaves for
  I/O, the slot shows a ghost tag for that tick (e.g. `→ LOW QUEUE`)
  so the transition is visible instead of a blank.
- **HIGH / LOW queues** — ready processes with their remaining burst.
  A process that exhausts its quantum drops from HIGH to LOW; newly
  enqueued processes flash highlighted.
- **I/O lanes** — one per device. Disk returns the process to the LOW
  queue; tape and printer return it to HIGH (that is the "feedback").
- **Gantt timeline** — per-process execution history, including idle
  ticks (I/O dispatch consumes the tick — Model A).
- **Stats bar** — live CPU utilization, throughput, and wait averages.

Clicking any process opens its detail view: an exact execution strip
plus the event log filtered to that PID.

<!-- screenshots/viewer-dashboard.png: main window mid-run — CPU view,
     HIGH/LOW queues, per-device I/O, Gantt timeline and stats bar -->
![Live dashboard](screenshots/viewer-dashboard.png)

<!-- screenshots/process-detail.png: ProcessDetail after clicking a
     process — execution strip + per-process event log -->
![Process detail](screenshots/process-detail.png)

### 3. Script your own scenario

The scenario editor builds `.scn` files without hand-editing: global
scheduling parameters on the left, scripted processes on the right.
Each process gets an arrival tick, a CPU burst, and an I/O timeline
written as `tick:device[:duration]` — the tick is relative to the
process's own CPU progress, so scripted I/O always fires. Everything is
validated live by the C parser; **SAVE & USE** feeds the file straight
back into the launch screen.

<!-- screenshots/scenario-editor.png: ScenarioEditor with globals on the
     left and a scripted process timeline on the right -->
![Scenario editor](screenshots/scenario-editor.png)

### 4. Inspect any tick, even past ones

The inspector panel shows the raw snapshot JSON and the launch
configuration of the running simulation. Every tick is recorded, so the
`‹ ›` controls time-travel through history — the events and snapshot
follow the tick being viewed, and the LIVE chip jumps back to the
present.

<!-- screenshots/inspector-timetravel.png: InspectorPanel browsing a
     recorded tick (‹ › navigation, tick N/M, LIVE chip) -->
![Inspector — time travel](screenshots/inspector-timetravel.png)

### 5. Or skip the UI entirely

The CLI runs the same engine standalone: `--trace` prints every tick —
CPU occupancy, both queues, I/O lanes and the tick's event log — and a
final statistics summary.

<!-- screenshots/cli-trace.png: terminal running
     ./build/apps/rr-feedback/rr-feedback scenarios/03-scripted-showcase.scn --trace
     (or replace the image with a fenced text block of the real output) -->
![CLI trace](screenshots/cli-trace.png)

## Layout

| Path | What it is |
|---|---|
| `src/`, `include/cutesim/` | simulation engine — C11 static library |
| `apps/rr-feedback/` | CLI simulator (trace, statistics, scenario files, TCP serve mode) |
| `apps/viewer/` | Qt 6 QML viewer (connects to `rr-feedback --serve`) |
| `apps/common/` | snapshot serializer + file/TCP emit backends |
| `scenarios/` | bundled preset scenarios shipped with the viewer |
| `examples/` | sample scenario files |
| `tests/` | GoogleTest + QML QuickTest suites (ctest) |

## Building

Requirements: CMake ≥ 3.25, a C11/C++20 toolchain, Qt ≥ 6.5 for the viewer.
Qt is located through `CMAKE_PREFIX_PATH` (set it in your environment if Qt
is not installed system-wide).

```sh
cmake --preset debug        # or: asan (ASan + UBSan)
cmake --build build
ctest --preset debug
```

To build only the C library and CLI on a machine without Qt:

```sh
cmake --preset debug -DCUTESIM_VIEWER=OFF
```

## Running

```sh
./build/apps/rr-feedback/rr-feedback --trace              # random workload
./build/apps/rr-feedback/rr-feedback scenarios/03-scripted-showcase.scn --trace
./build/apps/viewer/cutesim_viewer                        # launches rr-feedback itself
```

## Tooling

- `cmake --build build --target format` — clang-format + qmlformat in place
- `scripts/install-hooks.sh` — pre-commit format check
- `scripts/lint-cxx.sh` — clang-tidy over the project TUs (clazy opt-in)
