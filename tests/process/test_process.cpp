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
// Status transitions — valid
// ---------------------------------------------------------------------------

TEST(Status, ReadyToRunningIsAllowed) {
    DESCRIBE("READY -> RUNNING is a valid transition: process_set_status returns 0 and updates status");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(process_set_status(p, PROC_RUNNING), 0);
    EXPECT_EQ(p->status, PROC_RUNNING);
    process_destroy(p);
}

TEST(Status, RunningToReadyIsAllowed) {
    DESCRIBE("RUNNING -> READY is valid: process preempted by quantum exhaustion returns to ready queue");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    process_set_status(p, PROC_RUNNING);
    EXPECT_EQ(process_set_status(p, PROC_READY), 0);
    EXPECT_EQ(p->status, PROC_READY);
    process_destroy(p);
}

TEST(Status, RunningToBlockedIsAllowed) {
    DESCRIBE("RUNNING -> BLOCKED is valid: process requested I/O and vacates the CPU");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    process_set_status(p, PROC_RUNNING);
    EXPECT_EQ(process_set_status(p, PROC_BLOCKED), 0);
    EXPECT_EQ(p->status, PROC_BLOCKED);
    process_destroy(p);
}

TEST(Status, RunningToDoneIsAllowed) {
    DESCRIBE("RUNNING -> DONE is valid: process completed its work");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    process_set_status(p, PROC_RUNNING);
    EXPECT_EQ(process_set_status(p, PROC_DONE), 0);
    EXPECT_EQ(p->status, PROC_DONE);
    process_destroy(p);
}

TEST(Status, BlockedToReadyIsAllowed) {
    DESCRIBE("BLOCKED -> READY is valid: I/O completed and process re-enters a CPU queue");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    process_set_status(p, PROC_RUNNING);
    process_set_status(p, PROC_BLOCKED);
    EXPECT_EQ(process_set_status(p, PROC_READY), 0);
    EXPECT_EQ(p->status, PROC_READY);
    process_destroy(p);
}

// ---------------------------------------------------------------------------
// Status transitions — invalid
// ---------------------------------------------------------------------------

TEST(Status, ReadyToBlockedIsRejected) {
    DESCRIBE("READY -> BLOCKED is invalid: a process cannot request I/O without holding the CPU");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(process_set_status(p, PROC_BLOCKED), -1);
    EXPECT_EQ(p->status, PROC_READY);
    process_destroy(p);
}

TEST(Status, ReadyToDoneIsRejected) {
    DESCRIBE("READY -> DONE is invalid: a process cannot finish without ever running");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(process_set_status(p, PROC_DONE), -1);
    EXPECT_EQ(p->status, PROC_READY);
    process_destroy(p);
}

TEST(Status, BlockedToRunningIsRejected) {
    DESCRIBE("BLOCKED -> RUNNING is invalid: a process returning from I/O must pass through the ready queue");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    process_set_status(p, PROC_RUNNING);
    process_set_status(p, PROC_BLOCKED);
    EXPECT_EQ(process_set_status(p, PROC_RUNNING), -1);
    EXPECT_EQ(p->status, PROC_BLOCKED);
    process_destroy(p);
}

TEST(Status, DoneToAnyIsRejected) {
    DESCRIBE("DONE is a terminal state: no further transitions are allowed");
    Process *p = process_create(1, 0, 0);
    ASSERT_NE(p, nullptr);
    process_set_status(p, PROC_RUNNING);
    process_set_status(p, PROC_DONE);
    EXPECT_EQ(process_set_status(p, PROC_READY),   -1);
    EXPECT_EQ(process_set_status(p, PROC_RUNNING), -1);
    EXPECT_EQ(process_set_status(p, PROC_BLOCKED), -1);
    EXPECT_EQ(p->status, PROC_DONE);
    process_destroy(p);
}
