#include <gtest/gtest.h>

extern "C" {
#include "cutesim/process.h"
#include "cutesim/statistics.h"
}

#include "../test_describe.h"

/* Build a completed (or, when completion < 0, still-running) process with
   hand-chosen fields. Caller owns the returned process. */
static Process *
make_proc(int pid, int arrival, int first_cpu, int completion, int cpu_ticks, int io_ticks) {
    Process *p         = process_create(pid, arrival, pid - 1);
    p->first_cpu_tick  = first_cpu;
    p->completion_tick = completion;
    p->cpu_ticks       = cpu_ticks;
    p->io_ticks        = io_ticks;
    if (completion >= 0) {
        p->status = PROC_DONE;
    }
    return p;
}

// ---------------------------------------------------------------------------
// stats_for_process — per-process metrics
// ---------------------------------------------------------------------------

TEST(StatsForProcess, TurnaroundIsCompletionMinusArrival) {
    DESCRIBE("turnaround = completion_tick - arrival_tick for a completed process");
    Process *p         = process_create(1, /*arrival*/ 2, /*seq*/ 0);
    p->completion_tick = 12;
    p->status          = PROC_DONE;

    ProcStats st = stats_for_process(p);

    EXPECT_EQ(st.turnaround, 10); /* 12 - 2 */
    EXPECT_EQ(st.completed, 1);

    process_destroy(p);
}

TEST(StatsForProcess, ResponseIsFirstCpuMinusArrival) {
    DESCRIBE("response = first_cpu_tick - arrival_tick");
    Process *p         = process_create(1, /*arrival*/ 3, /*seq*/ 0);
    p->first_cpu_tick  = 5;
    p->completion_tick = 20;
    p->status          = PROC_DONE;

    ProcStats st = stats_for_process(p);

    EXPECT_EQ(st.response, 2); /* 5 - 3 */

    process_destroy(p);
}

TEST(StatsForProcess, ResponseIsMinusOneWhenNeverScheduled) {
    DESCRIBE("response = -1 when the process never received the CPU (first_cpu_tick < 0)");
    Process *p = process_create(1, /*arrival*/ 3, /*seq*/ 0);
    /* first_cpu_tick stays -1 from process_create */

    ProcStats st = stats_for_process(p);

    EXPECT_EQ(st.response, -1);

    process_destroy(p);
}

TEST(StatsForProcess, WaitingIsTurnaroundMinusServiceMinusIo) {
    DESCRIBE("waiting = turnaround - service - io_ticks for a completed process");
    Process *p         = process_create(1, /*arrival*/ 0, /*seq*/ 0);
    p->completion_tick = 20;
    p->status          = PROC_DONE;
    p->cpu_ticks       = 8;
    p->io_ticks        = 5;

    ProcStats st = stats_for_process(p);

    EXPECT_EQ(st.turnaround, 20);
    EXPECT_EQ(st.service, 8);
    EXPECT_EQ(st.io_ticks, 5);
    EXPECT_EQ(st.waiting, 7); /* 20 - 8 - 5 */

    process_destroy(p);
}

TEST(StatsForProcess, IoCountIsCarriedThrough) {
    DESCRIBE("io_count is reported as-is from the PCB");
    Process *p  = process_create(1, /*arrival*/ 0, /*seq*/ 0);
    p->io_count = 4;

    ProcStats st = stats_for_process(p);

    EXPECT_EQ(st.io_count, 4);

    process_destroy(p);
}

TEST(StatsForProcess, PerDeviceIoTicksCarriedThrough) {
    DESCRIBE("io_ticks_disk/tape/printer are copied from PCB fields into ProcStats");
    Process *p          = process_create(1, /*arrival*/ 0, /*seq*/ 0);
    p->io_ticks_disk    = 3;
    p->io_ticks_tape    = 5;
    p->io_ticks_printer = 2;
    p->io_ticks         = 10; /* total: 3 + 5 + 2 = 10 */

    ProcStats st = stats_for_process(p);

    EXPECT_EQ(st.io_ticks_disk,    3);
    EXPECT_EQ(st.io_ticks_tape,    5);
    EXPECT_EQ(st.io_ticks_printer, 2);
    EXPECT_EQ(st.io_ticks,         10);

    process_destroy(p);
}

TEST(StatsForProcess, IncompleteProcessHasNoTurnaroundOrWaiting) {
    DESCRIBE(
        "a process that never reached PROC_DONE is marked incomplete; turnaround/waiting stay 0");
    Process *p = process_create(1, /*arrival*/ 0, /*seq*/ 0);
    /* completion_tick stays -1 from process_create */
    p->cpu_ticks = 5;

    ProcStats st = stats_for_process(p);

    EXPECT_EQ(st.completed, 0);
    EXPECT_EQ(st.turnaround, 0);
    EXPECT_EQ(st.waiting, 0);
    EXPECT_EQ(st.service, 5); /* service is still reported */

    process_destroy(p);
}

