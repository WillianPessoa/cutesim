#include "display.h"

#include <stdio.h>

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
