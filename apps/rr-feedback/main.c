#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "display.h"
#include "cutesim/simulation.h"

/* -------------------------------------------------------------------------
 * LCG — same parameters as simulation.c; used independently for arrivals
 * ---------------------------------------------------------------------- */

static unsigned lcg_next(unsigned *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

static int lcg_range(unsigned *state, int lo, int hi) {
    if (lo >= hi) return lo;
    return lo + (int)(lcg_next(state) % (unsigned)(hi - lo + 1));
}

/* -------------------------------------------------------------------------
 * Process generation
 * ---------------------------------------------------------------------- */

static void spawn_processes(SimConfig cfg, Simulation *sim) {
    unsigned rng     = cfg.seed;
    int      spawned = 0;

    if (cfg.arrival_rate == 0) {
        /* All processes arrive at tick 0 */
        for (int i = 0; i < cfg.process_count; i++) {
            Process *p = process_create(i + 1, 0, i);
            if (cfg.service_duration.min > 0) {
                p->cpu_burst_total = lcg_range(&rng,
                                               cfg.service_duration.min,
                                               cfg.service_duration.max);
            }
            sim_add_process(sim, p);
        }
        return;
    }

    /* Stagger arrivals: roll arrival_rate% chance each tick */
    int tick = 0;
    while (spawned < cfg.process_count) {
        if ((int)(lcg_next(&rng) % 100) < cfg.arrival_rate) {
            Process *p = process_create(spawned + 1, tick, spawned);
            if (cfg.service_duration.min > 0) {
                p->cpu_burst_total = lcg_range(&rng,
                                               cfg.service_duration.min,
                                               cfg.service_duration.max);
            }
            sim_add_process(sim, p);
            spawned++;
        }
        tick++;
        if (tick > 100000) break; /* safety cap */
    }
    /* Any stragglers get spawned at the last tick */
    while (spawned < cfg.process_count) {
        Process *p = process_create(spawned + 1, tick, spawned);
        if (cfg.service_duration.min > 0) {
            p->cpu_burst_total = lcg_range(&rng,
                                           cfg.service_duration.min,
                                           cfg.service_duration.max);
        }
        sim_add_process(sim, p);
        spawned++;
    }
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(int argc, char *argv[]) {
    int       error = 0;
    SimConfig cfg   = parse_args(argc, argv, &error);

    if (error == 2) { print_help(); return 0; }
    if (error != 0) { fprintf(stderr, "Run with -h for usage.\n"); return 1; }

    print_header(cfg);

    Simulation *sim = sim_create(cfg);
    if (!sim) { fprintf(stderr, "error: failed to create simulation\n"); return 1; }

    spawn_processes(cfg, sim);

    if (cfg.run_mode == RUN_INTERACTIVE) {
        printf("\nPressione Enter para avançar um tick, 'q' + Enter para sair.\n");
        char line[16];
        while (!sim_is_done(sim)) {
            sim_step(sim);
            print_tick_trace(sim);
            printf("\n  [tick %d] > ", sim->tick);
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin)) break;
            if (line[0] == 'q' || line[0] == 'Q') break;
        }
    } else if (cfg.run_mode == RUN_STEPS) {
        for (int i = 0; i < cfg.steps; i++) {
            sim_step(sim);
            if (cfg.trace) print_tick_trace(sim);
        }
    } else { /* RUN_BATCH */
        while (!sim_is_done(sim)) {
            sim_step(sim);
            if (cfg.trace) print_tick_trace(sim);
        }
    }

    print_sim_done(sim);
    sim_destroy(sim);
    return 0;
}
