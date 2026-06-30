#pragma once

#include "cutesim/config.h"
#include "cutesim/simulation.h"

/* Print a human-readable summary of cfg to stdout (compact, English). */
void print_sim_summary(SimConfig cfg);

/* Print usage help to stdout. */
void print_help(void);

/* Print the full styled Portuguese header with simulation parameters.
   `scripted` non-zero adjusts the workload lines for a scripted scenario
   (explicit arrivals and per-process bursts instead of the random knobs). */
void print_header(SimConfig cfg, int scripted);

/* Print the state of the simulation after the tick that just completed.
   Must be called immediately after sim_step (s->tick is already advanced). */
void print_tick_trace(const Simulation *s);

/* Print the final "done" or "stopped" summary. */
void print_sim_done(const Simulation *s);

/* Print the per-process metrics table and system-wide statistics. */
void print_sim_statistics(const Simulation *s);
