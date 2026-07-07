#include <gtest/gtest.h>

extern "C" {
#include "cutesim/simulation.h"
}

#include "../test_describe.h"

static const int QUANTUM_HI = 3;
static const int QUANTUM_LO = 6;
static const int RNG_SEED   = 42;

/* Minimal config for deterministic tests: no random arrivals, no I/O. */
static SimConfig test_config(void) {
    SimConfig cfg    = {};
    cfg.quantum_hi   = QUANTUM_HI;
    cfg.quantum_lo   = QUANTUM_LO;
    cfg.seed         = RNG_SEED;
    cfg.p_io         = 0; /* no random I/O      */
    cfg.arrival_rate = 0; /* no random arrivals */
    return cfg;
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

TEST(Init, SimCreateInitializesAtTickZero) {
    DESCRIBE("a freshly created simulation starts at tick 0");
    Simulation *s = sim_create(test_config());
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->tick, 0);
    sim_destroy(s);
}

TEST(Init, SimCreateHasNoRunningProcess) {
    DESCRIBE("a freshly created simulation has no process on the CPU");
    Simulation *s = sim_create(test_config());
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->running, nullptr);
    sim_destroy(s);
}

TEST(Init, SimCreateAllQueuesEmpty) {
    DESCRIBE("all five queues start empty");
    Simulation *s = sim_create(test_config());
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(queue_is_empty(&s->hi_queue));
    EXPECT_TRUE(queue_is_empty(&s->lo_queue));
    EXPECT_TRUE(queue_is_empty(&s->disk_queue));
    EXPECT_TRUE(queue_is_empty(&s->tape_queue));
    EXPECT_TRUE(queue_is_empty(&s->printer_queue));
    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// Scheduling
// ---------------------------------------------------------------------------

TEST(Scheduling, ArrivingProcessEntersHiQueue) {
    DESCRIBE("a process added with arrival_tick=0 enters hi_queue on the first step");
    Simulation *s = sim_create(test_config());
    Process *p    = process_create(1, 0, 0);
    sim_add_process(s, p);

    /* before any step: process is pending, not yet in queue */
    EXPECT_TRUE(queue_is_empty(&s->hi_queue));

    sim_step(s); /* tick 0: process arrives, gets scheduled and runs */

    /* process is now running (scheduled from hi_queue) */
    EXPECT_EQ(s->running, p);
    sim_destroy(s);
}

TEST(Scheduling, IdleCpuSchedulesFromHiQueueFirst) {
    DESCRIBE("when CPU is idle the scheduler picks from hi_queue before lo_queue");
    Simulation *s = sim_create(test_config());
    Process *hi   = process_create(1, 0, 0);
    Process *lo   = process_create(2, 0, 1);

    sim_add_process(s, hi);
    sim_add_process(s, lo);

    /* Manually seed lo_queue before starting so we can test preference */
    queue_enqueue(&s->lo_queue, lo);
    /* Remove lo from pending so it doesn't also go to hi_queue */
    s->pending_count = 1; /* only hi remains pending */

    sim_step(s);
    EXPECT_EQ(s->running, hi);
    sim_destroy(s);
}

TEST(Scheduling, TieBreakingByCreationSeq) {
    DESCRIBE("two processes arriving at the same tick are scheduled in creation_seq order");
    Simulation *s = sim_create(test_config());
    Process *p0   = process_create(1, 0, 0); /* creation_seq = 0 */
    Process *p1   = process_create(2, 0, 1); /* creation_seq = 1 */

    /* Add in reverse order to prove creation_seq drives ordering, not add order */
    sim_add_process(s, p1);
    sim_add_process(s, p0);

    sim_step(s); /* both arrive at tick 0; p0 should be scheduled first */
    EXPECT_EQ(s->running, p0);
    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// Round Robin
// ---------------------------------------------------------------------------

TEST(RoundRobin, ProcessRunsForQuantumHiTicks) {
    DESCRIBE(
        "a process on the high-priority queue runs for exactly quantum_hi ticks before preemption");
    SimConfig cfg  = test_config();
    cfg.quantum_hi = QUANTUM_HI;
    Simulation *s  = sim_create(cfg);
    Process *p     = process_create(1, 0, 0);
    sim_add_process(s, p);

    sim_run(s, QUANTUM_HI);

    EXPECT_EQ(p->cpu_ticks, QUANTUM_HI);
    sim_destroy(s);
}

TEST(RoundRobin, PreemptedProcessMovesToLoQueue) {
    DESCRIBE("after exhausting quantum_hi the process is moved to lo_queue");
    SimConfig cfg  = test_config();
    cfg.quantum_hi = QUANTUM_HI;
    Simulation *s  = sim_create(cfg);
    Process *p     = process_create(1, 0, 0);
    sim_add_process(s, p);

    sim_run(s, QUANTUM_HI);

    EXPECT_EQ(s->running, nullptr);
    EXPECT_EQ(queue_size(&s->lo_queue), 1);
    EXPECT_EQ(queue_peek(&s->lo_queue), p);
    sim_destroy(s);
}

TEST(RoundRobin, PreemptedProcessGetsLowPriority) {
    DESCRIBE("after preemption the process priority field is updated to low (1)");
    SimConfig cfg  = test_config();
    cfg.quantum_hi = QUANTUM_HI;
    Simulation *s  = sim_create(cfg);
    Process *p     = process_create(1, 0, 0);
    sim_add_process(s, p);

    sim_run(s, QUANTUM_HI);

    EXPECT_EQ(p->priority, PRIORITY_LOW);
    sim_destroy(s);
}

TEST(RoundRobin, LoQueueProcessRunsForQuantumLoTicks) {
    DESCRIBE("a process re-scheduled from lo_queue runs for quantum_lo ticks");
    SimConfig cfg  = test_config();
    cfg.quantum_hi = QUANTUM_HI;
    cfg.quantum_lo = QUANTUM_LO;
    Simulation *s  = sim_create(cfg);
    Process *p     = process_create(1, 0, 0);
    sim_add_process(s, p);

    sim_run(s, QUANTUM_HI); /* quantum_hi exhausted → lo_queue */
    int cpu_after_hi = p->cpu_ticks;

    sim_run(s, QUANTUM_LO); /* re-scheduled from lo_queue, runs quantum_lo ticks */

    EXPECT_EQ(p->cpu_ticks, cpu_after_hi + QUANTUM_LO);
    EXPECT_EQ(queue_size(&s->lo_queue), 1); /* preempted again */
    sim_destroy(s);
}

TEST(RoundRobin, FirstCpuTickRecorded) {
    DESCRIBE("first_cpu_tick is set to the tick at which the process first runs");
    Simulation *s = sim_create(test_config());
    Process *p    = process_create(1, 0, 0);
    sim_add_process(s, p);

    sim_step(s); /* tick 0: process arrives and runs */

    EXPECT_EQ(p->first_cpu_tick, 0);
    sim_destroy(s);
}
