#include <gtest/gtest.h>

extern "C" {
#include "cutesim/simulation.h"
}

#include "../test_describe.h"

static const int QUANTUM_HI        = 3;
static const int QUANTUM_LO        = 6;
static const int RNG_SEED          = 42;
static const int IO_DURATION_TICKS = 1;
static const int IO_FIRES_AT_TICK  = 1;

static SimConfig test_config(void) {
    SimConfig cfg        = {};
    cfg.quantum_hi       = QUANTUM_HI;
    cfg.quantum_lo       = QUANTUM_LO;
    cfg.seed             = RNG_SEED;
    cfg.p_io             = 0;
    cfg.arrival_rate     = 0;
    cfg.disk_duration    = { IO_DURATION_TICKS, IO_DURATION_TICKS };
    cfg.tape_duration    = { IO_DURATION_TICKS, IO_DURATION_TICKS };
    cfg.printer_duration = { IO_DURATION_TICKS, IO_DURATION_TICKS };
    return cfg;
}

/* Build a process with a single scripted I/O event at IO_FIRES_AT_TICK. */
static Process *make_scripted(int pid, DeviceType dev) {
    Process *p         = process_create(pid, 0, 0);
    ScriptedIO *script = (ScriptedIO *)malloc(sizeof(ScriptedIO));
    script[0]          = { IO_FIRES_AT_TICK, dev };
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
    /* disk duration = IO_DURATION_TICKS so I/O completes one step after it starts */
    Simulation *s = sim_create(cfg);
    Process *p    = make_scripted(1, DEVICE_DISK); /* I/O fires at IO_FIRES_AT_TICK */
    sim_add_process(s, p);

    sim_run(s, IO_FIRES_AT_TICK + 1); /* tick 0: runs; tick 1: I/O fires → disk_queue */
    EXPECT_EQ(queue_size(&s->disk_queue), 1);

    sim_run(s, IO_DURATION_TICKS); /* I/O completes → should return to lo_queue */
    EXPECT_EQ(queue_size(&s->disk_queue), 0);
    EXPECT_EQ(queue_size(&s->lo_queue), 1);
    EXPECT_EQ(queue_peek(&s->lo_queue), p);

    sim_destroy(s);
}

TEST(Feedback, TapeReturnGoesToHiQueue) {
    DESCRIBE("a process returning from tape I/O enters the high-priority CPU queue");
    Simulation *s = sim_create(test_config());
    Process *p    = make_scripted(1, DEVICE_TAPE);
    sim_add_process(s, p);

    sim_run(s, IO_FIRES_AT_TICK + 1); /* I/O fires at IO_FIRES_AT_TICK */
    sim_run(s, IO_DURATION_TICKS);    /* I/O completes → hi_queue */

    EXPECT_EQ(queue_size(&s->tape_queue), 0);
    EXPECT_EQ(queue_size(&s->hi_queue), 1);
    EXPECT_EQ(queue_peek(&s->hi_queue), p);

    sim_destroy(s);
}

TEST(Feedback, PrinterReturnGoesToHiQueue) {
    DESCRIBE("a process returning from printer I/O enters the high-priority CPU queue");
    Simulation *s = sim_create(test_config());
    Process *p    = make_scripted(1, DEVICE_PRINTER);
    sim_add_process(s, p);

    sim_run(s, IO_FIRES_AT_TICK + 1);
    sim_run(s, IO_DURATION_TICKS);

    EXPECT_EQ(queue_size(&s->printer_queue), 0);
    EXPECT_EQ(queue_size(&s->hi_queue), 1);
    EXPECT_EQ(queue_peek(&s->hi_queue), p);

    sim_destroy(s);
}

TEST(Feedback, ProcessStatusIsReadyAfterIoReturn) {
    DESCRIBE("a process re-entering a CPU queue after I/O has READY status");
    Simulation *s = sim_create(test_config());
    Process *p    = make_scripted(1, DEVICE_TAPE);
    sim_add_process(s, p);

    /* arrives (tick 0), runs (tick 0), fires I/O (tick 1), I/O completes (tick 2) */
    sim_run(s, IO_FIRES_AT_TICK + IO_DURATION_TICKS + 1);

    EXPECT_EQ(p->status, PROC_READY);
    sim_destroy(s);
}

TEST(Feedback, IoCountIncrementedOnTrigger) {
    DESCRIBE("io_count is incremented each time the process is sent to a device");
    static const int SECOND_IO_TICK = 5;
    Simulation *s                   = sim_create(test_config());

    /* Two scripted I/O events on different devices */
    Process *p         = process_create(1, 0, 0);
    ScriptedIO *script = (ScriptedIO *)malloc(2 * sizeof(ScriptedIO));
    script[0]          = { IO_FIRES_AT_TICK, DEVICE_DISK };
    script[1]          = { SECOND_IO_TICK, DEVICE_TAPE };
    p->io_script       = script;
    p->io_script_len   = 2;

    sim_add_process(s, p);
    sim_run(s, IO_FIRES_AT_TICK + 1); /* first I/O fires at IO_FIRES_AT_TICK */
    EXPECT_EQ(p->io_count, 1);

    sim_destroy(s);
}
