#include <gtest/gtest.h>

#include <string>

extern "C" {
#include "args.h"
#include "display.h"
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------

TEST(Summary, DefaultConfigShowsAllDefaults) {
    char     *argv[] = {(char *)"rr-feedback", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(1, argv, &error);

    testing::internal::CaptureStdout();
    print_sim_summary(cfg);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("quantum-hi=3"), std::string::npos);
    EXPECT_NE(out.find("quantum-lo=6"), std::string::npos);
    EXPECT_NE(out.find("seed=42"),      std::string::npos);
    EXPECT_NE(out.find("mode=batch"),   std::string::npos);
    EXPECT_NE(out.find("trace=off"),    std::string::npos);
}

// ---------------------------------------------------------------------------
// Defaults — no flags, correct initial values
// ---------------------------------------------------------------------------

TEST(Defaults, QuantumHiIsThree) {
    char     *argv[] = {(char *)"rr-feedback", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(1, argv, &error);
    EXPECT_EQ(cfg.quantum_hi, 3);
}

TEST(Defaults, QuantumLoIsSix) {
    char     *argv[] = {(char *)"rr-feedback", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(1, argv, &error);
    EXPECT_EQ(cfg.quantum_lo, 6);
}

TEST(Defaults, SeedIsFortyTwo) {
    char     *argv[] = {(char *)"rr-feedback", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(1, argv, &error);
    EXPECT_EQ(cfg.seed, 42u);
}

TEST(Defaults, RunModeIsBatch) {
    char     *argv[] = {(char *)"rr-feedback", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(1, argv, &error);
    EXPECT_EQ(cfg.run_mode, RUN_BATCH);
}

TEST(Defaults, TraceIsOff) {
    char     *argv[] = {(char *)"rr-feedback", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(1, argv, &error);
    EXPECT_EQ(cfg.trace, 0);
}

// ---------------------------------------------------------------------------
// ParseArgs — each flag sets the correct field
// ---------------------------------------------------------------------------

TEST(ParseArgs, QuantumHiFromFlag) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--quantum-hi=4", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.quantum_hi, 4);
}

TEST(ParseArgs, QuantumLoFromFlag) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--quantum-lo=8", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.quantum_lo, 8);
}

TEST(ParseArgs, ProcessCountFromFlag) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--process-count=10", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.process_count, 10);
}

TEST(ParseArgs, ArrivalRateFromFlag) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--arrival-rate=20", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.arrival_rate, 20);
}

TEST(ParseArgs, PIoFromFlag) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--p-io=30", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.p_io, 30);
}

TEST(ParseArgs, PDiskFromFlag) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--p-disk=50", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.p_disk, 50);
}

TEST(ParseArgs, PTapeFromFlag) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--p-tape=30", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.p_tape, 30);
}

TEST(ParseArgs, PPrinterFromFlag) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--p-printer=20", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.p_printer, 20);
}

TEST(ParseArgs, SeedFromFlag) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--seed=99", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.seed, 99u);
}

TEST(ParseArgs, TraceFromFlag) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--trace", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.trace, 1);
}

TEST(ParseArgs, ScenarioFileAsPositionalArg) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"scenario.txt", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_STREQ(cfg.scenario_file, "scenario.txt");
}

// ---------------------------------------------------------------------------
// Duration — fixed value and range
// ---------------------------------------------------------------------------

TEST(Duration, DiskFixed) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--disk-duration=5", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.disk_duration.min, 5);
    EXPECT_EQ(cfg.disk_duration.max, 5);
}

TEST(Duration, DiskRange) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--disk-duration=3-8", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.disk_duration.min, 3);
    EXPECT_EQ(cfg.disk_duration.max, 8);
}

TEST(Duration, TapeFixed) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--tape-duration=4", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.tape_duration.min, 4);
    EXPECT_EQ(cfg.tape_duration.max, 4);
}

TEST(Duration, TapeRange) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--tape-duration=2-6", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.tape_duration.min, 2);
    EXPECT_EQ(cfg.tape_duration.max, 6);
}

