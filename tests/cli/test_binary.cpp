#include <gtest/gtest.h>

#include <cstdio>
#include <string>

/* Run the binary with the given arguments and capture stdout + exit code. */
static std::string run(const std::string &args, int *exit_code = nullptr) {
    std::string cmd = std::string(RR_FEEDBACK_BIN) + " " + args + " 2>&1";
    FILE       *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        if (exit_code) *exit_code = -1;
        return "";
    }

    std::string output;
    char        buf[256];
    while (fgets(buf, sizeof(buf), pipe)) {
        output += buf;
    }

    int status = pclose(pipe);
    if (exit_code) *exit_code = WEXITSTATUS(status);
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
    EXPECT_NE(out.find("alta prioridade"),  std::string::npos);
    EXPECT_NE(out.find("baixa prioridade"), std::string::npos);
    EXPECT_NE(out.find("42"),               std::string::npos); /* seed */
    EXPECT_NE(out.find("CONCLU"),           std::string::npos); /* CONCLUÍDA or CONCLUÍDOS */
}

TEST(Binary, QuantumHiFlagAppearsInHeader) {
    std::string out = run("--quantum-hi=7");
    EXPECT_NE(out.find("alta prioridade  : 7"), std::string::npos);
}
