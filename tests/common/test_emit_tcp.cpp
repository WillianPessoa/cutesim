#include <gtest/gtest.h>

extern "C" {
#include "emit_tcp.h"
}

TEST(TcpCmdParse, Step) { EXPECT_EQ(tcp_cmd_parse("step").type, TCP_CMD_STEP); }

TEST(TcpCmdParse, Reset) { EXPECT_EQ(tcp_cmd_parse("reset").type, TCP_CMD_RESET); }

TEST(TcpCmdParse, Status) { EXPECT_EQ(tcp_cmd_parse("status").type, TCP_CMD_STATUS); }

TEST(TcpCmdParse, TrailingWhitespaceIsIgnored) {
    EXPECT_EQ(tcp_cmd_parse("step  ").type, TCP_CMD_STEP);
    EXPECT_EQ(tcp_cmd_parse("status\t").type, TCP_CMD_STATUS);
}

TEST(TcpCmdParse, LeadingWhitespaceIsIgnored) {
    EXPECT_EQ(tcp_cmd_parse("  step").type, TCP_CMD_STEP);
}

TEST(TcpCmdParse, TrailingCarriageReturnIsIgnored) {
    /* Clients that send CRLF line endings must still be understood. */
    EXPECT_EQ(tcp_cmd_parse("step\r").type, TCP_CMD_STEP);
}

TEST(TcpCmdParse, EmptyLineIsInvalid) {
    EXPECT_EQ(tcp_cmd_parse("").type, TCP_CMD_INVALID);
    EXPECT_EQ(tcp_cmd_parse("   ").type, TCP_CMD_INVALID);
}

TEST(TcpCmdParse, UnknownWordIsInvalid) {
    EXPECT_EQ(tcp_cmd_parse("run").type, TCP_CMD_INVALID);
    EXPECT_EQ(tcp_cmd_parse("quit").type, TCP_CMD_INVALID);
    EXPECT_EQ(tcp_cmd_parse("stepp").type, TCP_CMD_INVALID);
}

TEST(TcpCmdParse, CaseSensitive) {
    EXPECT_EQ(tcp_cmd_parse("STEP").type, TCP_CMD_INVALID);
    EXPECT_EQ(tcp_cmd_parse("Step").type, TCP_CMD_INVALID);
}

TEST(TcpCmdParse, TrailingArgumentsAreRejected) {
    /* No command takes an argument; a second token is malformed. */
    EXPECT_EQ(tcp_cmd_parse("step now").type, TCP_CMD_INVALID);
    EXPECT_EQ(tcp_cmd_parse("reset 5").type, TCP_CMD_INVALID);
}
