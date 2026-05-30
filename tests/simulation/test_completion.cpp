#include <gtest/gtest.h>

extern "C" {
#include "cutesim/simulation.h"
}

#include "../test_describe.h"

static const int BURST_LEN       = 5;
static const int LARGE_QUANTUM   = 20; /* larger than any burst so quantum never preempts */
static const int DISK_DURATION   = 2;
static const int IO_FIRES_AT_TICK = 1;

static SimConfig test_config(void) {
    SimConfig cfg    = {};
    cfg.quantum_hi   = LARGE_QUANTUM;
    cfg.quantum_lo   = LARGE_QUANTUM * 2;
    cfg.seed         = 42;
    cfg.p_io         = 0;
    cfg.arrival_rate = 0;
    return cfg;
}

// ---------------------------------------------------------------------------
// Basic burst completion
// ---------------------------------------------------------------------------

TEST(Completion, ProcessReachesDoneWhenBurstExhausted) {
    DESCRIBE("a process with cpu_burst_total=N transitions to PROC_DONE after N CPU ticks");
    Simulation *s = sim_create(test_config());
    Process    *p = process_create(1, 0, 0);
    p->cpu_burst_total = BURST_LEN;
    sim_add_process(s, p);

    sim_run(s, BURST_LEN);

    EXPECT_EQ(p->status, PROC_DONE);
    sim_destroy(s);
}

TEST(Completion, CompletionTickIsRecorded) {
    DESCRIBE("completion_tick is set to the tick on which the process finishes");
    Simulation *s = sim_create(test_config());
    Process    *p = process_create(1, 0, 0);
    p->cpu_burst_total = BURST_LEN;
    sim_add_process(s, p);

    sim_run(s, BURST_LEN);

    EXPECT_EQ(p->completion_tick, BURST_LEN - 1);
    sim_destroy(s);
}

TEST(Completion, NoBurstTotalMeansProcessNeverDone) {
    DESCRIBE("a process with cpu_burst_total=0 never reaches PROC_DONE from CPU exhaustion");
    Simulation *s = sim_create(test_config());
    Process    *p = process_create(1, 0, 0);
    /* cpu_burst_total defaults to 0 — no termination */
    sim_add_process(s, p);

    sim_run(s, LARGE_QUANTUM * 3);

    EXPECT_NE(p->status, PROC_DONE);
    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// sim_is_done
// ---------------------------------------------------------------------------

TEST(Completion, SimIsDoneWhenAllProcessesDone) {
    DESCRIBE("sim_is_done returns 1 once every process has reached PROC_DONE");
    Simulation *s  = sim_create(test_config());
    Process    *p1 = process_create(1, 0, 0);
    Process    *p2 = process_create(2, 0, 1);
    p1->cpu_burst_total = BURST_LEN;
    p2->cpu_burst_total = BURST_LEN;
    sim_add_process(s, p1);
    sim_add_process(s, p2);

    sim_run_until_done(s);

    EXPECT_EQ(sim_is_done(s), 1);
    sim_destroy(s);
}
