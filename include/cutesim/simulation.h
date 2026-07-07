#pragma once

#include "cutesim/config.h"
#include "cutesim/process.h"
#include "cutesim/queue.h"

/* -------------------------------------------------------------------------
 * Per-tick event log
 * ---------------------------------------------------------------------- */

typedef enum {
    SIM_EVT_ARRIVED,   /* process entered hi_queue                               */
    SIM_EVT_SCHEDULED, /* process got CPU;  data1=0(ALTA)/1(BAIXA)               */
    SIM_EVT_PREEMPTED, /* quantum exhausted; data1=used, data2=max               */
    SIM_EVT_IO_START,  /* sent to device;   data1=DeviceType, data2=io_remaining */
    SIM_EVT_IO_TICK,   /* I/O advanced 1t;  data1=DeviceType, data2=remaining    */
    SIM_EVT_IO_RETURN, /* I/O done;         data1=DeviceType, data2=0(ALTA)/1(BAIXA) */
    SIM_EVT_COMPLETED, /* process finished                                       */
} SimEventType;

#define SIM_MAX_EVENTS 128

typedef struct {
    SimEventType type;
    int pid;
    int data1;
    int data2;
} SimEvent;

typedef struct {
    /* --- public state (readable by callers and tests) --- */
    int tick;
    Queue hi_queue; /* CPU ready — high priority  */
    Queue lo_queue; /* CPU ready — low priority   */
    Queue disk_queue;
    Queue tape_queue;
    Queue printer_queue;
    Process *running; /* NULL when CPU is idle      */
    int quantum_used; /* ticks consumed in current quantum */
    SimConfig cfg;

    /* Set when a preemption occurs during a tick; cleared at the start of the
       next tick.  Allows the display to show the quantum=N/N moment. */
    Process *last_preempted;
    int last_quantum_used;       /* quantum_used at the moment of preemption  */
    int last_quantum_max;        /* quantum limit that was reached             */
    int last_preempted_priority; /* priority BEFORE demotion to PRIORITY_LOW  */

    /* Set when a process completes during a tick; cleared at the start of the
       next tick.  The process ran this tick, so displays must not show idle. */
    Process *last_completed;
    int last_completed_quantum_used; /* quantum_used at the moment of completion */
    int last_completed_priority;

    /* Set when the running process departs for I/O during a tick; cleared at
       the start of the next tick.  Model A: the process does NOT consume a CPU
       tick when I/O fires, so the tick is genuinely idle — these fields only
       let displays show where the process went instead of a bare idle. */
    Process *last_io_started;
    DeviceType last_io_device;
    int last_io_quantum_used; /* quantum_used at the moment of departure */
    int last_io_priority;

    /* Per-tick event log — cleared at the start of each sim_step */
    SimEvent events[SIM_MAX_EVENTS];
    int event_count;

    /* --- internal --- */
    Process **pending; /* processes waiting to arrive */
    int pending_count;
    int pending_cap;
    Process **all_processes; /* every process ever added    */
    int all_count;
    int all_cap;
    unsigned rng_state;
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
