#include "display.h"

#include <stdio.h>

#include "cutesim/process.h"
#include "cutesim/queue.h"

static const char *io_mode_str(IoMode mode) {
    return mode == IO_MODE_CONCURRENT ? "concurrent" : "queue";
}

static const char *run_mode_str(RunMode mode) {
    switch (mode) {
    case RUN_BATCH:
        return "batch";
    case RUN_STEPS:
        return "steps";
    case RUN_INTERACTIVE:
        return "interactive";
    default:
        return "batch";
    }
}

void print_help(void) {
    printf("Usage: rr-feedback [scenario.txt] [OPTIONS]\n");
    printf("\n");

    printf("Scheduler\n");
    printf("  --quantum-hi=N          time quantum for the high-priority queue (default: 3)\n");
    printf("  --quantum-lo=N          time quantum for the low-priority queue  (default: 6)\n");
    printf("\n");

    printf("Workload\n");
    printf("  --process-count=N       total number of processes to generate\n");
    printf("  --arrival-rate=N        probability (0-100) of a new process arriving each tick\n");
    printf("                          use 0 to spawn all processes at tick 0\n");
    printf("\n");

    printf("I/O probability\n");
    printf("  --p-io=N                probability (0-100) of an I/O event per tick while on CPU\n");
    printf("                          integer only — floats are rejected\n");
    printf("  --p-disk=N              share of I/O events routed to the disk device\n");
    printf("  --p-tape=N              share of I/O events routed to the tape device\n");
    printf("  --p-printer=N           share of I/O events routed to the printer device\n");
    printf("                          device shares must sum to 100 when all three are given;\n");
    printf("                          if only one or two are given, the remainder is split\n");
    printf("                          evenly among the unspecified devices\n");
    printf("\n");

    printf("I/O duration\n");
    printf("  --disk-duration=N       fixed disk service time in ticks\n");
    printf("  --disk-duration=N-M     random disk service time sampled from [N, M]\n");
    printf("  --tape-duration=N       fixed tape service time in ticks\n");
    printf("  --tape-duration=N-M     random tape service time sampled from [N, M]\n");
    printf("  --printer-duration=N    fixed printer service time in ticks\n");
    printf("  --printer-duration=N-M  random printer service time sampled from [N, M]\n");
    printf("\n");

    printf("I/O execution mode\n");
    printf("  --io-mode-disk=concurrent     all queued disk requests advance one tick each\n");
    printf("  --io-mode-disk=queue          only the head of the disk queue advances (default)\n");
    printf("  --io-mode-tape=concurrent     all queued tape requests advance one tick each\n");
    printf("  --io-mode-tape=queue          only the head of the tape queue advances (default)\n");
    printf("  --io-mode-printer=concurrent  all queued printer requests advance one tick each\n");
    printf("  --io-mode-printer=queue       only the head of the printer queue advances (default)\n");
    printf("\n");

    printf("Execution\n");
    printf("  --seed=N                RNG seed for reproducibility (default: 42)\n");
    printf("  -n N / --steps=N        run exactly N ticks then stop\n");
    printf("  -i / --interactive      advance one tick per Enter keypress\n");
    printf("  --trace                 print full scheduler state after each tick\n");
    printf("\n");

    printf("Scenario file\n");
    printf("  scenario.txt            optional positional argument describing specific\n");
    printf("                          process arrivals and forced I/O events\n");
    printf("\n");

    printf("Other\n");
    printf("  -h / --help             show this help\n");
}

/* -------------------------------------------------------------------------
 * Helpers for trace output
 * ---------------------------------------------------------------------- */

static const char *SEP  = "─────────────────────────────────────────────────────────\n";
static const char *SEP2 = "  ─────────────────────────────────────────────────────\n";

static const char *device_name_pt(int dev) {
    switch (dev) {
    case DEVICE_DISK:    return "disco";
    case DEVICE_TAPE:    return "fita";
    case DEVICE_PRINTER: return "impressora";
    default:             return "?";
    }
}

/* skip: if non-NULL, that process is omitted from the listing.
   Used to hide a just-preempted process from lo_queue on the same tick
   it was preempted — it appears in the CPU line instead. */
