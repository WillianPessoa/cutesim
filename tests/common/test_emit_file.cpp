#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include "cutesim/process.h"
#include "cutesim/simulation.h"
#include "emit_file.h"
}

static SimConfig base_cfg() {
    SimConfig cfg  = {};
    cfg.quantum_hi = 3;
    cfg.quantum_lo = 6;
    cfg.seed       = 42;
    return cfg;
}

/* Read all lines from a FILE* into a vector of strings. Rewinds before reading. */
static std::vector<std::string> read_lines(FILE *f) {
    std::vector<std::string> lines;
    rewind(f);
    char buf[65536];
    while (fgets(buf, sizeof(buf), f)) {
        std::string line(buf);
        /* strip trailing newline */
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

// ---------------------------------------------------------------------------
// One line written per sim_step call
// ---------------------------------------------------------------------------

TEST(EmitFile, OneLinePerStep) {
    Simulation *s = sim_create(base_cfg());
    ASSERT_NE(s, nullptr);

    Process *p         = process_create(0, 0, 0);
    p->cpu_burst_total = 9;
    sim_add_process(s, p);

    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);

    static const int STEPS = 5;
    for (int i = 0; i < STEPS; i++) {
        sim_step(s);
        EXPECT_EQ(emit_file_write(f, s), 0);
    }

    auto lines = read_lines(f);
    EXPECT_EQ((int)lines.size(), STEPS);

    fclose(f);
    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// Each line is a JSON object
// ---------------------------------------------------------------------------

TEST(EmitFile, EachLineIsJsonObject) {
    Simulation *s = sim_create(base_cfg());
    ASSERT_NE(s, nullptr);

    Process *p         = process_create(0, 0, 0);
    p->cpu_burst_total = 6;
    sim_add_process(s, p);

    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);

    for (int i = 0; i < 4; i++) {
        sim_step(s);
        emit_file_write(f, s);
    }

    auto lines = read_lines(f);
    for (const auto &line : lines) {
        EXPECT_EQ(line.front(), '{') << "line does not start with '{'";
        EXPECT_EQ(line.back(), '}') << "line does not end with '}'";
    }

    fclose(f);
    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// Tick numbers are sequential starting at 1 (snapshot is taken after step)
// ---------------------------------------------------------------------------

TEST(EmitFile, TickNumbersAreSequential) {
    Simulation *s = sim_create(base_cfg());
    ASSERT_NE(s, nullptr);

    Process *p         = process_create(0, 0, 0);
    p->cpu_burst_total = 12;
    sim_add_process(s, p);

    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);

    static const int STEPS = 6;
    for (int i = 0; i < STEPS; i++) {
        sim_step(s);
        emit_file_write(f, s);
    }

    auto lines = read_lines(f);
    ASSERT_EQ((int)lines.size(), STEPS);

    for (int i = 0; i < STEPS; i++) {
        char expected[32];
        snprintf(expected, sizeof(expected), "\"tick\":%d", i + 1);
        EXPECT_NE(lines[i].find(expected), std::string::npos)
            << "line " << i << " missing " << expected;
    }

    fclose(f);
    sim_destroy(s);
}

// ---------------------------------------------------------------------------
// Final line has "done":true after the simulation completes
// ---------------------------------------------------------------------------

TEST(EmitFile, FinalLineMarksDone) {
    Simulation *s = sim_create(base_cfg());
    ASSERT_NE(s, nullptr);

    Process *p         = process_create(0, 0, 0);
    p->cpu_burst_total = 3;
    sim_add_process(s, p);

    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);

    while (!sim_is_done(s)) {
        sim_step(s);
        emit_file_write(f, s);
    }

    auto lines = read_lines(f);
    ASSERT_FALSE(lines.empty());
    EXPECT_NE(lines.back().find("\"done\":true"), std::string::npos);

    /* all lines before the last should have "done":false */
    for (size_t i = 0; i + 1 < lines.size(); i++) {
        EXPECT_NE(lines[i].find("\"done\":false"), std::string::npos)
            << "line " << i << " should not be done yet";
    }

    fclose(f);
    sim_destroy(s);
}
