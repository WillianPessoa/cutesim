#include "cutesim/process.h"

#include <stdlib.h>

Process *process_create(int pid, int arrival_tick, int creation_seq) {
    Process *p = calloc(1, sizeof(Process));
    if (!p) {
        return NULL;
    }
    p->pid            = pid;
    p->arrival_tick   = arrival_tick;
    p->creation_seq   = creation_seq;
    p->status         = PROC_READY;
    p->first_cpu_tick = -1;
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
