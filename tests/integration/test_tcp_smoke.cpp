#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <string>
#include <thread>

extern "C" {
#include "emit_tcp.h"
}

namespace {

constexpr int kPort = 19234;

void batch_spawn(Simulation *sim, void *) {
    /* Three processes, all arriving at tick 0, each with a small CPU burst. */
    for (int i = 0; i < 3; i++) {
        Process *p         = process_create(i + 1, 0, i);
        p->cpu_burst_total = 4;
        sim_add_process(sim, p);
    }
}

/* Connect to 127.0.0.1:kPort, retrying briefly while the server binds. */
int connect_with_retry() {
    for (int attempt = 0; attempt < 100; attempt++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            return -1;
        }
        struct sockaddr_in addr = {};
        addr.sin_family         = AF_INET;
        addr.sin_addr.s_addr    = htonl(INADDR_LOOPBACK);
        addr.sin_port           = htons(kPort);
        if (connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0) {
            return fd;
        }
        close(fd);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return -1;
}

std::string recv_line(int fd) {
    std::string out;
    char c;
    while (read(fd, &c, 1) == 1) {
        if (c == '\n') {
            break;
        }
        out.push_back(c);
    }
    return out;
}

} // namespace

TEST(TcpSmoke, StepReturnsOneJsonLine) {
    SimConfig cfg  = {};
    cfg.quantum_hi = 3;
    cfg.quantum_lo = 6;
    cfg.seed       = 42;

    std::thread server([&] { emit_tcp_serve(cfg, batch_spawn, nullptr, kPort); });

    int fd = connect_with_retry();
    ASSERT_GE(fd, 0) << "could not connect to the TCP server";

    /* step -> one snapshot line at tick 1 */
    ASSERT_EQ(write(fd, "step\n", 5), 5);
    std::string line = recv_line(fd);
    EXPECT_NE(line.find("\"tick\":1"), std::string::npos) << "got: " << line;
    EXPECT_EQ(line.front(), '{');
    EXPECT_EQ(line.back(), '}');

    /* status -> same tick, no advance */
    ASSERT_EQ(write(fd, "status\n", 7), 7);
    EXPECT_NE(recv_line(fd).find("\"tick\":1"), std::string::npos);

    /* reset -> back to tick 0 */
    ASSERT_EQ(write(fd, "reset\n", 6), 6);
    EXPECT_NE(recv_line(fd).find("\"tick\":0"), std::string::npos);

    /* invalid command -> error line, server stays up */
    ASSERT_EQ(write(fd, "bogus\n", 6), 6);
    EXPECT_NE(recv_line(fd).find("\"error\""), std::string::npos);

    /* server still responds after the error */
    ASSERT_EQ(write(fd, "step\n", 5), 5);
    EXPECT_NE(recv_line(fd).find("\"tick\":1"), std::string::npos);

    /* disconnecting ends the client session; the server loops back to accept */
    close(fd);
    server.detach();
}
