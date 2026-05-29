#include <gtest/gtest.h>

extern "C" {
#include "cutesim/simulation.h"
}

#include "../test_describe.h"

static SimConfig test_config(void) {
    SimConfig cfg    = {};
    cfg.quantum_hi   = 3;
    cfg.quantum_lo   = 6;
    cfg.seed         = 42;
    cfg.p_io         = 0;
    cfg.arrival_rate = 0;
    cfg.disk_duration    = { 1, 1 };
    cfg.tape_duration    = { 1, 1 };
    cfg.printer_duration = { 1, 1 };
    return cfg;
}

/* Build a process with a single scripted I/O event. */
static Process *make_scripted(int pid, DeviceType dev, int io_tick) {
    Process    *p      = process_create(pid, 0, 0);
    ScriptedIO *script = (ScriptedIO *)malloc(sizeof(ScriptedIO));
    script[0]          = { io_tick, dev };
    p->io_script       = script;
    p->io_script_len   = 1;
    p->io_script_pos   = 0;
    return p;
}

// ---------------------------------------------------------------------------
// Feedback routing — return queue depends on device
// ---------------------------------------------------------------------------

TEST(Feedback, DiskReturnGoesToLoQueue) {
    DESCRIBE("a process returning from disk I/O enters the low-priority CPU queue");
    SimConfig cfg = test_config();
    /* disk duration = 1 tick so I/O completes one step after it starts */
    Simulation *s = sim_create(cfg);
    Process    *p = make_scripted(1, DEVICE_DISK, 1); /* I/O fires at tick 1 */
    sim_add_process(s, p);

    sim_run(s, 2); /* tick 0: runs; tick 1: I/O fires → disk_queue */
    EXPECT_EQ(queue_size(&s->disk_queue), 1);

    sim_run(s, 1); /* tick 2: disk I/O completes → should return to lo_queue */
    EXPECT_EQ(queue_size(&s->disk_queue), 0);
    EXPECT_EQ(queue_size(&s->lo_queue),   1);
    EXPECT_EQ(queue_peek(&s->lo_queue),   p);

    sim_destroy(s);
}

TEST(Feedback, TapeReturnGoesToHiQueue) {
    DESCRIBE("a process returning from tape I/O enters the high-priority CPU queue");
    Simulation *s = sim_create(test_config());
    Process    *p = make_scripted(1, DEVICE_TAPE, 1);
    sim_add_process(s, p);

    sim_run(s, 2); /* tick 1: I/O fires */
    sim_run(s, 1); /* tick 2: I/O completes → hi_queue */

    EXPECT_EQ(queue_size(&s->tape_queue), 0);
    EXPECT_EQ(queue_size(&s->hi_queue),   1);
    EXPECT_EQ(queue_peek(&s->hi_queue),   p);

    sim_destroy(s);
}

TEST(Feedback, PrinterReturnGoesToHiQueue) {
    DESCRIBE("a process returning from printer I/O enters the high-priority CPU queue");
    Simulation *s = sim_create(test_config());
    Process    *p = make_scripted(1, DEVICE_PRINTER, 1);
    sim_add_process(s, p);

    sim_run(s, 2);
    sim_run(s, 1);

    EXPECT_EQ(queue_size(&s->printer_queue), 0);
    EXPECT_EQ(queue_size(&s->hi_queue),      1);
    EXPECT_EQ(queue_peek(&s->hi_queue),      p);

    sim_destroy(s);
}

TEST(Feedback, ProcessStatusIsReadyAfterIoReturn) {
    DESCRIBE("a process re-entering a CPU queue after I/O has READY status");
    Simulation *s = sim_create(test_config());
    Process    *p = make_scripted(1, DEVICE_TAPE, 1);
    sim_add_process(s, p);

    sim_run(s, 3); /* arrives, runs, fires I/O, I/O completes */

    EXPECT_EQ(p->status, PROC_READY);
    sim_destroy(s);
}

TEST(Feedback, IoCountIncrementedOnTrigger) {
    DESCRIBE("io_count is incremented each time the process is sent to a device");
    Simulation *s = sim_create(test_config());

    /* Two scripted I/O events on different devices */
    Process    *p      = process_create(1, 0, 0);
    ScriptedIO *script = (ScriptedIO *)malloc(2 * sizeof(ScriptedIO));
    script[0]          = { 1, DEVICE_DISK };
    script[1]          = { 5, DEVICE_TAPE };
    p->io_script       = script;
    p->io_script_len   = 2;

    sim_add_process(s, p);
    sim_run(s, 2); /* first I/O fires at tick 1 */
    EXPECT_EQ(p->io_count, 1);

    sim_destroy(s);
}
