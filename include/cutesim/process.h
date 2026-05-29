#pragma once

enum { PRIORITY_HIGH = 0, PRIORITY_LOW = 1 };

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_DONE,
} ProcStatus;

typedef enum {
    DEVICE_DISK,
    DEVICE_TAPE,
    DEVICE_PRINTER,
} DeviceType;

typedef struct {
    int        tick;
    DeviceType device;
} ScriptedIO;

typedef struct {
    int        pid;
    int        ppid;
    int        priority;
    int        creation_seq;
    ProcStatus status;

    int arrival_tick;
    int first_cpu_tick;   /* -1 until first scheduled */
    int completion_tick;  /* -1 until done            */

    int cpu_burst_total;   /* total CPU ticks until PROC_DONE; 0 = no limit */

    int cpu_ticks;
    int io_ticks;
    int io_count;

    int quantum_remaining;
    int io_remaining;

    ScriptedIO *io_script;
    int         io_script_len;
    int         io_script_pos;
} Process;

/* Allocate and initialise a new process.
   Returns NULL on allocation failure. */
Process *process_create(int pid, int arrival_tick, int creation_seq);

/* Free the process and its io_script buffer. */
void process_destroy(Process *p);

/* Attempt to transition p to new_status.
   Valid transitions:
     READY   -> RUNNING
     RUNNING -> READY | BLOCKED | DONE
     BLOCKED -> READY
   Returns 0 on success, -1 if the transition is not allowed. */
int process_set_status(Process *p, ProcStatus new_status);
