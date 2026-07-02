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

        function test_flash_when_idle() {
            // flash overrides display values when CPU is idle
            var c = createTemporaryObject(cpuComp, this, {
                flash: { pid: 2, used: 3, max: 3 }
            })
            compare(c.displayQuantumUsed, 3)
            compare(c.displayQuantumMax,  3)
        }

        function test_no_flash_no_cpu() {
            var c = createTemporaryObject(cpuComp, this)
            compare(c.displayQuantumUsed, 0)
            compare(c.displayQuantumMax,  1)
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

        Component { id: overlayComp; LaunchOverlay { visible: false } }
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

        Component { id: inspComp; InspectorPanel {} }
    }
}
