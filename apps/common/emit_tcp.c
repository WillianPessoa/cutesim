#include "emit_tcp.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "snapshot.h"

/* -------------------------------------------------------------------------
 * Command parsing (pure)
 * ---------------------------------------------------------------------- */

/* Copy the single whitespace-delimited token of `line` into `word` (capacity
   `cap`). Returns the number of tokens seen: 0 (blank), 1 (exactly one token,
   stored in word), or 2 (a second token exists — caller treats as malformed). */
/* NOLINTBEGIN(clang-analyzer-security.ArrayBound) — glibc's isspace is a
   table lookup and the analyzer treats the network byte as a tainted index,
   but the unsigned char cast bounds it to 0..255, inside the table's domain. */
static int single_token(const char *line, char *word, size_t cap) {
    const char *p = line;
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p == '\0') {
        return 0;
    }
    size_t n = 0;
    while (*p && !isspace((unsigned char)*p)) {
        if (n + 1 < cap) {
            word[n] = *p;
        }
        n++;
        p++;
    }
    word[n < cap ? n : cap - 1] = '\0';

    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    return *p == '\0' ? 1 : 2;
}
/* NOLINTEND(clang-analyzer-security.ArrayBound) */

TcpCommand tcp_cmd_parse(const char *line) {
    TcpCommand cmd = { TCP_CMD_INVALID };
    char word[16];

    if (single_token(line, word, sizeof(word)) != 1) {
        return cmd;
    }
    if (strcmp(word, "step") == 0) {
        cmd.type = TCP_CMD_STEP;
    } else if (strcmp(word, "reset") == 0) {
        cmd.type = TCP_CMD_RESET;
    } else if (strcmp(word, "status") == 0) {
        cmd.type = TCP_CMD_STATUS;
    }
    return cmd;
}

/* -------------------------------------------------------------------------
 * Socket helpers
 * ---------------------------------------------------------------------- */

#define TCP_LINE_MAX 256
#define SNAP_STACK 16384

/* Write the whole buffer, tolerating short writes. Returns 0 on success,
   -1 if the peer went away. SIGPIPE is masked process-wide in emit_tcp_serve. */
static int write_all(int fd, const char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t w = write(fd, buf + off, len - off);
        if (w <= 0) {
            return -1;
        }
        off += (size_t)w;
    }
    return 0;
}

/* Serialize the current snapshot and send it as one '\n'-terminated line. */
static int send_snapshot(int fd, const Simulation *s) {
    char stack[SNAP_STACK];
    char *heap = NULL;
    int n      = snapshot_to_json(s, stack, sizeof(stack));

    const char *line = stack;
    if (n >= SNAP_STACK) {
        heap = malloc((size_t)n + 1);
        if (!heap) {
            return -1;
        }
        snapshot_to_json(s, heap, (size_t)n + 1);
        line = heap;
    }

    int rc = write_all(fd, line, (size_t)n);
    if (rc == 0) {
        rc = write_all(fd, "\n", 1);
    }
    free(heap);
    return rc;
}

static int send_error(int fd, const char *msg) {
    char line[128];
    int n = snprintf(line, sizeof(line), "{\"error\":\"%s\"}\n", msg);
    return write_all(fd, line, (size_t)n);
}

/* -------------------------------------------------------------------------
 * Server loop
 * ---------------------------------------------------------------------- */

/* Read one line (up to '\n') from fd into buf. Returns the line length
   (excluding the newline) on success, -1 on EOF/error. Overlong lines are
   truncated at the buffer boundary; the remainder is left for the next read. */
static int recv_line(int fd, char *buf, size_t cap) {
    size_t n = 0;
    for (;;) {
        char c;
        ssize_t r = read(fd, &c, 1);
        if (r <= 0) {
            return -1;
        }
        if (c == '\n') {
            break;
        }
        if (n + 1 < cap) {
            buf[n++] = c;
        }
    }
    buf[n] = '\0';
    return (int)n;
}

/* Handle one connected client until it disconnects. sim points to the caller's
   Simulation pointer so RESET can swap it for a fresh instance. */
static void serve_client(int fd,
                         Simulation **sim,
                         SimConfig cfg,
                         void (*spawn)(Simulation *, void *),
                         void *ctx) {
    char line[TCP_LINE_MAX];
    while (recv_line(fd, line, sizeof(line)) >= 0) {
        TcpCommand cmd = tcp_cmd_parse(line);
        int rc         = 0;
        switch (cmd.type) {
        case TCP_CMD_STEP:
            if (!sim_is_done(*sim)) {
                sim_step(*sim);
            }
            rc = send_snapshot(fd, *sim);
            break;
        case TCP_CMD_STATUS:
            rc = send_snapshot(fd, *sim);
            break;
        case TCP_CMD_RESET: {
            Simulation *fresh = sim_create(cfg);
            if (!fresh) {
                rc = send_error(fd, "reset failed");
                break;
            }
            spawn(fresh, ctx);
            sim_destroy(*sim);
            *sim = fresh;
            rc   = send_snapshot(fd, *sim);
            break;
        }
        case TCP_CMD_INVALID:
            rc = send_error(fd, "unknown command");
            break;
        }
        if (rc != 0) {
            break; /* peer went away mid-write */
        }
    }
}

int emit_tcp_serve(SimConfig cfg,
                   void (*spawn)(Simulation *sim, void *ctx),
                   void *spawn_ctx,
                   int port) {
    signal(SIGPIPE, SIG_IGN);

    if (port <= 0) {
        port = TCP_DEFAULT_PORT;
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        return -1;
    }

    int one = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr = { 0 };
    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = htonl(INADDR_LOOPBACK);
    addr.sin_port           = htons((uint16_t)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(listen_fd, 1) != 0) {
        close(listen_fd);
        return -1;
    }

    Simulation *sim = sim_create(cfg);
    if (!sim) {
        close(listen_fd);
        return -1;
    }
    spawn(sim, spawn_ctx);

    for (;;) {
        int client = accept(listen_fd, NULL, NULL);
        if (client < 0) {
            break;
        }
        serve_client(client, &sim, cfg, spawn, spawn_ctx);
        close(client);
    }

    sim_destroy(sim);
    close(listen_fd);
    return 0;
}
