#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

/* Run the binary with the given arguments and capture stdout + exit code.
   _popen/_pclose are cmd.exe-backed and return the exit code directly;
   popen/pclose return a wait status that WEXITSTATUS unpacks. */
static std::string run(const std::string &args, int *exit_code = nullptr) {
    std::string bin = RR_FEEDBACK_BIN;
#ifdef _WIN32
    for (char &c : bin) {
        if (c == '/') {
            c = '\\'; /* cmd.exe reads / as a switch in the program path */
        }
    }
#endif
    std::string cmd = bin + " " + args + " 2>&1";
#ifdef _WIN32
    FILE *pipe = _popen(cmd.c_str(), "r");
#else
    FILE *pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) {
        if (exit_code) {
            *exit_code = -1;
        }
        return "";
    }

    std::string output;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) {
        output += buf;
    }

#ifdef _WIN32
    int status = _pclose(pipe);
    if (exit_code) {
        *exit_code = status;
    }
#else
    int status = pclose(pipe);
    if (exit_code) {
        *exit_code = WEXITSTATUS(status);
    }
#endif
    return output;
}

// --- Help flag ---

TEST(Binary, HelpFlagExitsZero) {
    int exit_code = -1;
    run("-h", &exit_code);
    EXPECT_EQ(exit_code, 0);
}

TEST(Binary, HelpLongFlagExitsZero) {
    int exit_code = -1;
    run("--help", &exit_code);
    EXPECT_EQ(exit_code, 0);
}

// --- Unknown flag ---

TEST(Binary, UnknownFlagExitsNonZero) {
    int exit_code = -1;
    run("--this-flag-does-not-exist", &exit_code);
    EXPECT_NE(exit_code, 0);
}

// --- Header output ---

TEST(Binary, DefaultRunShowsHeader) {
    std::string out = run("");
    EXPECT_NE(out.find("alta prioridade"), std::string::npos);
    EXPECT_NE(out.find("baixa prioridade"), std::string::npos);
    EXPECT_NE(out.find("42"), std::string::npos);     /* seed */
    EXPECT_NE(out.find("CONCLU"), std::string::npos); /* CONCLUÍDA or CONCLUÍDOS */
}

TEST(Binary, QuantumHiFlagAppearsInHeader) {
    std::string out = run("--quantum-hi=7");
    EXPECT_NE(out.find("alta prioridade  : 7"), std::string::npos);
}

// --- Statistics report ---

TEST(Binary, BatchRunShowsStatisticsReport) {
    std::string out = run("--process-count=3");
    EXPECT_NE(out.find("ESTAT"), std::string::npos);   /* ESTATÍSTICAS */
    EXPECT_NE(out.find("Utiliza"), std::string::npos); /* Utilização da CPU */
    EXPECT_NE(out.find("Throughput"), std::string::npos);
    EXPECT_NE(out.find("turnaround"), std::string::npos); /* per-process table header */
}

// --- Scenario file ---

static std::string write_temp(const char *contents) {
    static int counter = 0;
    std::string path   = testing::TempDir() + "cutesim_bin_scn_" + std::to_string(counter++);
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return "";
    }
    out << contents;
    return path;
}

TEST(Binary, RunsAScriptedScenarioFile) {
    std::string path = write_temp("quantum-hi = 3\n"
                                  "disk-duration = 2\n"
                                  "[process]\n"
                                  "arrival = 0\n"
                                  "burst = 4\n"
                                  "io = 2:disk\n");
    ASSERT_FALSE(path.empty());

    int code        = -1;
    std::string out = run(path, &code);

    EXPECT_EQ(code, 0);
    EXPECT_NE(out.find("CONCLU"), std::string::npos); /* simulation completed */
    EXPECT_NE(out.find("ESTAT"), std::string::npos);  /* statistics printed */

    std::remove(path.c_str());
}

TEST(Binary, MalformedScenarioFileExitsNonZero) {
    std::string path = write_temp("totally-not-a-key = 1\n");
    ASSERT_FALSE(path.empty());

    int code = -1;
    run(path, &code);

    EXPECT_NE(code, 0);

    std::remove(path.c_str());
}
