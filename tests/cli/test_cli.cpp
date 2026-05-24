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