static void print_cpu_queue(const Queue *q, const Process *skip) {
    int printed = 0;
    const QueueNode *node = q->head;
    while (node) {
        const Process *p = (const Process *)node->data;
        if (p != skip) {
            if (p->cpu_burst_total > 0) {
                int restante = p->cpu_burst_total - p->cpu_ticks;
                printf("PID=%-2d(rest=%-3d)  ", p->pid, restante);
            } else {
                printf("PID=%-2d             ", p->pid);
            }
            printed++;
        }
        node = node->next;
    }
    if (!printed) printf("[vazia]");
}

static void print_io_queue(const Queue *q) {
    if (q->size == 0) { printf("[vazia]"); return; }
    const QueueNode *node = q->head;
    while (node) {
        const Process *p = (const Process *)node->data;
        printf("PID=%-2d(faltam %-2d)  ", p->pid, p->io_remaining);
        node = node->next;
    }
}

/* -------------------------------------------------------------------------
 * Public API — styled Portuguese output
 * ---------------------------------------------------------------------- */

void print_header(SimConfig cfg) {
    printf("=========================================================\n");
    printf("  Simulador de Escalonamento — Round Robin com Feedback  \n");
    printf("  ICP246 — UFRJ — 2025-1                                 \n");
    printf("=========================================================\n");
    printf("  Modo                     : %s\n", run_mode_str(cfg.run_mode));
    printf("  Processos a criar        : %d\n", cfg.process_count);
    printf("---------------------------------------------------------\n");
    printf("  Quantum alta prioridade  : %d ticks\n", cfg.quantum_hi);
    printf("  Quantum baixa prioridade : %d ticks\n", cfg.quantum_lo);
    if (cfg.service_duration.min > 0)
        printf("  Serviço                  : [%d, %d] ticks\n",
               cfg.service_duration.min, cfg.service_duration.max);
    printf("  Probabilidade de I/O     : %d%%\n", cfg.p_io);
    printf("  I/O disco                : [%d, %d] ticks  (%s)\n",
           cfg.disk_duration.min, cfg.disk_duration.max,
           io_mode_str(cfg.io_mode_disk));
    printf("  I/O fita                 : [%d, %d] ticks  (%s)\n",
           cfg.tape_duration.min, cfg.tape_duration.max,
           io_mode_str(cfg.io_mode_tape));
    printf("  I/O impressora           : [%d, %d] ticks  (%s)\n",
           cfg.printer_duration.min, cfg.printer_duration.max,
           io_mode_str(cfg.io_mode_printer));
    printf("  Semente RNG              : %u\n", cfg.seed);
    printf("=========================================================\n");
}

