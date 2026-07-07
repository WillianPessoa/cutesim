#pragma once

#include "cutesim/config.h"
#include "cutesim/simulation.h"

/* -------------------------------------------------------------------------
 * TCP command server backend.
 *
 * A single-threaded, line-oriented request/response server. A client connects,
 * sends one command per line, and receives one JSON line (snapshot format) per
 * command. Play/pause loops are the client's responsibility (e.g. a QML Timer
 * sending `step` repeatedly).
 * ---------------------------------------------------------------------- */

typedef enum {
    TCP_CMD_INVALID, /* unrecognized line — server replies with an error, stays up */
    TCP_CMD_STEP,    /* advance one tick, reply with one snapshot line             */
    TCP_CMD_RESET,   /* rebuild the simulation from the original config            */
    TCP_CMD_STATUS,  /* reply with the current snapshot without advancing          */
} TcpCmdType;

typedef struct {
    TcpCmdType type;
} TcpCommand;

/* Default TCP port used when --serve is given without an explicit port. */
#define TCP_DEFAULT_PORT 9000

/* Parse a single command line (without trailing newline) into a TcpCommand.
   Pure and I/O-free — the unit of the command grammar, tested in isolation.
   Unknown or malformed input yields TCP_CMD_INVALID. */
TcpCommand tcp_cmd_parse(const char *line);

/* Serve the simulation over TCP on the given port (0 selects TCP_DEFAULT_PORT).
   Blocks accepting one client at a time, processing commands until the client
   disconnects, then loops back to accept the next client. Returns 0 on clean
   shutdown, -1 on a fatal socket setup error.

   `spawn` is invoked to (re)populate a freshly created Simulation both at start
   and on each `reset`; `spawn_ctx` is passed through untouched. */
int emit_tcp_serve(SimConfig cfg,
                   void (*spawn)(Simulation *sim, void *ctx),
                   void *spawn_ctx,
                   int port);
