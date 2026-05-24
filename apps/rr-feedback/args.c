#include "args.h"

#include <getopt.h>
#include <stdlib.h>

static SimConfig default_config(void) {
    SimConfig cfg  = { 0 };
    cfg.quantum_hi = 3;
    cfg.quantum_lo = 6;
    cfg.seed       = 42;
    cfg.run_mode   = RUN_BATCH;
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

SimConfig parse_args(int argc, char **argv, int *error) {
    *error        = 0;
    SimConfig cfg = default_config();

    optind = 1;

    int c;
    while ((c = getopt_long(argc, argv, "n:iht", long_opts, NULL)) != -1) {
        switch (c) {
        case 'H':
            cfg.quantum_hi = atoi(optarg);
            break;
        case 'L':
            cfg.quantum_lo = atoi(optarg);
            break;
        case 'c':
            cfg.process_count = atoi(optarg);
            break;
        case 'a':
            cfg.arrival_rate = atoi(optarg);
            break;
        case 'p':
            cfg.p_io = atoi(optarg);
            break;
        case 'D':
            cfg.p_disk = atoi(optarg);
            break;
        case 'T':
            cfg.p_tape = atoi(optarg);
            break;
        case 'P':
            cfg.p_printer = atoi(optarg);
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
        case 4:
            cfg.io_mode_disk = parse_io_mode(optarg);
            break;
        case 5:
            cfg.io_mode_tape = parse_io_mode(optarg);
            break;
        case 6:
            cfg.io_mode_printer = parse_io_mode(optarg);
            break;
        case 's':
            cfg.seed = (unsigned)atoi(optarg);
            break;
        case 'n':
            cfg.run_mode = RUN_STEPS;
            cfg.steps    = atoi(optarg);
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

    return cfg;
}
