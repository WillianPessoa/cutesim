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
