#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

/* Run the binary with the given arguments and capture stdout + exit code. */
static std::string run(const std::string &args, int *exit_code = nullptr) {
    std::string cmd = std::string(RR_FEEDBACK_BIN) + " " + args + " 2>&1";
    FILE *pipe      = popen(cmd.c_str(), "r");
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

    int status = pclose(pipe);
    if (exit_code) {
        *exit_code = WEXITSTATUS(status);
    }
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
    char path[] = "/tmp/cutesim_bin_scn_XXXXXX";
    int fd      = mkstemp(path);
    if (fd == -1) {
        return "";
    }
    ssize_t n = write(fd, contents, std::strlen(contents));
    (void)n;
    close(fd);
    return std::string(path);
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

    unlink(path.c_str());
}

TEST(Binary, MalformedScenarioFileExitsNonZero) {
    std::string path = write_temp("totally-not-a-key = 1\n");
    ASSERT_FALSE(path.empty());

    int code = -1;
    run(path, &code);

    EXPECT_NE(code, 0);

    unlink(path.c_str());
}
