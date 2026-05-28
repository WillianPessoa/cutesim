#pragma once

#include "cutesim/config.h"
#include "cutesim/process.h"
#include "cutesim/queue.h"

typedef struct {
    /* --- public state (readable by callers and tests) --- */
    int       tick;
    Queue     hi_queue;      /* CPU ready — high priority  */
    Queue     lo_queue;      /* CPU ready — low priority   */
    Queue     disk_queue;
    Queue     tape_queue;
    Queue     printer_queue;
    Process  *running;       /* NULL when CPU is idle      */
    int       quantum_used;  /* ticks consumed in current quantum */
    SimConfig cfg;

    /* --- internal --- */
    Process **pending;       /* processes waiting to arrive */
    int       pending_count;
    int       pending_cap;
    Process **all_processes; /* every process ever added    */
    int       all_count;
    int       all_cap;
    unsigned  rng_state;
} Simulation;

/* Create a simulation from cfg.
   Returns NULL on allocation failure. */
Simulation *sim_create(SimConfig cfg);

/* Free all resources including processes owned by the simulation. */
void sim_destroy(Simulation *s);

/* Add a process to the simulation.
   Ownership transfers to the simulation — do not free p yourself.
   p is placed in the pending arrival list and enters hi_queue at p->arrival_tick. */
void sim_add_process(Simulation *s, Process *p);

/* Advance the simulation by one tick. */
void sim_step(Simulation *s);

/* Advance the simulation by n ticks. */
void sim_run(Simulation *s, int n);

/* Run until sim_is_done returns 1. */
void sim_run_until_done(Simulation *s);

/* Returns 1 when no pending arrivals remain and all processes are DONE. */
int sim_is_done(const Simulation *s);
