#include <gtest/gtest.h>

extern "C" {
#include "cutesim/simulation.h"
}

#include "../test_describe.h"

/* Quanta large enough that I/O fires before the process is preempted. */
static const int LARGE_QUANTUM_HI = 10;
static const int LARGE_QUANTUM_LO = 20;
static const int RNG_SEED         = 42;
static const int IO_FIRES_AT_TICK = 1;

static SimConfig test_config(void) {
    SimConfig cfg    = {};
    cfg.quantum_hi   = LARGE_QUANTUM_HI;
    cfg.quantum_lo   = LARGE_QUANTUM_LO;
    cfg.seed         = RNG_SEED;
    cfg.p_io         = 0;
    cfg.arrival_rate = 0;
    return cfg;
}

// ---------------------------------------------------------------------------
// I/O trigger
// ---------------------------------------------------------------------------

TEST(IoTrigger, ScriptedIoMovesProcessToDeviceQueue) {
    DESCRIBE("a process with a scripted disk I/O at tick 1 enters disk_queue on that tick");
    static const int DISK_DURATION = 3;
    SimConfig cfg                  = test_config();
    cfg.disk_duration              = { DISK_DURATION, DISK_DURATION };
    Simulation *s                  = sim_create(cfg);

    Process *p       = process_create(1, 0, 0);
    ScriptedIO ev    = { IO_FIRES_AT_TICK, DEVICE_DISK };
    p->io_script     = &ev;
    p->io_script_len = 1;

    sim_add_process(s, p);
    sim_run(s, IO_FIRES_AT_TICK + 1); /* tick 0: runs; tick 1: I/O fires */

    EXPECT_EQ(queue_size(&s->disk_queue), 1);
    EXPECT_EQ(s->running, nullptr);
    EXPECT_EQ(p->status, PROC_BLOCKED);

    p->io_script = nullptr; /* stack-allocated, don't free */
    sim_destroy(s);
}

TEST(IoTrigger, IoRemainingSetOnTrigger) {
    DESCRIBE("io_remaining is set to the device duration when I/O fires");
    static const int DISK_DURATION = 5;
    SimConfig cfg                  = test_config();
    cfg.disk_duration              = { DISK_DURATION, DISK_DURATION };
    Simulation *s                  = sim_create(cfg);

    Process *p       = process_create(1, 0, 0);
    ScriptedIO ev    = { IO_FIRES_AT_TICK, DEVICE_DISK };
    p->io_script     = &ev;
    p->io_script_len = 1;

    sim_add_process(s, p);
    sim_run(s, IO_FIRES_AT_TICK + 1);

    EXPECT_EQ(p->io_remaining, DISK_DURATION);

    p->io_script = nullptr;
    sim_destroy(s);
}

TEST(ScriptedIo, TriggerIsRelativeToCpuService) {
    DESCRIBE("scripted I/O fires after the process accrues service_tick CPU ticks, "
             "independent of the global clock");
    SimConfig cfg     = test_config();
    cfg.disk_duration = { 3, 3 };
    Simulation *s     = sim_create(cfg);

    /* Arrives at global tick 3, so the global clock never equals the service tick. */
    Process *p         = process_create(1, /*arrival*/ 3, /*seq*/ 0);
    p->cpu_burst_total = 5;
    ScriptedIO ev      = { /*service_tick*/ 2, DEVICE_DISK };
    p->io_script       = &ev;
    p->io_script_len   = 1;

    sim_add_process(s, p);
    sim_run(s, 6); /* ticks 3,4 run (cpu_ticks 1,2); tick 5 fires the I/O */

    EXPECT_EQ(p->io_count, 1);
    EXPECT_EQ(p->status, PROC_BLOCKED);
    EXPECT_EQ(queue_size(&s->disk_queue), 1);

    p->io_script = nullptr; /* stack-allocated */
    sim_destroy(s);
}

