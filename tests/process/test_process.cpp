#include <gtest/gtest.h>

extern "C" {
#include "cutesim/process.h"
}

#include "../test_describe.h"

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

TEST(Init, NewProcessStatusIsReady) {
    DESCRIBE("a freshly created process starts in READY status");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->status, PROC_READY);
    process_destroy(p);
}

TEST(Init, PidAndArrivalTickStored) {
    DESCRIBE("pid and arrival_tick are stored as given to process_create");
    Process *p = process_create(7, 42, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->pid, 7);
    EXPECT_EQ(p->arrival_tick, 42);
    process_destroy(p);
}

TEST(Init, CreationSeqStored) {
    DESCRIBE("creation_seq is stored for tie-breaking during scheduling");
    Process *p = process_create(1, 0, 3);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->creation_seq, 3);
    process_destroy(p);
}

TEST(Init, FirstCpuTickIsMinusOne) {
    DESCRIBE("first_cpu_tick is -1 until the process first receives the CPU");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->first_cpu_tick, -1);
    process_destroy(p);
}

TEST(Init, CompletionTickIsMinusOne) {
    DESCRIBE("completion_tick is -1 until the process finishes");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->completion_tick, -1);
    process_destroy(p);
}

TEST(Init, CountersStartAtZero) {
    DESCRIBE("cpu_ticks, io_ticks and io_count all start at zero");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->cpu_ticks, 0);
    EXPECT_EQ(p->io_ticks, 0);
    EXPECT_EQ(p->io_count, 0);
    process_destroy(p);
}

TEST(Init, IoScriptIsNull) {
    DESCRIBE("a process created without a script has a null io_script pointer");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->io_script, nullptr);
    EXPECT_EQ(p->io_script_len, 0);
    EXPECT_EQ(p->io_script_pos, 0);
    process_destroy(p);
}

// ---------------------------------------------------------------------------
// Status transitions
// ---------------------------------------------------------------------------

TEST(Status, ReadyToRunning) {
    DESCRIBE("status can transition from READY to RUNNING");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    p->status = PROC_RUNNING;
    EXPECT_EQ(p->status, PROC_RUNNING);
    process_destroy(p);
}

TEST(Status, RunningToBlocked) {
    DESCRIBE("status can transition from RUNNING to BLOCKED when I/O is requested");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    p->status = PROC_RUNNING;
    p->status = PROC_BLOCKED;
    EXPECT_EQ(p->status, PROC_BLOCKED);
    process_destroy(p);
}

TEST(Status, BlockedToReady) {
    DESCRIBE("status can transition from BLOCKED back to READY when I/O completes");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    p->status = PROC_BLOCKED;
    p->status = PROC_READY;
    EXPECT_EQ(p->status, PROC_READY);
    process_destroy(p);
}

TEST(Status, RunningToDone) {
    DESCRIBE("status can transition from RUNNING to DONE when the process finishes");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    p->status = PROC_RUNNING;
    p->status = PROC_DONE;
    EXPECT_EQ(p->status, PROC_DONE);
    process_destroy(p);
}

// ---------------------------------------------------------------------------
// Stats accumulation
// ---------------------------------------------------------------------------

TEST(Stats, CpuTicksAccumulate) {
    DESCRIBE("cpu_ticks correctly counts every tick the process holds the CPU");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    p->cpu_ticks += 1;
    p->cpu_ticks += 1;
    p->cpu_ticks += 1;
    EXPECT_EQ(p->cpu_ticks, 3);
    process_destroy(p);
}

TEST(Stats, IoTicksAccumulate) {
    DESCRIBE("io_ticks correctly counts every tick the process spends in an I/O queue");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    p->io_ticks += 2;
    p->io_ticks += 3;
    EXPECT_EQ(p->io_ticks, 5);
    process_destroy(p);
}

TEST(Stats, IoCountIncrements) {
    DESCRIBE("io_count increments once per I/O request, independently of duration");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    p->io_count++;
    p->io_count++;
    EXPECT_EQ(p->io_count, 2);
    process_destroy(p);
}

TEST(Stats, CpuAndIoTicksAreIndependent) {
    DESCRIBE("cpu_ticks and io_ticks do not interfere with each other");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    p->cpu_ticks += 4;
    p->io_ticks  += 7;
    EXPECT_EQ(p->cpu_ticks, 4);
    EXPECT_EQ(p->io_ticks,  7);
    process_destroy(p);
}

// ---------------------------------------------------------------------------
// Scripted I/O
// ---------------------------------------------------------------------------

TEST(Script, ScriptedEventsCanBeAttached) {
    DESCRIBE("a scripted I/O list can be attached to a process after creation");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);

    ScriptedIO events[2] = { { 3, DEVICE_DISK }, { 7, DEVICE_TAPE } };
    p->io_script     = events;
    p->io_script_len = 2;

    EXPECT_EQ(p->io_script_len, 2);
    EXPECT_EQ(p->io_script[0].tick,   3);
    EXPECT_EQ(p->io_script[0].device, DEVICE_DISK);
    EXPECT_EQ(p->io_script[1].tick,   7);
    EXPECT_EQ(p->io_script[1].device, DEVICE_TAPE);

    p->io_script = nullptr; /* not heap-allocated in this test */
    process_destroy(p);
}

TEST(Script, ScriptPosAdvances) {
    DESCRIBE("io_script_pos advances as scripted events are consumed");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);

    ScriptedIO events[2] = { { 2, DEVICE_PRINTER }, { 5, DEVICE_DISK } };
    p->io_script     = events;
    p->io_script_len = 2;
    p->io_script_pos = 0;

    p->io_script_pos++;
    EXPECT_EQ(p->io_script_pos, 1);
    EXPECT_EQ(p->io_script[p->io_script_pos].tick,   5);
    EXPECT_EQ(p->io_script[p->io_script_pos].device, DEVICE_DISK);

    p->io_script = nullptr;
    process_destroy(p);
}
