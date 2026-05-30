#pragma once

typedef struct {
    int min;
    int max;
} Duration;

typedef enum {
    IO_MODE_CONCURRENT,
    IO_MODE_QUEUE
} IoMode;

typedef enum {
    RUN_BATCH,
    RUN_STEPS,
    RUN_INTERACTIVE
} RunMode;

typedef struct {
    int quantum_hi;       /* default: 3  */
    int quantum_lo;       /* default: 6  */
    int process_count;    /* total processes to generate */
    int arrival_rate;     /* % per tick; 0 = all arrive at tick 0 */
    Duration service_duration; /* CPU burst length per process; {0,0} = no limit */
    int p_io;          /* % chance of I/O per tick (e.g. 20) */
    int p_disk;        /* conditional %; 0 = equal share */
    int p_tape;
    int p_printer;
    Duration disk_duration; /* {5,5} = fixed; {3,8} = range */
    Duration tape_duration;
    Duration printer_duration;
    IoMode io_mode_disk;
    IoMode io_mode_tape;
    IoMode io_mode_printer;
    unsigned seed; /* default: 42 */
    RunMode run_mode;
    int steps;           /* used when run_mode == RUN_STEPS */
    int trace;           /* 1 = print state each tick */
    char *scenario_file; /* NULL if not provided */
} SimConfig;