TEST(ScriptedIo, DurationOverrideIgnoresGlobal) {
    DESCRIBE("a scripted I/O carrying an explicit duration overrides the global device duration");
    SimConfig cfg     = test_config();
    cfg.disk_duration = { 2, 2 }; /* global disk duration = 2 */
    Simulation *s     = sim_create(cfg);

    Process *p         = process_create(1, /*arrival*/ 0, /*seq*/ 0);
    p->cpu_burst_total = 5;
    ScriptedIO ev      = { /*service_tick*/ 1, DEVICE_DISK };
    ev.has_duration    = 1;
    ev.duration        = { 7, 7 }; /* override = 7, not the global 2 */
    p->io_script       = &ev;
    p->io_script_len   = 1;

    sim_add_process(s, p);
    sim_run(s, 2); /* tick 0 runs (cpu_ticks 1); tick 1 fires the I/O */

    EXPECT_EQ(p->io_remaining, 7); /* uses the override, not the global 2 */

    p->io_script = nullptr;
    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// I/O execution mode — CONCURRENT
// ---------------------------------------------------------------------------

TEST(ConcurrentIo, BothProcessesDecrementEachTick) {
    DESCRIBE(
        "in CONCURRENT mode every process in the device queue decrements io_remaining each tick");
    static const int DISK_DURATION = 3;
    SimConfig cfg                  = test_config();
    cfg.disk_duration              = { DISK_DURATION, DISK_DURATION };
    cfg.io_mode_disk               = IO_MODE_CONCURRENT;
    Simulation *s                  = sim_create(cfg);

    /* Two processes both arrive and immediately go to disk_queue */
    Process *p1 = process_create(1, 0, 0);
    Process *p2 = process_create(2, 0, 1);
    /* White-box staging: place the processes directly in the BLOCKED state they
       would hold while sitting in a device queue (READY->BLOCKED is not a valid
       transition, so the status field is set directly, like io_remaining below). */
    p1->status       = PROC_BLOCKED;
    p2->status       = PROC_BLOCKED;
    p1->io_remaining = DISK_DURATION;
    p2->io_remaining = DISK_DURATION;
    queue_enqueue(&s->disk_queue, p1);
    queue_enqueue(&s->disk_queue, p2);
    /* Register so sim_destroy frees them */
    sim_add_process(s, p1);
    sim_add_process(s, p2);
    /* Remove from pending (they're already in disk_queue) */
    s->pending_count = 0;

    sim_step(s); /* one tick: both should decrement */

    EXPECT_EQ(p1->io_remaining, DISK_DURATION - 1);
    EXPECT_EQ(p2->io_remaining, DISK_DURATION - 1);

    sim_destroy(s);
}

TEST(ConcurrentIo, BothProcessesCompleteAtSameTick) {
    DESCRIBE("in CONCURRENT mode two processes with the same duration complete simultaneously");
    static const int DISK_DURATION = 2;
    SimConfig cfg                  = test_config();
    cfg.disk_duration              = { DISK_DURATION, DISK_DURATION };
    cfg.io_mode_disk               = IO_MODE_CONCURRENT;
    Simulation *s                  = sim_create(cfg);

    Process *p1 = process_create(1, 0, 0);
    Process *p2 = process_create(2, 0, 1);
    /* White-box staging: place the processes directly in the BLOCKED state they
       would hold while sitting in a device queue (READY->BLOCKED is not a valid
       transition, so the status field is set directly, like io_remaining below). */
    p1->status       = PROC_BLOCKED;
    p2->status       = PROC_BLOCKED;
    p1->io_remaining = DISK_DURATION;
    p2->io_remaining = DISK_DURATION;
    queue_enqueue(&s->disk_queue, p1);
    queue_enqueue(&s->disk_queue, p2);
    sim_add_process(s, p1);
    sim_add_process(s, p2);
    s->pending_count = 0;

    sim_run(s, DISK_DURATION); /* after DISK_DURATION ticks both should have completed */

    EXPECT_EQ(queue_size(&s->disk_queue), 0);
    /* both return to lo_queue (disk → lo) */
    EXPECT_EQ(queue_size(&s->lo_queue), 2);

    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// I/O execution mode — QUEUE
// ---------------------------------------------------------------------------

TEST(QueueIo, OnlyHeadDecrementsEachTick) {
    DESCRIBE("in QUEUE mode only the head of the device queue has io_remaining decremented");
    static const int DISK_DURATION = 3;
    SimConfig cfg                  = test_config();
    cfg.disk_duration              = { DISK_DURATION, DISK_DURATION };
    cfg.io_mode_disk               = IO_MODE_QUEUE;
    Simulation *s                  = sim_create(cfg);

    Process *p1 = process_create(1, 0, 0);
    Process *p2 = process_create(2, 0, 1);
    /* White-box staging: place the processes directly in the BLOCKED state they
       would hold while sitting in a device queue (READY->BLOCKED is not a valid
       transition, so the status field is set directly, like io_remaining below). */
    p1->status       = PROC_BLOCKED;
    p2->status       = PROC_BLOCKED;
    p1->io_remaining = DISK_DURATION;
    p2->io_remaining = DISK_DURATION;
    queue_enqueue(&s->disk_queue, p1); /* p1 is head */
    queue_enqueue(&s->disk_queue, p2);
    sim_add_process(s, p1);
    sim_add_process(s, p2);
    s->pending_count = 0;

    sim_step(s);

    EXPECT_EQ(p1->io_remaining, DISK_DURATION - 1); /* head decremented */
    EXPECT_EQ(p2->io_remaining, DISK_DURATION);     /* second process unchanged */

    sim_destroy(s);
}

TEST(QueueIo, SecondProcessDelayedBehindFirst) {
    DESCRIBE("in QUEUE mode the second process starts decrementing only after the first completes");
    static const int DISK_DURATION = 2;
    SimConfig cfg                  = test_config();
    cfg.disk_duration              = { DISK_DURATION, DISK_DURATION };
    cfg.io_mode_disk               = IO_MODE_QUEUE;
    Simulation *s                  = sim_create(cfg);

    Process *p1 = process_create(1, 0, 0);
    Process *p2 = process_create(2, 0, 1);
    /* White-box staging: place the processes directly in the BLOCKED state they
       would hold while sitting in a device queue (READY->BLOCKED is not a valid
       transition, so the status field is set directly, like io_remaining below). */
    p1->status       = PROC_BLOCKED;
    p2->status       = PROC_BLOCKED;
    p1->io_remaining = DISK_DURATION;
    p2->io_remaining = DISK_DURATION;
    queue_enqueue(&s->disk_queue, p1);
    queue_enqueue(&s->disk_queue, p2);
    sim_add_process(s, p1);
    sim_add_process(s, p2);
    s->pending_count = 0;

    sim_run(s, DISK_DURATION); /* p1 completes; p2 hasn't started */

    EXPECT_EQ(queue_size(&s->disk_queue), 1);   /* only p2 remains */
    EXPECT_EQ(p2->io_remaining, DISK_DURATION); /* p2 not yet decremented */

    sim_run(s, DISK_DURATION); /* now p2 runs its ticks */
    EXPECT_EQ(queue_size(&s->disk_queue), 0);

    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// Per-device I/O tick counters
// ---------------------------------------------------------------------------

TEST(PerDeviceIo, DiskTicksCountedInDiskField) {
    DESCRIBE("io_ticks_disk is incremented each tick a process spends in disk_queue; "
             "tape and printer fields stay zero");
    static const int DISK_DURATION = 3;
    SimConfig cfg                  = test_config();
    cfg.disk_duration              = { DISK_DURATION, DISK_DURATION };
    cfg.io_mode_disk               = IO_MODE_CONCURRENT;
    Simulation *s                  = sim_create(cfg);

    Process *p       = process_create(1, 0, 0);
    p->status        = PROC_BLOCKED;
    p->io_remaining  = DISK_DURATION;
    queue_enqueue(&s->disk_queue, p);
    sim_add_process(s, p);
    s->pending_count = 0;

    sim_run(s, DISK_DURATION);

    EXPECT_EQ(p->io_ticks,         DISK_DURATION);
    EXPECT_EQ(p->io_ticks_disk,    DISK_DURATION);
    EXPECT_EQ(p->io_ticks_tape,    0);
    EXPECT_EQ(p->io_ticks_printer, 0);

    sim_destroy(s);
}

TEST(PerDeviceIo, TapeAndPrinterTicksCountedSeparately) {
    DESCRIBE("io_ticks_tape and io_ticks_printer are incremented for their respective queues");
    static const int TAPE_DURATION    = 2;
    static const int PRINTER_DURATION = 3;
    SimConfig cfg                     = test_config();
    cfg.tape_duration                 = { TAPE_DURATION, TAPE_DURATION };
    cfg.printer_duration              = { PRINTER_DURATION, PRINTER_DURATION };
    cfg.io_mode_tape                  = IO_MODE_CONCURRENT;
    cfg.io_mode_printer               = IO_MODE_CONCURRENT;
    Simulation *s                     = sim_create(cfg);

    Process *pt = process_create(1, 0, 0);
    Process *pp = process_create(2, 0, 1);
    pt->status        = PROC_BLOCKED;
    pp->status        = PROC_BLOCKED;
    pt->io_remaining  = TAPE_DURATION;
    pp->io_remaining  = PRINTER_DURATION;
    queue_enqueue(&s->tape_queue,    pt);
    queue_enqueue(&s->printer_queue, pp);
    sim_add_process(s, pt);
    sim_add_process(s, pp);
    s->pending_count = 0;

    sim_run(s, PRINTER_DURATION); /* long enough for both to finish */

    EXPECT_EQ(pt->io_ticks_disk,    0);
    EXPECT_EQ(pt->io_ticks_tape,    TAPE_DURATION);
    EXPECT_EQ(pt->io_ticks_printer, 0);

    EXPECT_EQ(pp->io_ticks_disk,    0);
    EXPECT_EQ(pp->io_ticks_tape,    0);
    EXPECT_EQ(pp->io_ticks_printer, PRINTER_DURATION);

    sim_destroy(s);
}
