#include "cutesim/process.h"

#include <stdlib.h>

Process *process_create(int pid, int arrival_tick, int creation_seq) {
    Process *p = calloc(1, sizeof(Process));
    if (!p) {
        return NULL;
    }
    p->pid             = pid;
    p->arrival_tick    = arrival_tick;
    p->creation_seq    = creation_seq;
    p->status          = PROC_READY;
    p->first_cpu_tick  = -1;
    p->completion_tick = -1;
    return p;
}

void process_destroy(Process *p) {
    if (!p) {
        return;
    }
    free(p->io_script);
    free(p);
}

int process_set_status(Process *p, ProcStatus new_status) {
    switch (p->status) {
    case PROC_READY:
        if (new_status == PROC_RUNNING) {
            break;
        }
        return -1;
    case PROC_RUNNING:
        if (new_status == PROC_READY || new_status == PROC_BLOCKED || new_status == PROC_DONE) {
            break;
        }
        return -1;
    case PROC_BLOCKED:
        if (new_status == PROC_READY) {
            break;
        }
        return -1;
    case PROC_DONE:
    default:
        return -1;
    }
    p->status = new_status;
    return 0;
}