void print_tick_trace(const Simulation *s) {
    int tick = s->tick - 1; /* tick that just ran */

    printf("\n%s", SEP);
    printf("  TICK %d\n", tick);
    printf("%s", SEP);

    /* CPU */
    printf("  CPU           : ");
    if (s->running) {
        const Process *p       = s->running;
        int quantum_max        = (p->priority == PRIORITY_HIGH)
                                 ? s->cfg.quantum_hi : s->cfg.quantum_lo;
        const char *fila_nome  = (p->priority == PRIORITY_HIGH) ? "ALTA" : "BAIXA";
        if (p->cpu_burst_total > 0) {
            int restante = p->cpu_burst_total - p->cpu_ticks;
            printf("PID=%-2d  restante=%-3d  quantum=%d/%d  fila=%s\n",
                   p->pid, restante, s->quantum_used, quantum_max, fila_nome);
        } else {
            printf("PID=%-2d  quantum=%d/%d  fila=%s\n",
                   p->pid, s->quantum_used, quantum_max, fila_nome);
        }
    } else if (s->last_preempted) {
        /* Show the quantum=N/N moment that caused preemption */
        const Process *p      = s->last_preempted;
        const char *fila_nome = (s->last_preempted_priority == PRIORITY_HIGH) ? "ALTA" : "BAIXA";
        if (p->cpu_burst_total > 0) {
            int restante = p->cpu_burst_total - p->cpu_ticks;
            printf("PID=%-2d  restante=%-3d  quantum=%d/%d  fila=%s  [preemptado]\n",
                   p->pid, restante, s->last_quantum_used, s->last_quantum_max, fila_nome);
        } else {
            printf("PID=%-2d  quantum=%d/%d  fila=%s  [preemptado]\n",
                   p->pid, s->last_quantum_used, s->last_quantum_max, fila_nome);
        }
    } else {
        printf("[ocioso]\n");
    }

    printf("\n");
    printf("  FILA ALTA     : "); print_cpu_queue(&s->hi_queue, NULL);               printf("\n");
    printf("  FILA BAIXA    : "); print_cpu_queue(&s->lo_queue, s->last_preempted); printf("\n");
    printf("\n");
    printf("  I/O DISCO     : "); print_io_queue(&s->disk_queue);   printf("\n");
    printf("  I/O FITA      : "); print_io_queue(&s->tape_queue);   printf("\n");
    printf("  I/O IMPRESSORA: "); print_io_queue(&s->printer_queue);printf("\n");
    printf("\n");

    /* Completed processes */
    printf("  CONCLUÍDOS    : ");
    int any_done = 0;
    for (int i = 0; i < s->all_count; i++) {
        if (s->all_processes[i]->status == PROC_DONE) {
            printf("PID=%-2d  ", s->all_processes[i]->pid);
            any_done = 1;
        }
    }
    if (!any_done) printf("nenhum");
    printf("\n");

    /* Event log */
    printf("\n  LOG\n");
    printf("%s", SEP2);
    if (s->event_count == 0) {
        printf("  (nenhum evento)\n");
    } else {
        for (int i = 0; i < s->event_count; i++) {
            const SimEvent *ev = &s->events[i];
            printf("  ");
            switch (ev->type) {
            case SIM_EVT_ARRIVED:
                printf("P%d chegou → fila alta\n", ev->pid);
                break;
            case SIM_EVT_SCHEDULED:
                printf("P%d entrou na CPU (fila %s)\n",
                       ev->pid, ev->data1 == 0 ? "alta" : "baixa");
                break;
            case SIM_EVT_PREEMPTED:
                printf("P%d preemptado (quantum %d/%d) → fila baixa\n",
                       ev->pid, ev->data1, ev->data2);
                break;
            case SIM_EVT_IO_START:
                printf("P%d → I/O %s iniciado (durará %d tick%s)\n",
                       ev->pid, device_name_pt(ev->data1), ev->data2,
                       ev->data2 == 1 ? "" : "s");
                break;
            case SIM_EVT_IO_TICK:
                printf("P%d [%s] processou 1 tick, faltam %d\n",
                       ev->pid, device_name_pt(ev->data1), ev->data2);
                break;
            case SIM_EVT_IO_RETURN:
                printf("P%d [%s] I/O concluído → fila %s\n",
                       ev->pid, device_name_pt(ev->data1),
                       ev->data2 == 0 ? "alta" : "baixa");
                break;
            case SIM_EVT_COMPLETED:
                printf("P%d concluído\n", ev->pid);
                break;
            }
        }
    }
}

void print_sim_done(const Simulation *s) {
    printf("\n%s", SEP);
    if (sim_is_done(s)) {
        printf("  SIMULAÇÃO CONCLUÍDA — tick %d\n", s->tick);
    } else {
        printf("  SIMULAÇÃO PAUSADA   — tick %d\n", s->tick);
    }
    printf("%s", SEP);

    int done = 0;
    for (int i = 0; i < s->all_count; i++) {
        if (s->all_processes[i]->status == PROC_DONE) done++;
    }
    printf("  Processos concluídos : %d de %d\n", done, s->all_count);
    printf("%s\n", SEP);
}

void print_sim_summary(SimConfig cfg) {
    printf("=== rr-feedback — simulation parameters ===\n");
    printf("scheduler     quantum-hi=%d   quantum-lo=%d\n", cfg.quantum_hi, cfg.quantum_lo);
    printf("processes     count=%d        arrival-rate=%d%%/tick\n", cfg.process_count,
           cfg.arrival_rate);
    printf("i/o           p-io=%d%%        disk=%d%%   tape=%d%%   printer=%d%%\n", cfg.p_io,
           cfg.p_disk, cfg.p_tape, cfg.p_printer);
    printf("durations     disk=%d-%d       tape=%d-%d    printer=%d-%d\n", cfg.disk_duration.min,
           cfg.disk_duration.max, cfg.tape_duration.min, cfg.tape_duration.max,
           cfg.printer_duration.min, cfg.printer_duration.max);
    printf("i/o mode      disk=%s  tape=%s  printer=%s\n", io_mode_str(cfg.io_mode_disk),
           io_mode_str(cfg.io_mode_tape), io_mode_str(cfg.io_mode_printer));
    printf("execution     mode=%s     seed=%u   trace=%s\n", run_mode_str(cfg.run_mode), cfg.seed,
           cfg.trace ? "on" : "off");
    printf("============================================\n");
}
