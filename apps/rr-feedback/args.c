#include "args.h"

#include <getopt.h>
#include <stdlib.h>

static SimConfig default_config(void) {
    SimConfig cfg          = { 0 };
    cfg.quantum_hi         = 3;
    cfg.quantum_lo         = 6;
    cfg.seed               = 42;
    cfg.run_mode           = RUN_BATCH;
    cfg.process_count      = 5;
    cfg.service_duration   = (Duration){ 5, 15 };
    return cfg;
}

static struct option long_opts[] = { { "quantum-hi", required_argument, 0, 'H' },
                                     { "quantum-lo", required_argument, 0, 'L' },
                                     { "process-count", required_argument, 0, 'c' },
                                     { "arrival-rate", required_argument, 0, 'a' },
                                     { "p-io", required_argument, 0, 'p' },
                                     { "p-disk", required_argument, 0, 'D' },
                                     { "p-tape", required_argument, 0, 'T' },
                                     { "p-printer", required_argument, 0, 'P' },
                                     { "disk-duration", required_argument, 0, 1 },
                                     { "tape-duration", required_argument, 0, 2 },
                                     { "printer-duration", required_argument, 0, 3 },
                                     { "service-duration", required_argument, 0, 7 },
                                     { "io-mode-disk", required_argument, 0, 4 },
                                     { "io-mode-tape", required_argument, 0, 5 },
                                     { "io-mode-printer", required_argument, 0, 6 },
                                     { "seed", required_argument, 0, 's' },
                                     { "steps", required_argument, 0, 'n' },
                                     { "trace", no_argument, 0, 't' },
                                     { "interactive", no_argument, 0, 'i' },
                                     { "help", no_argument, 0, 'h' },
                                     { 0, 0, 0, 0 } };

static Duration parse_duration(const char *str) {
    Duration d = { 0, 0 };
    char *end  = NULL;

    long lo = strtol(str, &end, 10);
    if (end == str) {
        return d;
    }

    d.min = (int)lo;
    d.max = (int)lo;

    if (*end == '-') {
        long hi = strtol(end + 1, &end, 10);
        if (*end == '\0') {
            d.max = (int)hi;
        }
    }

    return d;
}

static IoMode parse_io_mode(const char *str) {
    if (str && str[0] == 'q') {
        return IO_MODE_QUEUE;
    }
    return IO_MODE_CONCURRENT;
}

/* Returns 0 and stores the value in *out when str is a clean base-10 integer.
   Returns -1 on floats, trailing junk, or empty input. */
static int parse_int_strict(const char *str, int *out) {
    char *end = NULL;
    long  val = strtol(str, &end, 10);
    if (end == str || *end != '\0') {
        return -1;
    }
    *out = (int)val;
    return 0;
}

SimConfig parse_args(int argc, char **argv, int *error) {
    *error          = 0;
    SimConfig cfg   = default_config();
    int disk_set    = 0;
    int tape_set    = 0;
    int printer_set = 0;

    optind = 1;
    opterr = 0; /* suppress getopt's own error messages; we handle '?' ourselves */

    int c;
    while ((c = getopt_long(argc, argv, "n:iht", long_opts, NULL)) != -1) {
        switch (c) {
        case 'H':
            if (parse_int_strict(optarg, &cfg.quantum_hi) != 0) { *error = 1; return cfg; }
            break;
        case 'L':
            if (parse_int_strict(optarg, &cfg.quantum_lo) != 0) { *error = 1; return cfg; }
            break;
        case 'c':
            if (parse_int_strict(optarg, &cfg.process_count) != 0) { *error = 1; return cfg; }
            break;
        case 'a':
            if (parse_int_strict(optarg, &cfg.arrival_rate) != 0) { *error = 1; return cfg; }
            break;
        case 'p':
            if (parse_int_strict(optarg, &cfg.p_io) != 0) { *error = 1; return cfg; }
            break;
        case 'D':
            if (parse_int_strict(optarg, &cfg.p_disk) != 0) { *error = 1; return cfg; }
            disk_set = 1;
            break;
        case 'T':
            if (parse_int_strict(optarg, &cfg.p_tape) != 0) { *error = 1; return cfg; }
            tape_set = 1;
            break;
        case 'P':
            if (parse_int_strict(optarg, &cfg.p_printer) != 0) { *error = 1; return cfg; }
            printer_set = 1;
            break;
        case 1:
            cfg.disk_duration = parse_duration(optarg);
            break;
        case 2:
            cfg.tape_duration = parse_duration(optarg);
            break;
        case 3:
            cfg.printer_duration = parse_duration(optarg);
            break;
        case 7:
            cfg.service_duration = parse_duration(optarg);
            break;
        case 4:
            cfg.io_mode_disk = parse_io_mode(optarg);
            break;
        case 5:
            cfg.io_mode_tape = parse_io_mode(optarg);
            break;
        case 6:
            cfg.io_mode_printer = parse_io_mode(optarg);
            break;
        case 's': {
            int v;
            if (parse_int_strict(optarg, &v) != 0) { *error = 1; return cfg; }
            cfg.seed = (unsigned)v;
            break;
        }
        case 'n':
            if (parse_int_strict(optarg, &cfg.steps) != 0) { *error = 1; return cfg; }
            cfg.run_mode = RUN_STEPS;
            break;
        case 't':
            cfg.trace = 1;
            break;
        case 'i':
            cfg.run_mode = RUN_INTERACTIVE;
            break;
        case 'h':
            *error = 2;
            return cfg;
        case '?':
            *error = 1;
            return cfg;
        default:
            *error = 1;
            return cfg;
        }
    }

    if (optind < argc) {
        cfg.scenario_file = argv[optind];
    }

    /* --- post-parse validation --- */
    if (cfg.quantum_hi <= 0 || cfg.quantum_lo <= 0) {
        *error = 1;
        return cfg;
    }

    if (cfg.p_io < 0 || cfg.p_io > 100) {
        *error = 1;
        return cfg;
    }

    /* device probability redistribution:
       - 0 explicit: nothing to redistribute (no I/O configured)
       - 1 or 2 explicit: remaining probability is split evenly among unset devices;
         when the remainder is odd, the extra goes to the first unset device
         in disk -> tape -> printer order
       - 3 explicit: must sum to exactly 100 */
    int n_explicit = disk_set + tape_set + printer_set;
    if (n_explicit > 0 && n_explicit < 3) {
        int assigned = (disk_set ? cfg.p_disk : 0)
                     + (tape_set ? cfg.p_tape : 0)
                     + (printer_set ? cfg.p_printer : 0);
        int rem = 100 - assigned;
        if (rem < 0) {
            *error = 1;
            return cfg;
        }
        int n_unset  = 3 - n_explicit;
        int base     = rem / n_unset;
        int leftover = rem % n_unset;
        if (!disk_set) {
            cfg.p_disk = base + (leftover > 0 ? 1 : 0);
            if (leftover > 0) {
                leftover--;
            }
        }
        if (!tape_set) {
            cfg.p_tape = base + (leftover > 0 ? 1 : 0);
            if (leftover > 0) {
                leftover--;
            }
        }
        if (!printer_set) {
            cfg.p_printer = base;
        }
    } else if (n_explicit == 3) {
        if (cfg.p_disk + cfg.p_tape + cfg.p_printer != 100) {
            *error = 1;
            return cfg;
        }
    }

    if (cfg.disk_duration.min > cfg.disk_duration.max ||
        cfg.tape_duration.min > cfg.tape_duration.max ||
        cfg.printer_duration.min > cfg.printer_duration.max) {
        *error = 1;
        return cfg;
    }

    return cfg;
}
