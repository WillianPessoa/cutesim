#include "snapshot.h"

#include <stdarg.h>
#include <stdio.h>

#include "cutesim/statistics.h"

/* -------------------------------------------------------------------------
 * Write helper — accumulates bytes like snprintf; tracks overflow.
 * ---------------------------------------------------------------------- */

typedef struct {
    char *buf;
    int   pos;
    int   cap;
} Wr;

static void wr(Wr *w, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rem = w->cap - w->pos;
    /* vsnprintf(NULL, 0, ...) is valid C and returns the count — used for
       overflow tracking once the buffer is full. */
    char *dest     = (rem > 0) ? w->buf + w->pos : NULL;
    size_t dest_sz = (rem > 0) ? (size_t)rem : 0;
    int n          = vsnprintf(dest, dest_sz, fmt, ap);
    va_end(ap);
    if (n > 0) {
        w->pos += n;
    }
}

/* -------------------------------------------------------------------------
 * String tables
 * ---------------------------------------------------------------------- */

static const char *event_type_str(SimEventType t) {
    switch (t) {
    case SIM_EVT_ARRIVED:   return "arrived";
    case SIM_EVT_SCHEDULED: return "scheduled";
    case SIM_EVT_PREEMPTED: return "preempted";
    case SIM_EVT_IO_START:  return "io_start";
    case SIM_EVT_IO_TICK:   return "io_tick";
    case SIM_EVT_IO_RETURN: return "io_return";
    case SIM_EVT_COMPLETED: return "completed";
    default:                return "unknown";
    }
}

static const char *device_str(int d) {
    switch (d) {
    case DEVICE_DISK:    return "disk";
    case DEVICE_TAPE:    return "tape";
    case DEVICE_PRINTER: return "printer";
    default:             return "unknown";
    }
}

static const char *queue_str(int priority) {
    return (priority == PRIORITY_HIGH) ? "high" : "low";
}

/* -------------------------------------------------------------------------
 * Section writers
 * ---------------------------------------------------------------------- */

static void write_cpu_queue(Wr *w, const Queue *q) {
    wr(w, "[");
    const QueueNode *node = q->head;
    int first             = 1;
    while (node) {
        const Process *p = (const Process *)node->data;
        if (!first) {
            wr(w, ",");
        }
        wr(w, "{\"pid\":%d,\"remaining\":%d}", p->pid, p->quantum_remaining);
        first = 0;
        node  = node->next;
    }
    wr(w, "]");
}

static void write_io_queue(Wr *w, const Queue *q) {
    wr(w, "[");
    const QueueNode *node = q->head;
    int first             = 1;
    while (node) {
        const Process *p = (const Process *)node->data;
        if (!first) {
            wr(w, ",");
        }
        wr(w, "{\"pid\":%d,\"io_remaining\":%d}", p->pid, p->io_remaining);
        first = 0;
        node  = node->next;
    }
    wr(w, "]");
}

static void write_event(Wr *w, const SimEvent *e) {
    wr(w, "{\"type\":\"%s\",\"pid\":%d", event_type_str(e->type), e->pid);
    switch (e->type) {
    case SIM_EVT_SCHEDULED:
        wr(w, ",\"queue\":\"%s\"", queue_str(e->data1));
        break;
    case SIM_EVT_PREEMPTED:
        wr(w, ",\"quantum_used\":%d,\"quantum_max\":%d", e->data1, e->data2);
        break;
    case SIM_EVT_IO_START:
        wr(w, ",\"device\":\"%s\",\"io_remaining\":%d", device_str(e->data1), e->data2);
        break;
    case SIM_EVT_IO_TICK:
        wr(w, ",\"device\":\"%s\",\"remaining\":%d", device_str(e->data1), e->data2);
        break;
    case SIM_EVT_IO_RETURN:
        wr(w, ",\"device\":\"%s\",\"queue\":\"%s\"", device_str(e->data1), queue_str(e->data2));
        break;
    default:
        break;
    }
    wr(w, "}");
}

static void write_finished(Wr *w, const Process *p) {
    ProcStats st = stats_for_process(p);
    wr(w,
       "{\"pid\":%d,\"arrival_tick\":%d,\"finish_tick\":%d,"
       "\"service_time\":%d,\"cpu_time_used\":%d,"
       "\"io_disk\":%d,\"io_tape\":%d,\"io_printer\":%d,\"io_total\":%d,\"io_count\":%d,"
       "\"wait_time\":%d,\"turnaround\":%d,\"response_time\":%d}",
       st.pid, st.arrival, p->completion_tick,
       st.service, st.service,
       st.io_ticks_disk, st.io_ticks_tape, st.io_ticks_printer, st.io_ticks, st.io_count,
       st.waiting, st.turnaround, st.response);
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

int snapshot_to_json(const Simulation *s, char *buf, size_t bufsz) {
    Wr w = { buf, 0, (int)bufsz };

    wr(&w, "{\"tick\":%d,", s->tick);

    /* cpu */
    if (s->running) {
        int qmax = (s->running->priority == PRIORITY_HIGH) ? s->cfg.quantum_hi
                                                           : s->cfg.quantum_lo;
        wr(&w,
           "\"cpu\":{\"pid\":%d,\"remaining\":%d,\"quantum_used\":%d,\"quantum_max\":%d,"
           "\"queue\":\"%s\"},",
           s->running->pid, s->running->quantum_remaining, s->quantum_used, qmax,
           queue_str(s->running->priority));
    } else {
        wr(&w, "\"cpu\":null,");
    }

    /* queues */
    wr(&w, "\"queues\":{");
    wr(&w, "\"high\":");
    write_cpu_queue(&w, &s->hi_queue);
    wr(&w, ",\"low\":");
    write_cpu_queue(&w, &s->lo_queue);
    wr(&w, ",\"disk\":");
    write_io_queue(&w, &s->disk_queue);
    wr(&w, ",\"tape\":");
    write_io_queue(&w, &s->tape_queue);
    wr(&w, ",\"printer\":");
    write_io_queue(&w, &s->printer_queue);
    wr(&w, "},");

    /* events */
    wr(&w, "\"events\":[");
    for (int i = 0; i < s->event_count; i++) {
        if (i > 0) {
            wr(&w, ",");
        }
        write_event(&w, &s->events[i]);
    }
    wr(&w, "],");

    /* finished processes */
    wr(&w, "\"finished\":[");
    int first_done = 1;
    for (int i = 0; i < s->all_count; i++) {
        const Process *p = s->all_processes[i];
        if (p->status == PROC_DONE) {
            if (!first_done) {
                wr(&w, ",");
            }
            write_finished(&w, p);
            first_done = 0;
        }
    }
    wr(&w, "],");

    /* stats */
    SimStats st = stats_compute(
        (const Process *const *)s->all_processes, s->all_count, s->tick);
    wr(&w,
       "\"stats\":{\"cpu_utilization\":%.4f,\"throughput\":%.4f,"
       "\"avg_turnaround\":%.4f,\"avg_waiting\":%.4f,\"avg_response\":%.4f},",
       st.cpu_utilization, st.throughput, st.avg_turnaround, st.avg_waiting,
       st.avg_response);

    /* done */
    wr(&w, "\"done\":%s}", sim_is_done(s) ? "true" : "false");

    return w.pos;
}
