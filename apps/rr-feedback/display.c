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
