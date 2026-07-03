import QtQuick
import QtTest
import CuteSim.Viewer

Item {
    width: 640; height: 800

    /* ── TickChip ──────────────────────────────────────────────────────── */
    TestCase {
        name: "TickChip"
        width: 300; height: 60

        function test_zero_padded() {
            var c = createTemporaryObject(tickChipComp, this, { tick: 0 })
            verify(c !== null, "TickChip creation failed")
            var num = findChild(c, "num")
            if (num) compare(num.text, "000")
            else skip("num id not found in TickChip")
        }

        function test_value_padded() {
            var c = createTemporaryObject(tickChipComp, this, { tick: 42 })
            var num = findChild(c, "num")
            if (num) compare(num.text, "042")
            else skip("num id not found")
        }

        function test_large_value() {
            var c = createTemporaryObject(tickChipComp, this, { tick: 1234 })
            verify(c !== null)
            var num = findChild(c, "num")
            if (num) verify(num.text.length >= 4)
        }

        Component { id: tickChipComp; TickChip {} }
    }

    /* ── StatusChip ───────────────────────────────────────────────────── */
    TestCase {
        name: "StatusChip"
        width: 300; height: 60

        function test_connected_label() {
            var c = createTemporaryObject(statusComp, this, { status: "connected" })
            verify(c !== null)
            compare(c.statusLabel, "connected")
        }

        function test_disconnected_label() {
            var c = createTemporaryObject(statusComp, this, { status: "disconnected" })
            compare(c.statusLabel, "disconnected")
        }

        function test_done_label() {
            var c = createTemporaryObject(statusComp, this, { status: "done" })
            compare(c.statusLabel, "finished")
        }

        function test_breathe_when_connected() {
            var c = createTemporaryObject(statusComp, this, { status: "connected" })
            compare(c.breathe, true)
        }

        function test_no_breathe_when_done() {
            var c = createTemporaryObject(statusComp, this, { status: "done" })
            compare(c.breathe, false)
        }

        Component { id: statusComp; StatusChip {} }
    }

    /* ── CPUView ──────────────────────────────────────────────────────── */
    TestCase {
        name: "CPUView"
        width: 400; height: 220

        function test_idle_by_default() {
            var c = createTemporaryObject(cpuComp, this)
            verify(c !== null)
            compare(c.isIdle, true)
        }

        function test_active_high_queue() {
            var c = createTemporaryObject(cpuComp, this, {
                cpu: { pid: 3, remaining: 5, quantum_used: 2, quantum_max: 3, queue: "high" }
            })
            compare(c.isIdle,            false)
            compare(c.fromHigh,          true)
            compare(c.displayQuantumUsed, 2)
            compare(c.displayQuantumMax,  3)
        }

        function test_active_low_queue() {
            var c = createTemporaryObject(cpuComp, this, {
                cpu: { pid: 1, remaining: 2, quantum_used: 1, quantum_max: 6, queue: "low" }
            })
            compare(c.fromHigh, false)
        }

        function test_pid_zero_is_not_idle() {
            // pid 0 is a valid pid; idleness means "no pid field"
            var c = createTemporaryObject(cpuComp, this, {
                cpu: { pid: 0, remaining: 1, quantum_used: 1, quantum_max: 3, queue: "high" }
            })
            compare(c.isIdle, false)
        }

        function test_ghost_preempted_label() {
            var c = createTemporaryObject(cpuComp, this, {
                cpu: { pid: 2, remaining: 0, quantum_used: 3, quantum_max: 3, queue: "high" },
                ghost: "preempted"
            })
            compare(c.ghostLabel, "→ low queue")
            compare(c.displayQuantumUsed, 3)
        }

        function test_ghost_completed_label() {
            var c = createTemporaryObject(cpuComp, this, {
                cpu: { pid: 2, remaining: 1, quantum_used: 2, quantum_max: 3, queue: "low" },
                ghost: "completed"
            })
            compare(c.ghostLabel, "finished ✓")
        }

        function test_no_ghost_no_label() {
            var c = createTemporaryObject(cpuComp, this)
            compare(c.ghostLabel, "")
            compare(c.displayQuantumUsed, 0)
            compare(c.displayQuantumMax,  1)
        }

        function test_ghost_io_labels() {
            // BUG-16: process departing for I/O is tagged with its device queue
            var c = createTemporaryObject(cpuComp, this, {
                cpu: { pid: 2, remaining: 2, quantum_used: 1, quantum_max: 3, queue: "high" },
                ghost: "io_disk"
            })
            compare(c.ghostLabel, "→ disk queue")
            c.ghost = "io_tape"
            compare(c.ghostLabel, "→ tape queue")
            c.ghost = "io_printer"
            compare(c.ghostLabel, "→ printer queue")
        }

        Component { id: cpuComp; CPUView {} }
    }

    /* ── QueueView ────────────────────────────────────────────────────── */
    TestCase {
        name: "QueueView"
        width: 300; height: 120

        function test_empty_by_default() {
            var c = createTemporaryObject(queueComp, this)
            compare(c.items.length, 0)
        }

        function test_items_count() {
            var c = createTemporaryObject(queueComp, this, {
                items: [{ pid: 1, remaining: 4 }, { pid: 2, remaining: 2 }]
            })
            compare(c.items.length, 2)
        }

        function test_kind_default_cpu() {
            var c = createTemporaryObject(queueComp, this)
            compare(c.kind, "cpu")
        }

        Component { id: queueComp; QueueView {} }
    }

    /* ── CtrlButton ───────────────────────────────────────────────────── */
    TestCase {
        name: "CtrlButton"
        width: 200; height: 60

        function test_enabled_by_default() {
            var c = createTemporaryObject(btnComp, this, { label: "Step" })
            compare(c.enabledState, true)
        }

        function test_signal_connectable() {
            var c = createTemporaryObject(btnComp, this, { label: "Step" })
            var count = 0
            var fn = function() { count++ }
            c.clicked.connect(fn)
            c.clicked()
            compare(count, 1)
        }

        function test_disabled_state() {
            var c = createTemporaryObject(btnComp, this,
                { label: "Step", enabledState: false })
            compare(c.enabledState, false)
        }

        Component { id: btnComp; CtrlButton {} }
    }

    /* ── SpinBox ──────────────────────────────────────────────────────── */
    TestCase {
        name: "SpinBox"
        width: 200; height: 60

        function test_initial_value() {
            var c = createTemporaryObject(spinComp, this, { value: 7 })
            compare(c.value, 7)
        }

        function test_minimum_value() {
            var c = createTemporaryObject(spinComp, this,
                { value: 0, minimumValue: 0, maximumValue: 10 })
            compare(c.value, 0)
        }

        function test_maximum_value() {
            var c = createTemporaryObject(spinComp, this,
                { value: 10, minimumValue: 0, maximumValue: 10 })
            compare(c.value, 10)
        }

        Component { id: spinComp; SpinBox {} }
    }

    /* ── LaunchOverlay ────────────────────────────────────────────────── */
    TestCase {
        name: "LaunchOverlay"
        width: 640; height: 700

        function test_default_processes() {
            var c = createTemporaryObject(overlayComp, this)
            compare(c.processes, 5)
        }

        function test_default_quantum_hi() {
            var c = createTemporaryObject(overlayComp, this)
            compare(c.quantumHi, 3)
        }

        function test_default_quantum_lo() {
            var c = createTemporaryObject(overlayComp, this)
            compare(c.quantumLo, 6)
        }

        function test_default_pio() {
            var c = createTemporaryObject(overlayComp, this)
            compare(c.pIo, 0)
        }

        function test_default_service_range() {
            var c = createTemporaryObject(overlayComp, this)
            compare(c.serviceMin, 5)
            compare(c.serviceMax, 15)
        }

        function test_default_seed() {
            var c = createTemporaryObject(overlayComp, this)
            compare(c.seed, 42)
        }

        function test_not_launching_initially() {
            var c = createTemporaryObject(overlayComp, this)
            compare(c.launching, false)
        }

        function test_default_mode_random() {
            var c = createTemporaryObject(overlayComp, this)
            compare(c.mode, "random")
            compare(c.canLaunch, true)
        }

        function test_default_arrival_and_split() {
            var c = createTemporaryObject(overlayComp, this)
            compare(c.arrivalMode, "batch")
            compare(c.pDisk, 34)
            compare(c.pTape, 33)
            compare(c.pPrinter, 33)
        }

        function test_printer_share_is_remainder() {
            var c = createTemporaryObject(overlayComp, this, { pDisk: 60, pTape: 30 })
            compare(c.pPrinter, 10)
        }

        function test_scenario_mode_without_file_blocks_launch() {
            var c = createTemporaryObject(overlayComp, this, { mode: "scenario" })
            compare(c.canLaunch, false)
        }

        function test_build_params_random_carries_all_keys() {
            var c = createTemporaryObject(overlayComp, this, {
                pIo: 40, arrivalMode: "poisson", arrivalLambdaPct: 75,
                diskMode: "queue"
            })
            var p = c.buildParams()
            compare(p.pIo, 40)
            compare(p.arrivalMode, "poisson")
            compare(p.arrivalLambda, 0.75)
            compare(p.diskMode, "queue")
            compare(p.scenarioFile, undefined)
        }

        function test_build_params_scenario_carries_file_only() {
            var c = createTemporaryObject(overlayComp, this)
            c.mode = "scenario"
            c.scenarioPath = "/tmp/x.scn"
            var p = c.buildParams()
            compare(p.scenarioFile, "/tmp/x.scn")
            compare(p.processes, undefined)
        }

        function test_apply_params_restores_state() {
            var c = createTemporaryObject(overlayComp, this)
            c.applyParams({ processes: 9, quantumHi: 4, arrivalMode: "uniform",
                            arrivalInterval: 7, pDisk: 40, pTape: 40,
                            diskMode: "queue", arrivalLambda: 1.25 })
            compare(c.processes, 9)
            compare(c.quantumHi, 4)
            compare(c.arrivalMode, "uniform")
            compare(c.arrivalInterval, 7)
            compare(c.pDisk, 40)
            compare(c.pPrinter, 20)
            compare(c.diskMode, "queue")
            compare(c.arrivalLambdaPct, 125)
            compare(c.mode, "random")
        }

        function test_apply_params_scenario_restores_mode() {
            var c = createTemporaryObject(overlayComp, this)
            c.applyParams({ scenarioFile: "/tmp/x.scn" })
            compare(c.mode, "scenario")
            compare(c.scenarioPath, "/tmp/x.scn")
        }

        function test_bundled_empty_without_bridge() {
            var c = createTemporaryObject(overlayComp, this)
            compare(c.bundled.length, 0)
        }

        Component { id: overlayComp; LaunchOverlay { visible: false } }
    }

    /* ── SegmentControl ───────────────────────────────────────────────── */
    TestCase {
        name: "SegmentControl"
        width: 300; height: 40

        function test_value_and_signal() {
            var c = createTemporaryObject(segComp, this, {
                options: ["a", "b"], value: "a"
            })
            var got = ""
            c.selected.connect(function(v) { got = v })
            compare(c.value, "a")
            c.value = "b"       // programmatic change does not emit
            compare(got, "")
        }

        Component { id: segComp; SegmentControl {} }
    }

    /* ── ScenarioEditor ───────────────────────────────────────────────── */
    TestCase {
        name: "ScenarioEditor"
        width: 900; height: 700

        function test_default_scn_has_globals_and_random_block() {
            var c = createTemporaryObject(editorComp, this)
            var t = c.toScn()
            verify(t.indexOf("quantum-hi = 3") >= 0)
            verify(t.indexOf("quantum-lo = 6") >= 0)
            verify(t.indexOf("seed = 42") >= 0)
            verify(t.indexOf("disk-duration = 5") >= 0)
            verify(t.indexOf("tape-duration = 8") >= 0)
            verify(t.indexOf("printer-duration = 12") >= 0)
            verify(t.indexOf("process-count = 5") >= 0)
            verify(t.indexOf("p-printer = 33") >= 0)
            verify(t.indexOf("[process]") < 0)
        }

        function test_scripted_scn_omits_random_block() {
            var c = createTemporaryObject(editorComp, this)
            c.addProcess()
            var t = c.toScn()
            verify(t.indexOf("[process]") >= 0)
            verify(t.indexOf("arrival = 0") >= 0)
            verify(t.indexOf("burst = 10") >= 0)
            verify(t.indexOf("process-count") < 0)
            verify(t.indexOf("p-io") < 0)
        }

        function test_duration_range_serialization() {
            var c = createTemporaryObject(editorComp, this, { diskMin: 2, diskMax: 6 })
            compare(c.durStr(2, 6), "2-6")
            compare(c.durStr(4, 4), "4")
            verify(c.toScn().indexOf("disk-duration = 2-6") >= 0)
        }

        function test_no_bridge_no_validation() {
            var c = createTemporaryObject(editorComp, this)
            compare(c.validation, null)
            compare(c.valid, false)
        }

        Component { id: editorComp; ScenarioEditor { visible: false } }
    }

    /* ── InspectorPanel ───────────────────────────────────────────────── */
    TestCase {
        name: "InspectorPanel"
        width: 320; height: 400

        function test_empty_events_by_default() {
            var c = createTemporaryObject(inspComp, this)
            compare(c.events.length, 0)
        }

        function test_raw_snapshot_property() {
            var snap = '{"tick":7,"done":false}'
            var c = createTemporaryObject(inspComp, this, { rawSnapshot: snap })
            compare(c.rawSnapshot, snap)
        }

        function test_multiple_events() {
            var c = createTemporaryObject(inspComp, this, {
                events: [
                    { type: "arrived",   pid: 1 },
                    { type: "scheduled", pid: 1, queue: "high" },
                    { type: "preempted", pid: 1, quantum_used: 3, quantum_max: 3 }
                ]
            })
            compare(c.events.length, 3)
        }

        function test_pretty_snapshot_indented() {
            // BUG-17: raw JSON is rendered pretty-printed, not as one line
            var c = createTemporaryObject(inspComp, this, {
                rawSnapshot: '{"tick":7,"cpu":{"pid":1},"done":false}'
            })
            verify(c.prettySnapshot.indexOf("\n") >= 0, "expected newlines")
            verify(c.prettySnapshot.indexOf('  "tick": 7') >= 0, "expected 2-space indent")
        }

        function test_pretty_snapshot_invalid_json_falls_back() {
            var c = createTemporaryObject(inspComp, this, { rawSnapshot: "not json" })
            compare(c.prettySnapshot, "not json")
        }

        function test_pretty_snapshot_empty() {
            var c = createTemporaryObject(inspComp, this)
            compare(c.prettySnapshot, "")
        }

        function test_config_text_empty_without_params() {
            var c = createTemporaryObject(inspComp, this)
            compare(c.configText, "")
            compare(c.view, "snapshot")
        }

        function test_config_text_random_lists_options() {
            var c = createTemporaryObject(inspComp, this, { params: {
                processes: 7, seed: 99, quantumHi: 3, quantumLo: 6,
                pIo: 25, serviceMin: 5, serviceMax: 15,
                arrivalMode: "bernoulli", arrivalRate: 20,
                pDisk: 50, pTape: 30,
                diskMin: 3, diskMax: 5, tapeMin: 8, tapeMax: 8,
                printerMin: 12, printerMax: 12,
                diskMode: "queue", tapeMode: "concurrent", printerMode: "concurrent"
            } })
            verify(c.configText.indexOf("random workload") >= 0)
            verify(c.configText.indexOf("7") >= 0, "process count shown")
            verify(c.configText.indexOf("bernoulli — 20% per tick") >= 0)
            verify(c.configText.indexOf("3-5t · queue") >= 0, "disk duration+mode")
            verify(c.configText.indexOf("printer 20") >= 0, "printer share is remainder")
        }

        function test_config_text_scenario_shows_path_without_bridge() {
            var c = createTemporaryObject(inspComp, this, {
                params: { scenarioFile: "/tmp/x.scn" }
            })
            verify(c.configText.indexOf("scenario file") >= 0)
            verify(c.configText.indexOf("/tmp/x.scn") >= 0)
            verify(c.configText.indexOf("cannot read file") >= 0)
        }

        function test_live_by_default_shows_current_snapshot() {
            var c = createTemporaryObject(inspComp, this, {
                rawSnapshot: '{"tick":3}',
                rawHistory: ['{"tick":1}', '{"tick":2}', '{"tick":3}']
            })
            compare(c.live, true)
            compare(c.shownRaw, '{"tick":3}')
        }

        function test_step_back_browses_history() {
            var c = createTemporaryObject(inspComp, this, {
                rawSnapshot: '{"tick":3}',
                events: [ { type: "completed", pid: 3 } ],
                rawHistory: ['{"tick":1}', '{"tick":2}', '{"tick":3}'],
                eventsHistory: [ [ { type: "arrived", pid: 1 } ], [], [] ]
            })
            c.stepBack()   // from live → previous recorded tick
            compare(c.live, false)
            compare(c.shownIndex, 1)
            compare(c.shownRaw, '{"tick":2}')
            c.stepBack()
            compare(c.shownIndex, 0)
            compare(c.shownEvents.length, 1)
            compare(c.shownEvents[0].type, "arrived")
            c.stepBack()   // clamps at the first recorded tick
            compare(c.shownIndex, 0)
        }

        function test_step_forward_returns_to_live() {
            var c = createTemporaryObject(inspComp, this, {
                rawSnapshot: '{"tick":2}',
                rawHistory: ['{"tick":1}', '{"tick":2}']
            })
            c.stepBack()
            compare(c.shownRaw, '{"tick":1}')
            c.stepForward()
            compare(c.live, false)
            compare(c.shownRaw, '{"tick":2}')
            c.stepForward()   // past the end → back to live
            compare(c.live, true)
        }

        function test_go_live() {
            var c = createTemporaryObject(inspComp, this, {
                rawSnapshot: '{"tick":2}',
                rawHistory: ['{"tick":1}', '{"tick":2}']
            })
            c.stepBack()
            compare(c.live, false)
            c.goLive()
            compare(c.live, true)
            compare(c.shownRaw, '{"tick":2}')
        }

        Component { id: inspComp; InspectorPanel {} }
    }

    /* ── FinishedTable ────────────────────────────────────────────────── */
    TestCase {
        name: "FinishedTable"
        width: 900; height: 300

        function test_wide_table_needs_no_hscroll() {
            var c = createTemporaryObject(tableComp, this, { width: 900 })
            compare(c.needsHScroll, false)
            compare(c.hx, 0)
        }

        function test_narrow_table_scrolls_instead_of_overflowing() {
            var c = createTemporaryObject(tableComp, this, { width: 500 })
            compare(c.needsHScroll, true)
            verify(c.scrollWidth > c.hViewport)
            // PID stays pinned regardless of scroll state
            compare(c.pidColWidth, 48)
        }

        function test_all_columns_always_present() {
            var c = createTemporaryObject(tableComp, this, { width: 500 })
            compare(c.cols.length, 11)   // + pinned PID = 12 columns total
        }

        Component { id: tableComp; FinishedTable {} }
    }

    /* ── ProcessDetail ────────────────────────────────────────────────── */
    TestCase {
        name: "ProcessDetail"
        width: 320; height: 500

        // pid 1 lifecycle: arrive+run high → depart for disk I/O → wait →
        // return to low queue → run low → complete. pid 2 noise is filtered.
        readonly property var sampleHistory: [
            [ { type: "arrived",   pid: 1 },
              { type: "scheduled", pid: 1, queue: "high" },
              { type: "arrived",   pid: 2 } ],                                  // t1
            [ { type: "io_start",  pid: 1, device: "disk", io_remaining: 2 },
              { type: "scheduled", pid: 2, queue: "high" } ],                   // t2
            [ { type: "io_tick",   pid: 1, device: "disk", remaining: 1 } ],    // t3
            [ { type: "io_return", pid: 1, queue: "low" } ],                    // t4
            [ { type: "scheduled", pid: 1, queue: "low" } ],                    // t5
            [ { type: "completed", pid: 1 } ]                                   // t6
        ]

        function makeDetail(pid) {
            return createTemporaryObject(detailComp, this, {
                selectedPid: pid,
                process: { pid: pid, status: "done" },
                eventsHistory: sampleHistory
            })
        }

        function test_no_pid_by_default() {
            var c = createTemporaryObject(detailComp, this)
            compare(c.hasPid, false)
            compare(c.stripData.length, 0)
            compare(c.pidEvents.length, 0)
        }

        function test_strip_states_follow_events() {
            var c = makeDetail(1)
            // t1 cpu-high, t2 io-disk (no CPU tick), t3 io-disk carry,
            // t4 low queue, t5 cpu-low, t6 cpu-low (completion tick ran)
            compare(JSON.stringify(c.stripData), JSON.stringify([1, 5, 5, 4, 2, 2]))
        }

        function test_events_filtered_by_pid_without_io_ticks() {
            var c = makeDetail(1)
            compare(c.pidEvents.length, 6)  // io_tick excluded
            compare(c.pidEvents[0].tick, 1)
            compare(c.pidEvents[0].type, "arrived")
            compare(c.pidEvents[5].tick, 6)
            compare(c.pidEvents[5].type, "completed")
        }

        function test_other_pid_sees_own_events() {
            var c = makeDetail(2)
            compare(c.pidEvents.length, 2)  // arrived + scheduled only
            compare(JSON.stringify(c.stripData), JSON.stringify([3, 1, 1, 1, 1, 1]))
        }

        function test_event_labels() {
            var c = makeDetail(1)
            compare(c.eventLabel({ type: "io_start", device: "disk", io_remaining: 2 }),
                    "→ i/o disk (2t)")
            compare(c.eventLabel({ type: "preempted", quantum_used: 3, quantum_max: 3 }),
                    "preempted 3/3 → low queue")
            compare(c.eventLabel({ type: "completed" }), "completed ✓")
        }

        Component { id: detailComp; ProcessDetail {} }
    }
}
