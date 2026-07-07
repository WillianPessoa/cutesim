#include "cutesim/simulation.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cutesim/rng.h"

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

static int sample_duration(Duration d, unsigned *rng) { return rng_range(rng, d.min, d.max); }

/* Apply a status transition that is known to be valid by construction.
   Asserts in debug builds so an unexpected invalid transition surfaces
   instead of being silently ignored. */
static void must_set_status(Process *p, ProcStatus st) {
    int rc = process_set_status(p, st);
    assert(rc == 0);
    (void)rc;
}

static DeviceType select_device(SimConfig cfg, unsigned *rng) {
    int r = (int)(rng_next(rng) % 100);
    if (r < cfg.p_disk) {
        return DEVICE_DISK;
    }
    if (r < cfg.p_disk + cfg.p_tape) {
        return DEVICE_TAPE;
    }
    return DEVICE_PRINTER;
}

static Queue *device_queue(Simulation *s, DeviceType dev) {
    switch (dev) {
    case DEVICE_DISK:
        return &s->disk_queue;
    case DEVICE_TAPE:
        return &s->tape_queue;
    case DEVICE_PRINTER:
        return &s->printer_queue;
    default:
        return &s->disk_queue;
    }
}

static Duration device_duration(SimConfig cfg, DeviceType dev) {
    switch (dev) {
    case DEVICE_DISK:
        return cfg.disk_duration;
    case DEVICE_TAPE:
        return cfg.tape_duration;
    case DEVICE_PRINTER:
        return cfg.printer_duration;
    default:
        return cfg.disk_duration;
    }
}

static IoMode device_io_mode(SimConfig cfg, DeviceType dev) {
    switch (dev) {
    case DEVICE_DISK:
        return cfg.io_mode_disk;
    case DEVICE_TAPE:
        return cfg.io_mode_tape;
    case DEVICE_PRINTER:
        return cfg.io_mode_printer;
    default:
        return cfg.io_mode_disk;
    }
}

/* Return the CPU queue (hi or lo) that a process returning from I/O should
   enter, following the feedback routing rules. */
static Queue *return_queue(Simulation *s, DeviceType dev) {
    if (dev == DEVICE_DISK) {
        return &s->lo_queue;
    }
    return &s->hi_queue; /* tape and printer → high priority */
}

/* -------------------------------------------------------------------------
 * Pending arrivals — dynamic array sorted by arrival_tick
 * ---------------------------------------------------------------------- */

#define INITIAL_CAP 8

static int ensure_cap(void ***arr, int *count, int *cap) {
    if (*count < *cap) {
        return 0;
    }
    int new_cap = (*cap == 0) ? INITIAL_CAP : *cap * 2;
    void **tmp  = realloc(*arr, (size_t)new_cap * sizeof(void *));
    if (!tmp) {
        return -1;
    }
    *arr = tmp;
    *cap = new_cap;
    return 0;
}

/* -------------------------------------------------------------------------
 * Event log helper
 * ---------------------------------------------------------------------- */