// ---------------------------------------------------------------------------
// stats_compute — system-wide aggregation
// ---------------------------------------------------------------------------

TEST(StatsCompute, CountsProcessesAndCompleted) {
    DESCRIBE("process_count counts all; completed_count counts only finished processes");
    Process *p1            = make_proc(1, 0, 0, 10, 6, 2); /* completed */
    Process *p2            = make_proc(2, 2, 4, -1, 3, 0); /* still running */
    const Process *procs[] = { p1, p2 };

    SimStats st = stats_compute(procs, 2, 20);

    EXPECT_EQ(st.process_count, 2);
    EXPECT_EQ(st.completed_count, 1);

    process_destroy(p1);
    process_destroy(p2);
}

/* Shared hand-calculated scenario for the aggregate tests below.
 *
 *  PID  arrival  first_cpu  completion  cpu  io  | turn  resp  wait
 *   1      0         0          10        6   2  |  10     0     2
 *   2      2         4          16        8   3  |  14     2     3
 *   3      5         7          20        5   4  |  15     2     6
 *  total_ticks = 20
 *  busy = 6+8+5 = 19   util = 19/20 = 0.95   throughput = 3/20 = 0.15
 *  avg_turn = 39/3 = 13.0   avg_wait = 11/3   avg_resp = 4/3
 */
static const int SCENARIO_TOTAL_TICKS = 20;

static void make_scenario(Process **out /* [3] */) {
    out[0] = make_proc(1, 0, 0, 10, 6, 2);
    out[1] = make_proc(2, 2, 4, 16, 8, 3);
    out[2] = make_proc(3, 5, 7, 20, 5, 4);
}

TEST(StatsCompute, CpuUtilizationIsBusyOverTotal) {
    DESCRIBE("cpu_utilization = sum(cpu_ticks) / total_ticks");
    Process *p[3];
    make_scenario(p);
    const Process *procs[] = { p[0], p[1], p[2] };

    SimStats st = stats_compute(procs, 3, SCENARIO_TOTAL_TICKS);

    EXPECT_EQ(st.busy_ticks, 19);
    EXPECT_DOUBLE_EQ(st.cpu_utilization, 0.95); /* 19 / 20 */

    for (int i = 0; i < 3; i++) {
        process_destroy(p[i]);
    }
}

TEST(StatsCompute, ThroughputIsCompletedOverTotal) {
    DESCRIBE("throughput = completed_count / total_ticks");
    Process *p[3];
    make_scenario(p);
    const Process *procs[] = { p[0], p[1], p[2] };

    SimStats st = stats_compute(procs, 3, SCENARIO_TOTAL_TICKS);

    EXPECT_DOUBLE_EQ(st.throughput, 0.15); /* 3 / 20 */

    for (int i = 0; i < 3; i++) {
        process_destroy(p[i]);
    }
}

TEST(StatsCompute, AveragesOverCompletedProcesses) {
    DESCRIBE("avg turnaround/waiting/response are means over completed processes");
    Process *p[3];
    make_scenario(p);
    const Process *procs[] = { p[0], p[1], p[2] };

    SimStats st = stats_compute(procs, 3, SCENARIO_TOTAL_TICKS);

    EXPECT_DOUBLE_EQ(st.avg_turnaround, 39.0 / 3.0); /* (10+14+15)/3 = 13.0 */
    EXPECT_DOUBLE_EQ(st.avg_waiting, 11.0 / 3.0);    /* (2+3+6)/3 */
    EXPECT_DOUBLE_EQ(st.avg_response, 4.0 / 3.0);    /* (0+2+2)/3 */

    for (int i = 0; i < 3; i++) {
        process_destroy(p[i]);
    }
}

TEST(StatsCompute, ZeroTotalTicksYieldsZeroRates) {
    DESCRIBE("total_ticks = 0 yields zero utilization/throughput instead of dividing by zero");
    Process *p1            = make_proc(1, 0, 0, 0, 0, 0);
    const Process *procs[] = { p1 };

    SimStats st = stats_compute(procs, 1, 0);

    EXPECT_DOUBLE_EQ(st.cpu_utilization, 0.0);
    EXPECT_DOUBLE_EQ(st.throughput, 0.0);

    process_destroy(p1);
}

TEST(StatsCompute, NoCompletedProcessesYieldsZeroAverages) {
    DESCRIBE("with no completed processes, averages stay 0.0 instead of NaN");
    Process *p1            = make_proc(1, 0, 0, -1, 5, 0); /* still running */
    const Process *procs[] = { p1 };

    SimStats st = stats_compute(procs, 1, 10);

    EXPECT_EQ(st.completed_count, 0);
    EXPECT_DOUBLE_EQ(st.avg_turnaround, 0.0);
    EXPECT_DOUBLE_EQ(st.avg_waiting, 0.0);
    EXPECT_DOUBLE_EQ(st.avg_response, 0.0);

    process_destroy(p1);
}
