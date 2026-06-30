#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include "snapshot.h"
#include "cutesim/simulation.h"
#include "cutesim/process.h"
}

static SimConfig base_cfg() {
    SimConfig cfg   = {};
    cfg.quantum_hi  = 3;
    cfg.quantum_lo  = 6;
    cfg.seed        = 42;
    return cfg;
}

static bool has(const char *buf, const char *needle) {
    return strstr(buf, needle) != nullptr;
}

// ---------------------------------------------------------------------------
// Empty simulation — no processes
// ---------------------------------------------------------------------------

TEST(Snapshot, EmptySimulation) {
    Simulation *s = sim_create(base_cfg());
    ASSERT_NE(s, nullptr);

    char buf[4096];
    int n = snapshot_to_json(s, buf, sizeof(buf));

    EXPECT_GT(n, 0);
    EXPECT_LT(n, (int)sizeof(buf));

    EXPECT_TRUE(has(buf, "\"tick\":0"));
    EXPECT_TRUE(has(buf, "\"cpu\":null"));
    EXPECT_TRUE(has(buf, "\"high\":[]"));
    EXPECT_TRUE(has(buf, "\"low\":[]"));
    EXPECT_TRUE(has(buf, "\"disk\":[]"));
    EXPECT_TRUE(has(buf, "\"tape\":[]"));
    EXPECT_TRUE(has(buf, "\"printer\":[]"));
    EXPECT_TRUE(has(buf, "\"events\":[]"));
    EXPECT_TRUE(has(buf, "\"finished\":[]"));
    EXPECT_TRUE(has(buf, "\"done\":true"));

    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// Running process — cpu field populated
// ---------------------------------------------------------------------------

TEST(Snapshot, RunningProcess) {
    Simulation *s = sim_create(base_cfg());
    ASSERT_NE(s, nullptr);

    Process *p       = process_create(0, 0, 0);
    p->cpu_burst_total = 6;
    sim_add_process(s, p);

    sim_step(s); /* tick 0→1: process arrives, gets scheduled, runs one tick */

    char buf[4096];
    int n = snapshot_to_json(s, buf, sizeof(buf));

    EXPECT_GT(n, 0);
    EXPECT_TRUE(has(buf, "\"tick\":1"));
    EXPECT_TRUE(has(buf, "\"cpu\":{"));
    EXPECT_TRUE(has(buf, "\"pid\":0"));
    EXPECT_TRUE(has(buf, "\"queue\":\"high\""));
    EXPECT_TRUE(has(buf, "\"done\":false"));

    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// Finished process — finished array with per-device IO breakdown
// ---------------------------------------------------------------------------

TEST(Snapshot, FinishedProcess) {
    SimConfig cfg     = base_cfg();
    cfg.disk_duration = { 2, 2 };
    Simulation *s     = sim_create(cfg);
    ASSERT_NE(s, nullptr);

    Process *p       = process_create(0, 0, 0);
    p->cpu_burst_total = 5;
    /* scripted disk I/O after 1 CPU tick */
    ScriptedIO ev    = { 1, DEVICE_DISK };
    p->io_script     = &ev;
    p->io_script_len = 1;
    sim_add_process(s, p);

    sim_run_until_done(s);
    p->io_script = nullptr; /* stack-allocated */

    char buf[4096];
    int n = snapshot_to_json(s, buf, sizeof(buf));

    EXPECT_GT(n, 0);
    EXPECT_TRUE(has(buf, "\"done\":true"));
    EXPECT_TRUE(has(buf, "\"finished\":[{"));
    EXPECT_TRUE(has(buf, "\"service_time\":5"));
    EXPECT_TRUE(has(buf, "\"io_disk\":2"));
    EXPECT_TRUE(has(buf, "\"io_tape\":0"));
    EXPECT_TRUE(has(buf, "\"io_printer\":0"));
    EXPECT_TRUE(has(buf, "\"io_total\":2"));

    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// Tick events — events array serialized with correct types
// ---------------------------------------------------------------------------

TEST(Snapshot, TickEventsIncludeArrivedAndScheduled) {
    Simulation *s = sim_create(base_cfg());
    ASSERT_NE(s, nullptr);

    Process *p       = process_create(0, 0, 0);
    p->cpu_burst_total = 6;
    sim_add_process(s, p);

    sim_step(s); /* first tick: arrived + scheduled events */

    char buf[4096];
    int n = snapshot_to_json(s, buf, sizeof(buf));

    EXPECT_GT(n, 0);
    EXPECT_TRUE(has(buf, "\"events\":[{"));
    EXPECT_TRUE(has(buf, "\"type\":\"arrived\""));
    EXPECT_TRUE(has(buf, "\"type\":\"scheduled\""));

    sim_destroy(s);
}

TEST(Snapshot, PreemptionEventIncludesQuantumFields) {
    SimConfig cfg  = base_cfg();
    cfg.quantum_hi = 2; /* short quantum so preemption happens quickly */
    Simulation *s  = sim_create(cfg);
    ASSERT_NE(s, nullptr);

    Process *p       = process_create(0, 0, 0);
    p->cpu_burst_total = 10;
    sim_add_process(s, p);

    /* tick 0: arrive+schedule+run(1); tick 1: run(2)+preempt — snapshot captures tick 1 events */
    sim_run(s, 2);

    char buf[4096];
    snapshot_to_json(s, buf, sizeof(buf));

    EXPECT_TRUE(has(buf, "\"type\":\"preempted\""));
    EXPECT_TRUE(has(buf, "\"quantum_used\":"));
    EXPECT_TRUE(has(buf, "\"quantum_max\":"));

    sim_destroy(s);
}

TEST(Snapshot, IoStartEventIncludesDeviceAndRemaining) {
    SimConfig cfg     = base_cfg();
    cfg.disk_duration = { 4, 4 };
    Simulation *s     = sim_create(cfg);
    ASSERT_NE(s, nullptr);

    Process *p       = process_create(0, 0, 0);
    p->cpu_burst_total = 10;
    ScriptedIO ev    = { 1, DEVICE_DISK };
    p->io_script     = &ev;
    p->io_script_len = 1;
    sim_add_process(s, p);

    sim_run(s, 2); /* tick 0: arrive+schedule; tick 1: run; tick 1 fires IO before tick 2 */

    char buf[4096];
    snapshot_to_json(s, buf, sizeof(buf));
    p->io_script = nullptr;

    EXPECT_TRUE(has(buf, "\"type\":\"io_start\""));
    EXPECT_TRUE(has(buf, "\"device\":\"disk\""));
    EXPECT_TRUE(has(buf, "\"io_remaining\":4"));

    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// Buffer overflow — returns true size, buffer null-terminated
// ---------------------------------------------------------------------------

TEST(Snapshot, BufferOverflowReturnsTrueSize) {
    Simulation *s = sim_create(base_cfg());
    ASSERT_NE(s, nullptr);

    char big[4096];
    int full = snapshot_to_json(s, big, sizeof(big));
    EXPECT_GT(full, 0);

    /* With a 1-byte buffer, nothing is written but the count is the same */
    char tiny[1] = {};
    int n        = snapshot_to_json(s, tiny, 1);
    EXPECT_EQ(n, full);
    EXPECT_EQ(tiny[0], '\0');

    sim_destroy(s);
}