static void add_event(Simulation *s, SimEventType type, int pid, int d1, int d2) {
    if (s->event_count >= SIM_MAX_EVENTS) {
        return;
    }
    s->events[s->event_count].type  = type;
    s->events[s->event_count].pid   = pid;
    s->events[s->event_count].data1 = d1;
    s->events[s->event_count].data2 = d2;
    s->event_count++;
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

Simulation *sim_create(SimConfig cfg) {
    Simulation *s = calloc(1, sizeof(Simulation));
    if (!s) {
        return NULL;
    }

    s->cfg       = cfg;
    s->rng_state = cfg.seed;

    queue_init(&s->hi_queue);
    queue_init(&s->lo_queue);
    queue_init(&s->disk_queue);
    queue_init(&s->tape_queue);
    queue_init(&s->printer_queue);

    return s;
}

void sim_destroy(Simulation *s) {
    if (!s) {
        return;
    }

    /* Free all owned processes */
    for (int i = 0; i < s->all_count; i++) {
        process_destroy(s->all_processes[i]);
    }
    free(s->all_processes);
    free(s->pending);

    /* Queues only hold pointers — nodes freed by queue_destroy,
       but the processes themselves were already freed above. */
    queue_destroy(&s->hi_queue);
    queue_destroy(&s->lo_queue);
    queue_destroy(&s->disk_queue);
    queue_destroy(&s->tape_queue);
    queue_destroy(&s->printer_queue);

    free(s);
}

void sim_add_process(Simulation *s, Process *p) {
    /* Register in all_processes (keeps ownership so the process is freed on
       destroy even if it never makes it into the pending list). */
    if (ensure_cap((void ***)&s->all_processes, &s->all_count, &s->all_cap) != 0) {
        return;
    }
    s->all_processes[s->all_count++] = p;

    /* Add to pending arrivals */
    if (ensure_cap((void ***)&s->pending, &s->pending_count, &s->pending_cap) != 0) {
        return;
    }
    s->pending[s->pending_count++] = p;
}

/* -------------------------------------------------------------------------
 * sim_step
 * ---------------------------------------------------------------------- */

void sim_step(Simulation *s) {
    Queue *dev_queues[3]    = { &s->disk_queue, &s->tape_queue, &s->printer_queue };
    DeviceType dev_types[3] = { DEVICE_DISK, DEVICE_TAPE, DEVICE_PRINTER };

    /* Clear per-tick state */
    s->last_preempted  = NULL;
    s->last_completed  = NULL;
    s->last_io_started = NULL;
    s->event_count     = 0;

    /* 1. Process arrivals for this tick — collect, sort by creation_seq, enqueue.
       The scratch array is sized to pending_count, the only upper bound on how
       many can arrive this tick, so there is no fixed cap to overflow. */
    Process **arrived = NULL;
    int n_arrived     = 0;
    if (s->pending_count > 0) {
        arrived = malloc((size_t)s->pending_count * sizeof(Process *));
    }
    if (arrived) {
        for (int i = 0; i < s->pending_count;) {
            Process *p = s->pending[i];
            if (p->arrival_tick == s->tick) {
                arrived[n_arrived++] = p;
                s->pending[i]        = s->pending[--s->pending_count];
            } else {
                i++;
            }
        }

        /* Insertion sort by creation_seq ascending (n is small) */
        for (int i = 1; i < n_arrived; i++) {
            Process *key = arrived[i];
            int j        = i - 1;
            while (j >= 0 && arrived[j]->creation_seq > key->creation_seq) {
                arrived[j + 1] = arrived[j];
                j--;
            }
            arrived[j + 1] = key;
        }

        for (int i = 0; i < n_arrived; i++) {
            queue_enqueue(&s->hi_queue, arrived[i]);
            add_event(s, SIM_EVT_ARRIVED, arrived[i]->pid, 0, 0);
        }

        free(arrived);
    }

    /* 2. Schedule if CPU idle ------------------------------------------- */
    if (!s->running) {
        int from_lo   = 0;
        Process *next = queue_dequeue(&s->hi_queue);
        if (!next) {
            next    = queue_dequeue(&s->lo_queue);
            from_lo = 1;
        }
        if (next) {
            must_set_status(next, PROC_RUNNING);
            s->running      = next;
            s->quantum_used = 0;
            if (next->first_cpu_tick < 0) {
                next->first_cpu_tick = s->tick;
            }
            add_event(s, SIM_EVT_SCHEDULED, next->pid, from_lo, 0);
        }
    }

    /* 3. Tick I/O queues (processes already waiting before this tick ran) -- */
    for (int d = 0; d < 3; d++) {
        Queue *q        = dev_queues[d];
        IoMode mode     = device_io_mode(s->cfg, dev_types[d]);
        QueueNode *node = q->head;
        while (node) {
            Process *p = node->data;
            if (mode == IO_MODE_CONCURRENT) {
                p->io_ticks++;
                if (dev_types[d] == DEVICE_DISK) {
                    p->io_ticks_disk++;
                } else if (dev_types[d] == DEVICE_TAPE) {
                    p->io_ticks_tape++;
                } else {
                    p->io_ticks_printer++;
                }
                p->io_remaining--;
                if (p->io_remaining > 0) {
                    add_event(s, SIM_EVT_IO_TICK, p->pid, (int)dev_types[d], p->io_remaining);
                }
            } else {
                if (node == q->head) {
                    p->io_ticks++;
                    if (dev_types[d] == DEVICE_DISK) {
                        p->io_ticks_disk++;
                    } else if (dev_types[d] == DEVICE_TAPE) {
                        p->io_ticks_tape++;
                    } else {
                        p->io_ticks_printer++;
                    }
                    p->io_remaining--;
                    if (p->io_remaining > 0) {
                        add_event(s, SIM_EVT_IO_TICK, p->pid, (int)dev_types[d], p->io_remaining);
                    }
                }
                break;
            }
            node = node->next;
        }
    }

    /* 4. Promote completed I/O to CPU queues ----------------------------- */
    for (int d = 0; d < 3; d++) {
        Queue *q       = dev_queues[d];
        DeviceType dev = dev_types[d];
        if (q->size == 0) {
            continue;
        }

        /* Scratch array sized to the queue length — no fixed cap to overflow. */
        Process **finished = malloc((size_t)q->size * sizeof(Process *));
        if (!finished) {
            continue;
        }
        int n_finished  = 0;
        QueueNode *node = q->head;
        while (node) {
            Process *p = node->data;
            if (p->io_remaining <= 0) {
                finished[n_finished++] = p;
            }
            node = node->next;
        }
        for (int i = 0; i < n_finished; i++) {
            Process *p      = finished[i];
            QueueNode *prev = NULL;
            QueueNode *cur  = q->head;
            while (cur) {
                if (cur->data == p) {
                    if (prev) {
                        prev->next = cur->next;
                    } else {
                        q->head = cur->next;
                    }
                    if (!cur->next) {
                        q->tail = prev;
                    }
                    free(cur);
                    q->size--;
                    break;
                }
                prev = cur;
                cur  = cur->next;
            }
            Queue *rq = return_queue(s, dev);
            int dest  = (rq == &s->hi_queue) ? 0 : 1;
            must_set_status(p, PROC_READY);
            queue_enqueue(rq, p);
            add_event(s, SIM_EVT_IO_RETURN, p->pid, (int)dev, dest);
        }

        free(finished);
    }

    /* 5. Tick running process ------------------------------------------- */
    if (s->running) {
        Process *p   = s->running;
        int fired_io = 0;

        /* 5a. Check scripted I/O — fires once the process has accrued service_tick
           CPU ticks, before it consumes the next one. The trigger is relative to the
           process's own CPU service, not the global clock, so it is always reachable
           (a process only accrues CPU ticks while running). */
        if (p->io_script && p->io_script_pos < p->io_script_len &&
            p->io_script[p->io_script_pos].service_tick == p->cpu_ticks) {
            ScriptedIO ev  = p->io_script[p->io_script_pos];
            DeviceType dev = ev.device;
            p->io_script_pos++;
            Duration dur    = ev.has_duration ? ev.duration : device_duration(s->cfg, dev);
            p->io_remaining = sample_duration(dur, &s->rng_state);
            p->io_count++;
            must_set_status(p, PROC_BLOCKED);
            queue_enqueue(device_queue(s, dev), p);
            add_event(s, SIM_EVT_IO_START, p->pid, (int)dev, p->io_remaining);
            /* Record for display before clearing state */
            s->last_io_started      = p;
            s->last_io_device       = dev;
            s->last_io_quantum_used = s->quantum_used;
            s->last_io_priority     = p->priority;
            s->running              = NULL;
            fired_io                = 1;
        }

        /* 5b. Check random I/O (fires before the CPU tick) */
        if (!fired_io && rng_roll(&s->rng_state, s->cfg.p_io)) {
            DeviceType dev  = select_device(s->cfg, &s->rng_state);
            p->io_remaining = sample_duration(device_duration(s->cfg, dev), &s->rng_state);
            p->io_count++;
            must_set_status(p, PROC_BLOCKED);
            queue_enqueue(device_queue(s, dev), p);
            add_event(s, SIM_EVT_IO_START, p->pid, (int)dev, p->io_remaining);
            /* Record for display before clearing state */
            s->last_io_started      = p;
            s->last_io_device       = dev;
            s->last_io_quantum_used = s->quantum_used;
            s->last_io_priority     = p->priority;
            s->running              = NULL;
            fired_io                = 1;
        }

        /* 5c. CPU tick runs only when no I/O fired this step */
        if (!fired_io) {
            p->cpu_ticks++;
            s->quantum_used++;

            if (p->cpu_burst_total > 0 && p->cpu_ticks >= p->cpu_burst_total) {
                p->completion_tick = s->tick;
                must_set_status(p, PROC_DONE);
                add_event(s, SIM_EVT_COMPLETED, p->pid, 0, 0);
                /* Record for display before clearing state */
                s->last_completed              = p;
                s->last_completed_quantum_used = s->quantum_used;
                s->last_completed_priority     = p->priority;
                s->running                     = NULL;
                s->quantum_used                = 0;
            } else {
                int quantum =
                    (p->priority == PRIORITY_HIGH) ? s->cfg.quantum_hi : s->cfg.quantum_lo;
                if (s->quantum_used >= quantum) {
                    /* Record for display before clearing state */
                    s->last_preempted          = p;
                    s->last_quantum_used       = s->quantum_used;
                    s->last_quantum_max        = quantum;
                    s->last_preempted_priority = p->priority;
                    add_event(s, SIM_EVT_PREEMPTED, p->pid, s->quantum_used, quantum);
                    must_set_status(p, PROC_READY);
                    p->priority = PRIORITY_LOW;
                    queue_enqueue(&s->lo_queue, p);
                    s->running      = NULL;
                    s->quantum_used = 0;
                }
            }
        }
    }

    /* 6. Advance clock -------------------------------------------------- */
    s->tick++;
}

void sim_run(Simulation *s, int n) {
    for (int i = 0; i < n; i++) {
        sim_step(s);
    }
}

void sim_run_until_done(Simulation *s) {
    while (!sim_is_done(s)) {
        sim_step(s);
    }
}

int sim_is_done(const Simulation *s) {
    if (s->pending_count > 0) {
        return 0;
    }
    for (int i = 0; i < s->all_count; i++) {
        if (s->all_processes[i]->status != PROC_DONE) {
            return 0;
        }
    }
    return 1;
}
