#include <gtest/gtest.h>

extern "C" {
#include "cutesim/simulation.h"
}

#include "../test_describe.h"

static SimConfig test_config(void) {
    SimConfig cfg    = {};
    cfg.quantum_hi   = 10; /* large quantum so I/O fires before preemption */
    cfg.quantum_lo   = 20;
    cfg.seed         = 42;
    cfg.p_io         = 0;
    cfg.arrival_rate = 0;
    return cfg;
}

// ---------------------------------------------------------------------------
// I/O trigger
// ---------------------------------------------------------------------------

TEST(IoTrigger, ScriptedIoMovesProcessToDeviceQueue) {
    DESCRIBE("a process with a scripted disk I/O at tick 1 enters disk_queue on that tick");
    SimConfig cfg         = test_config();
    cfg.disk_duration     = { 3, 3 };
    Simulation *s         = sim_create(cfg);

    Process    *p         = process_create(1, 0, 0);
    ScriptedIO  ev        = { 1, DEVICE_DISK };
    p->io_script          = &ev;
    p->io_script_len      = 1;

    sim_add_process(s, p);
    sim_run(s, 2); /* tick 0: runs; tick 1: I/O fires */

    EXPECT_EQ(queue_size(&s->disk_queue), 1);
    EXPECT_EQ(s->running,                 nullptr);
    EXPECT_EQ(p->status,                  PROC_BLOCKED);

    p->io_script = nullptr; /* stack-allocated, don't free */
    sim_destroy(s);
}

TEST(IoTrigger, IoRemainingSetOnTrigger) {
    DESCRIBE("io_remaining is set to the device duration when I/O fires");
    SimConfig cfg     = test_config();
    cfg.disk_duration = { 5, 5 };
    Simulation *s     = sim_create(cfg);

    Process    *p     = process_create(1, 0, 0);
    ScriptedIO  ev    = { 1, DEVICE_DISK };
    p->io_script      = &ev;
    p->io_script_len  = 1;

    sim_add_process(s, p);
    sim_run(s, 2);

    EXPECT_EQ(p->io_remaining, 5);

    p->io_script = nullptr;
    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// I/O execution mode — CONCURRENT
// ---------------------------------------------------------------------------

TEST(ConcurrentIo, BothProcessesDecrementEachTick) {
    DESCRIBE("in CONCURRENT mode every process in the device queue decrements io_remaining each tick");
    SimConfig cfg     = test_config();
    cfg.disk_duration = { 3, 3 };
    cfg.io_mode_disk  = IO_MODE_CONCURRENT;
    Simulation *s     = sim_create(cfg);

    /* Two processes both arrive and immediately go to disk_queue */
    Process *p1 = process_create(1, 0, 0);
    Process *p2 = process_create(2, 0, 1);
    process_set_status(p1, PROC_BLOCKED);
    process_set_status(p2, PROC_BLOCKED);
    p1->io_remaining = 3;
    p2->io_remaining = 3;
    queue_enqueue(&s->disk_queue, p1);
    queue_enqueue(&s->disk_queue, p2);
    /* Register so sim_destroy frees them */
    sim_add_process(s, p1);
    sim_add_process(s, p2);
    /* Remove from pending (they're already in disk_queue) */
    s->pending_count = 0;

    sim_step(s); /* one tick: both should decrement */

    EXPECT_EQ(p1->io_remaining, 2);
    EXPECT_EQ(p2->io_remaining, 2);

    sim_destroy(s);
}

TEST(ConcurrentIo, BothProcessesCompleteAtSameTick) {
    DESCRIBE("in CONCURRENT mode two processes with the same duration complete simultaneously");
    SimConfig cfg     = test_config();
    cfg.disk_duration = { 2, 2 };
    cfg.io_mode_disk  = IO_MODE_CONCURRENT;
    Simulation *s     = sim_create(cfg);

    Process *p1 = process_create(1, 0, 0);
    Process *p2 = process_create(2, 0, 1);
    process_set_status(p1, PROC_BLOCKED);
    process_set_status(p2, PROC_BLOCKED);
    p1->io_remaining = 2;
    p2->io_remaining = 2;
    queue_enqueue(&s->disk_queue, p1);
    queue_enqueue(&s->disk_queue, p2);
    sim_add_process(s, p1);
    sim_add_process(s, p2);
    s->pending_count = 0;

    sim_run(s, 2); /* after 2 ticks both should have completed */

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
    SimConfig cfg     = test_config();
    cfg.disk_duration = { 3, 3 };
    cfg.io_mode_disk  = IO_MODE_QUEUE;
    Simulation *s     = sim_create(cfg);

    Process *p1 = process_create(1, 0, 0);
    Process *p2 = process_create(2, 0, 1);
    process_set_status(p1, PROC_BLOCKED);
    process_set_status(p2, PROC_BLOCKED);
    p1->io_remaining = 3;
    p2->io_remaining = 3;
    queue_enqueue(&s->disk_queue, p1); /* p1 is head */
    queue_enqueue(&s->disk_queue, p2);
    sim_add_process(s, p1);
    sim_add_process(s, p2);
    s->pending_count = 0;

    sim_step(s);

    EXPECT_EQ(p1->io_remaining, 2); /* head decremented */
    EXPECT_EQ(p2->io_remaining, 3); /* second process unchanged */

    sim_destroy(s);
}

TEST(QueueIo, SecondProcessDelayedBehindFirst) {
    DESCRIBE("in QUEUE mode the second process starts decrementing only after the first completes");
    SimConfig cfg     = test_config();
    cfg.disk_duration = { 2, 2 };
    cfg.io_mode_disk  = IO_MODE_QUEUE;
    Simulation *s     = sim_create(cfg);

    Process *p1 = process_create(1, 0, 0);
    Process *p2 = process_create(2, 0, 1);
    process_set_status(p1, PROC_BLOCKED);
    process_set_status(p2, PROC_BLOCKED);
    p1->io_remaining = 2;
    p2->io_remaining = 2;
    queue_enqueue(&s->disk_queue, p1);
    queue_enqueue(&s->disk_queue, p2);
    sim_add_process(s, p1);
    sim_add_process(s, p2);
    s->pending_count = 0;

    sim_run(s, 2); /* p1 completes after 2 ticks; p2 hasn't started */

    EXPECT_EQ(queue_size(&s->disk_queue), 1); /* only p2 remains */
    EXPECT_EQ(p2->io_remaining, 2);            /* p2 not yet decremented */

    sim_run(s, 2); /* now p2 runs its 2 ticks */
    EXPECT_EQ(queue_size(&s->disk_queue), 0);

    sim_destroy(s);
}
