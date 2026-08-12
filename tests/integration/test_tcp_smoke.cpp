#include <gtest/gtest.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <atomic>
#include <string>
#include <thread>

extern "C" {
#include "emit_tcp.h"
}

namespace {

/* Client-side seam mirroring the one in emit_tcp.c: descriptor type, I/O
   calls and teardown differ between winsock and BSD sockets. */
#ifdef _WIN32
using SockFd = SOCKET;
bool sock_valid(SockFd fd) { return fd != INVALID_SOCKET; }
int sock_read(SockFd fd, char *buf, size_t len) { return recv(fd, buf, static_cast<int>(len), 0); }
int sock_write(SockFd fd, const char *buf, size_t len) {
    return send(fd, buf, static_cast<int>(len), 0);
}
void sock_close(SockFd fd) { closesocket(fd); }
void sock_startup() {
    WSADATA wsa;
    ASSERT_EQ(WSAStartup(MAKEWORD(2, 2), &wsa), 0);
}
#else
using SockFd = int;
bool sock_valid(SockFd fd) { return fd >= 0; }
int sock_read(SockFd fd, char *buf, size_t len) { return static_cast<int>(read(fd, buf, len)); }
int sock_write(SockFd fd, const char *buf, size_t len) {
    return static_cast<int>(write(fd, buf, len));
}
void sock_close(SockFd fd) { close(fd); }
void sock_startup() {}
#endif

constexpr int kPort = 19234;

void batch_spawn(Simulation *sim, void * /*ctx*/) {
    /* Three processes, all arriving at tick 0, each with a small CPU burst. */
    for (int i = 0; i < 3; i++) {
        Process *p         = process_create(i + 1, 0, i);
        p->cpu_burst_total = 4;
        sim_add_process(sim, p);
    }
}

/* Connect to 127.0.0.1:kPort, retrying briefly while the server binds. */
SockFd connect_with_retry() {
    for (int attempt = 0; attempt < 100; attempt++) {
        SockFd fd = socket(AF_INET, SOCK_STREAM, 0);
        if (!sock_valid(fd)) {
            break;
        }
        struct sockaddr_in addr = {};
        addr.sin_family         = AF_INET;
        addr.sin_addr.s_addr    = htonl(INADDR_LOOPBACK);
        addr.sin_port           = htons(kPort);
        if (connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0) {
            return fd;
        }
        sock_close(fd);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return static_cast<SockFd>(-1);
}

std::string recv_line(SockFd fd) {
    std::string out;
    char c;
    while (sock_read(fd, &c, 1) == 1) {
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

    sock_startup();

    std::thread server([&] { emit_tcp_serve(cfg, batch_spawn, nullptr, kPort); });

    SockFd fd = connect_with_retry();
    ASSERT_TRUE(sock_valid(fd)) << "could not connect to the TCP server";

    /* step -> one snapshot line at tick 1 */
    ASSERT_EQ(sock_write(fd, "step\n", 5), 5);
    std::string line = recv_line(fd);
    EXPECT_NE(line.find("\"tick\":1"), std::string::npos) << "got: " << line;
    EXPECT_EQ(line.front(), '{');
    EXPECT_EQ(line.back(), '}');

    /* status -> same tick, no advance */
    ASSERT_EQ(sock_write(fd, "status\n", 7), 7);
    EXPECT_NE(recv_line(fd).find("\"tick\":1"), std::string::npos);

    /* reset -> back to tick 0 */
    ASSERT_EQ(sock_write(fd, "reset\n", 6), 6);
    EXPECT_NE(recv_line(fd).find("\"tick\":0"), std::string::npos);

    /* invalid command -> error line, server stays up */
    ASSERT_EQ(sock_write(fd, "bogus\n", 6), 6);
    EXPECT_NE(recv_line(fd).find("\"error\""), std::string::npos);

    /* server still responds after the error */
    ASSERT_EQ(sock_write(fd, "step\n", 5), 5);
    EXPECT_NE(recv_line(fd).find("\"tick\":1"), std::string::npos);

    /* disconnecting ends the client session; the server loops back to accept */
    sock_close(fd);
    server.detach();
}
