#include <gtest/gtest.h>

#include <cstring>
#include <unistd.h>

extern "C" {
#include "cutesim/scenario.h"
}

#include "../test_describe.h"

// ---------------------------------------------------------------------------
// Global key = value lines
// ---------------------------------------------------------------------------

TEST(ScenarioGlobals, ParsesQuantumHi) {
    DESCRIBE("a 'quantum-hi = N' line sets config.quantum_hi");
    Scenario sc = {};

    int rc = scenario_parse_string("quantum-hi = 5\n", &sc);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(sc.config.quantum_hi, 5);

    scenario_free(&sc);
}

TEST(ScenarioGlobals, ParsesIntegerSettings) {
    DESCRIBE("integer globals map to their SimConfig fields");
    Scenario sc      = {};
    const char *text = "quantum-hi = 3\n"
                       "quantum-lo = 6\n"
                       "seed = 42\n"
                       "process-count = 10\n"
                       "p-io = 25\n"
                       "p-disk = 50\n"
                       "p-tape = 30\n"
                       "p-printer = 20\n";

    int rc = scenario_parse_string(text, &sc);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(sc.config.quantum_hi, 3);
    EXPECT_EQ(sc.config.quantum_lo, 6);
    EXPECT_EQ(sc.config.seed, 42u);
    EXPECT_EQ(sc.config.process_count, 10);
    EXPECT_EQ(sc.config.p_io, 25);
    EXPECT_EQ(sc.config.p_disk, 50);
    EXPECT_EQ(sc.config.p_tape, 30);
    EXPECT_EQ(sc.config.p_printer, 20);

    scenario_free(&sc);
}

TEST(ScenarioGlobals, ParsesDurations) {
    DESCRIBE("device and service durations accept a fixed value or an N-M range");
    Scenario sc      = {};
    const char *text = "disk-duration = 3\n"
                       "tape-duration = 2-4\n"
                       "printer-duration = 5\n"
                       "service-duration = 8-12\n";

    int rc = scenario_parse_string(text, &sc);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(sc.config.disk_duration.min, 3);
    EXPECT_EQ(sc.config.disk_duration.max, 3);
    EXPECT_EQ(sc.config.tape_duration.min, 2);
    EXPECT_EQ(sc.config.tape_duration.max, 4);
    EXPECT_EQ(sc.config.printer_duration.min, 5);
    EXPECT_EQ(sc.config.printer_duration.max, 5);
    EXPECT_EQ(sc.config.service_duration.min, 8);
    EXPECT_EQ(sc.config.service_duration.max, 12);

    scenario_free(&sc);
}

TEST(ScenarioGlobals, ParsesIoModes) {
    DESCRIBE("device modes accept 'queue' or 'concurrent'");
    Scenario sc      = {};
    const char *text = "disk-mode = queue\n"
                       "tape-mode = concurrent\n"
                       "printer-mode = queue\n";

    int rc = scenario_parse_string(text, &sc);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(sc.config.io_mode_disk, IO_MODE_QUEUE);
    EXPECT_EQ(sc.config.io_mode_tape, IO_MODE_CONCURRENT);
    EXPECT_EQ(sc.config.io_mode_printer, IO_MODE_QUEUE);

    scenario_free(&sc);
}

// ---------------------------------------------------------------------------
// [process] blocks
// ---------------------------------------------------------------------------

TEST(ScenarioProcess, ParsesArrivalAndBurst) {
    DESCRIBE("a [process] block records arrival and burst");
    Scenario sc      = {};
    const char *text = "[process]\n"
                       "arrival = 4\n"
                       "burst = 8\n";

    int rc = scenario_parse_string(text, &sc);

    EXPECT_EQ(rc, 0);
    ASSERT_EQ(sc.process_count, 1);
    EXPECT_EQ(sc.processes[0].arrival_tick, 4);
    EXPECT_EQ(sc.processes[0].burst, 8);
    EXPECT_EQ(sc.processes[0].io_count, 0);

    scenario_free(&sc);
}

