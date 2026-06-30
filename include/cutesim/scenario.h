#pragma once

#include "cutesim/config.h"
#include "cutesim/process.h"

/* A scripted process parsed from a scenario file: explicit arrival + burst and an
   ordered timeline of I/O events. The `io` array is heap-owned by the Scenario. */
typedef struct {
    int arrival_tick;
    int burst;      /* cpu_burst_total; 0 = unlimited */
    ScriptedIO *io; /* dynamic array, length io_count */
    int io_count;
    int io_cap;
} ScriptedProcess;

/* A parsed scenario: the global config plus the scripted processes. The processes
   array and each process's io array are heap-owned; release with scenario_free. */
typedef struct {
    SimConfig config;
    ScriptedProcess *processes; /* dynamic array, length process_count */
    int process_count;
    int process_cap;
} Scenario;

/* Parse scenario text into `out`. `out` must be zero-initialised; `out->config` may
   be pre-set to defaults and the parser overrides only the keys it finds.
   Returns 0 on success, -1 on a syntax or validation error.
   On success the caller owns out's heap arrays — release with scenario_free. */
int scenario_parse_string(const char *text, Scenario *out);

/* Read and parse the file at `path`. Returns 0 on success, -1 on I/O or parse error. */
int scenario_parse_file(const char *path, Scenario *out);

/* Free all heap arrays owned by `out` and reset it to empty. Safe on a zeroed Scenario. */
void scenario_free(Scenario *out);
