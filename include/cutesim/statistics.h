#pragma once

#include "cutesim/process.h"

/* Per-process metrics derived from a finished (or partially run) PCB.
   turnaround / waiting are only meaningful when `completed` is 1.
   response is -1 when the process never received the CPU. */
typedef struct {
    int pid;
    int arrival;
    int service;  /* cpu_ticks consumed */
    int io_ticks; /* total I/O ticks across all devices */
    int io_ticks_disk;
    int io_ticks_tape;
    int io_ticks_printer;
    int io_count;
    int turnaround; /* completion - arrival   (valid iff completed) */
    int waiting;    /* turnaround - service - io_ticks (valid iff completed) */
    int response;   /* first_cpu - arrival, or -1 if never scheduled */
    int completed;  /* 1 if the process reached PROC_DONE */
} ProcStats;

/* System-wide metrics aggregated across a process set.
   Averages cover completed processes only. */
typedef struct {
    int process_count;
    int completed_count;
    int total_ticks;
    int busy_ticks;         /* sum of cpu_ticks across all processes */
    double cpu_utilization; /* busy_ticks / total_ticks */
    double throughput;      /* completed_count / total_ticks */
    double avg_turnaround;
    double avg_waiting;
    double avg_response;
} SimStats;

/* Derive per-process metrics from a single PCB. Pure — reads p, allocates nothing. */
ProcStats stats_for_process(const Process *p);

/* Aggregate system-wide metrics over `count` processes given the total tick count.
   Pure — reads the process set, allocates nothing. */
SimStats stats_compute(const Process *const *procs, int count, int total_ticks);
