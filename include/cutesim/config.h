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

typedef enum {
    ARRIVAL_BATCH,     /* all processes arrive at tick 0 (default) */
    ARRIVAL_BERNOULLI, /* p% chance of arrival each tick (--arrival-rate) */
    ARRIVAL_GEOMETRIC, /* alias for BERNOULLI */
    ARRIVAL_POISSON,   /* Poisson arrivals/tick with mean lambda (--arrival-lambda) */
    ARRIVAL_UNIFORM    /* one arrival every N ticks (--arrival-interval) */
} ArrivalMode;

typedef struct {
    int quantum_hi;            /* default: 3  */
    int quantum_lo;            /* default: 6  */
    int process_count;         /* total processes to generate */
    int arrival_rate;          /* % per tick; used by BERNOULLI/GEOMETRIC */
    ArrivalMode arrival_mode;  /* default: ARRIVAL_BATCH */
    double arrival_lambda;     /* mean arrivals/tick for POISSON */
    int arrival_interval;      /* ticks between arrivals for UNIFORM */
    Duration service_duration; /* CPU burst length per process; {0,0} = no limit */
    int p_io;                  /* % chance of I/O per tick (e.g. 20) */
    int p_disk;                /* conditional %; 0 = equal share */
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
    char *emit_file;     /* NULL if not provided; path for --emit-file */
    int serve_port;      /* TCP command server: 0 = off, -1 = on (default port),
                            >0 = on (explicit port). Set by --serve[=PORT]. */
} SimConfig;