TEST(ScenarioProcess, ParsesIoTimeline) {
    DESCRIBE("io = lists events; tick:device uses the global duration, tick:device:dur overrides");
    Scenario sc      = {};
    const char *text = "[process]\n"
                       "arrival = 0\n"
                       "burst = 10\n"
                       "io = 2:printer:6, 3:disk, 5:tape:4-6\n";

    int rc = scenario_parse_string(text, &sc);

    EXPECT_EQ(rc, 0);
    ASSERT_EQ(sc.process_count, 1);
    ScriptedProcess *p = &sc.processes[0];
    ASSERT_EQ(p->io_count, 3);

    /* 2:printer:6 — fixed-duration override */
    EXPECT_EQ(p->io[0].service_tick, 2);
    EXPECT_EQ(p->io[0].device, DEVICE_PRINTER);
    EXPECT_EQ(p->io[0].has_duration, 1);
    EXPECT_EQ(p->io[0].duration.min, 6);
    EXPECT_EQ(p->io[0].duration.max, 6);

    /* 3:disk — no override, uses global */
    EXPECT_EQ(p->io[1].service_tick, 3);
    EXPECT_EQ(p->io[1].device, DEVICE_DISK);
    EXPECT_EQ(p->io[1].has_duration, 0);

    /* 5:tape:4-6 — range override */
    EXPECT_EQ(p->io[2].service_tick, 5);
    EXPECT_EQ(p->io[2].device, DEVICE_TAPE);
    EXPECT_EQ(p->io[2].has_duration, 1);
    EXPECT_EQ(p->io[2].duration.min, 4);
    EXPECT_EQ(p->io[2].duration.max, 6);

    scenario_free(&sc);
}

TEST(ScenarioProcess, GrowsForManyProcessesAndKeepsOrder) {
    DESCRIBE("the process array grows dynamically past its initial capacity, preserving order");
    Scenario sc      = {};
    const char *text = "# five processes force a realloc past the initial cap\n"
                       "[process]\narrival = 0\nburst = 3\n"
                       "[process]\narrival = 1\nburst = 4\n"
                       "[process]\narrival = 2\nburst = 5\n"
                       "[process]\narrival = 3\nburst = 6\n"
                       "[process]\narrival = 4\nburst = 7\n";

    int rc = scenario_parse_string(text, &sc);

    EXPECT_EQ(rc, 0);
    ASSERT_EQ(sc.process_count, 5);
    EXPECT_EQ(sc.processes[0].arrival_tick, 0);
    EXPECT_EQ(sc.processes[4].arrival_tick, 4);
    EXPECT_EQ(sc.processes[4].burst, 7);

    scenario_free(&sc);
}

// ---------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------

TEST(ScenarioErrors, RejectsUnknownGlobalKey) {
    DESCRIBE("an unrecognised global key is a parse error");
    Scenario sc = {};
    EXPECT_EQ(scenario_parse_string("frobnicate = 1\n", &sc), -1);
    scenario_free(&sc);
}

TEST(ScenarioErrors, RejectsUnknownBlock) {
    DESCRIBE("an unrecognised [block] header is a parse error");
    Scenario sc = {};
    EXPECT_EQ(scenario_parse_string("[widget]\n", &sc), -1);
    scenario_free(&sc);
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

TEST(ScenarioValidation, RejectsIoAtOrBeyondBurst) {
    DESCRIBE("a scripted I/O at service_tick >= burst is unreachable and rejected");
    Scenario sc      = {};
    const char *text = "[process]\n"
                       "burst = 5\n"
                       "io = 5:disk\n"; /* 5 is not < 5 */

    int rc = scenario_parse_string(text, &sc);

    EXPECT_EQ(rc, -1);

    scenario_free(&sc);
}

// ---------------------------------------------------------------------------
// File reading
// ---------------------------------------------------------------------------

TEST(ScenarioFile, ReadsAndParsesAFile) {
    DESCRIBE("scenario_parse_file reads the file and parses its contents");
    char path[] = "/tmp/cutesim_scenario_XXXXXX";
    int fd      = mkstemp(path);
    ASSERT_NE(fd, -1);
    const char *text = "quantum-hi = 7\n[process]\narrival = 1\nburst = 4\n";
    ASSERT_EQ(write(fd, text, strlen(text)), (ssize_t)strlen(text));
    close(fd);

    Scenario sc = {};
    int rc      = scenario_parse_file(path, &sc);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(sc.config.quantum_hi, 7);
    ASSERT_EQ(sc.process_count, 1);
    EXPECT_EQ(sc.processes[0].burst, 4);

    scenario_free(&sc);
    unlink(path);
}

TEST(ScenarioFile, MissingFileReturnsError) {
    DESCRIBE("a missing scenario file is an error, not a crash");
    Scenario sc = {};
    EXPECT_EQ(scenario_parse_file("/no/such/cutesim/file", &sc), -1);
    scenario_free(&sc);
}

// keep the validation section below
TEST(ScenarioValidation, RejectsNonIncreasingIoTicks) {
    DESCRIBE("scripted I/O service ticks must be strictly increasing");
    Scenario sc      = {};
    const char *text = "[process]\n"
                       "burst = 10\n"
                       "io = 5:disk, 3:tape\n"; /* 3 after 5 */

    int rc = scenario_parse_string(text, &sc);

    EXPECT_EQ(rc, -1);

    scenario_free(&sc);
}