TEST(Duration, PrinterFixed) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--printer-duration=10", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.printer_duration.min, 10);
    EXPECT_EQ(cfg.printer_duration.max, 10);
}

TEST(Duration, PrinterRange) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--printer-duration=1-3", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.printer_duration.min, 1);
    EXPECT_EQ(cfg.printer_duration.max, 3);
}

// ---------------------------------------------------------------------------
// IoMode — concurrent and queue per device
// ---------------------------------------------------------------------------

TEST(IoMode, DiskConcurrent) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--io-mode-disk=concurrent", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.io_mode_disk, IO_MODE_CONCURRENT);
}

TEST(IoMode, DiskQueue) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--io-mode-disk=queue", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.io_mode_disk, IO_MODE_QUEUE);
}

TEST(IoMode, TapeConcurrent) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--io-mode-tape=concurrent", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.io_mode_tape, IO_MODE_CONCURRENT);
}

TEST(IoMode, TapeQueue) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--io-mode-tape=queue", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.io_mode_tape, IO_MODE_QUEUE);
}

TEST(IoMode, PrinterConcurrent) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--io-mode-printer=concurrent", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.io_mode_printer, IO_MODE_CONCURRENT);
}

TEST(IoMode, PrinterQueue) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--io-mode-printer=queue", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.io_mode_printer, IO_MODE_QUEUE);
}

// ---------------------------------------------------------------------------
// RunMode — batch, steps, interactive
// ---------------------------------------------------------------------------

TEST(RunMode, ShortFlagNSetsSteps) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"-n", (char *)"20", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(3, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.run_mode, RUN_STEPS);
}

TEST(RunMode, LongFlagStepsSetsSteps) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--steps=20", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.run_mode, RUN_STEPS);
}

TEST(RunMode, StepsValueIsStored) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--steps=42", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(cfg.steps, 42);
}

TEST(RunMode, ShortFlagISetsInteractive) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"-i", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.run_mode, RUN_INTERACTIVE);
}

TEST(RunMode, LongFlagInteractiveSetsInteractive) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--interactive", nullptr};
    int       error  = 0;
    SimConfig cfg    = parse_args(2, argv, &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(cfg.run_mode, RUN_INTERACTIVE);
}

// ---------------------------------------------------------------------------
// Validation — invalid inputs must set error = 1
// ---------------------------------------------------------------------------

TEST(Validation, UnknownFlagIsError) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--unknown-flag=1", nullptr};
    int       error  = 0;
    parse_args(2, argv, &error);
    EXPECT_EQ(error, 1);
}

TEST(Validation, QuantumHiZeroIsError) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--quantum-hi=0", nullptr};
    int       error  = 0;
    parse_args(2, argv, &error);
    EXPECT_EQ(error, 1);
}

TEST(Validation, QuantumLoZeroIsError) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--quantum-lo=0", nullptr};
    int       error  = 0;
    parse_args(2, argv, &error);
    EXPECT_EQ(error, 1);
}

TEST(Validation, PIoAboveHundredIsError) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--p-io=101", nullptr};
    int       error  = 0;
    parse_args(2, argv, &error);
    EXPECT_EQ(error, 1);
}

TEST(Validation, PIoBelowZeroIsError) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--p-io=-1", nullptr};
    int       error  = 0;
    parse_args(2, argv, &error);
    EXPECT_EQ(error, 1);
}

TEST(Validation, DeviceProbsNotSummingToHundredIsError) {
    char *argv[] = {(char *)"rr-feedback", (char *)"--p-disk=50",
                    (char *)"--p-tape=30",  (char *)"--p-printer=10", nullptr};
    int   error  = 0;
    parse_args(4, argv, &error);
    EXPECT_EQ(error, 1);
}

TEST(Validation, DurationRangeMinAboveMaxIsError) {
    char     *argv[] = {(char *)"rr-feedback", (char *)"--disk-duration=8-3", nullptr};
    int       error  = 0;
    parse_args(2, argv, &error);
    EXPECT_EQ(error, 1);
}
