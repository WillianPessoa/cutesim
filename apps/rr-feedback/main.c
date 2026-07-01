#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "cutesim/rng.h"
#include "cutesim/scenario.h"
#include "cutesim/simulation.h"
#include "display.h"
#include "emit_file.h"
#include "emit_tcp.h"

/* -------------------------------------------------------------------------
 * Process generation uses cutesim/rng.h with its own state word, kept
 * independent from the simulation engine's RNG so arrivals and in-sim rolls
 * never interfere.
 * ---------------------------------------------------------------------- */

/* Knuth algorithm: returns a Poisson-distributed random integer with mean lambda.
   Uses the LCG to generate uniform samples in (0,1]. */
static int poisson_sample(unsigned *state, double lambda) {
    double L  = exp(-lambda);
    double p  = 1.0;
    int count = 0;
    do {
        count++;
        double u = (rng_next(state) + 1.0) / 4294967296.0;
        p *= u;
    } while (p > L);
    return count - 1;
}

/* -------------------------------------------------------------------------
 * Process generation
 * ---------------------------------------------------------------------- */

static void spawn_one(SimConfig cfg, Simulation *sim, unsigned *rng, int pid, int tick, int seq) {
    Process *p = process_create(pid, tick, seq);
    if (cfg.service_duration.min > 0) {
        p->cpu_burst_total = rng_range(rng, cfg.service_duration.min, cfg.service_duration.max);
    }
    sim_add_process(sim, p);
}

static void spawn_processes(SimConfig cfg, Simulation *sim) {
    unsigned rng = cfg.seed;
    int spawned  = 0;

    switch (cfg.arrival_mode) {

    case ARRIVAL_BATCH:
        for (int i = 0; i < cfg.process_count; i++) {
            spawn_one(cfg, sim, &rng, i + 1, 0, i);
        }
        return;

    case ARRIVAL_UNIFORM:
        for (int i = 0; i < cfg.process_count; i++) {
            spawn_one(cfg, sim, &rng, i + 1, i * cfg.arrival_interval, i);
        }
        return;

    case ARRIVAL_POISSON: {
        int tick = 0;
        while (spawned < cfg.process_count) {
            int arrivals = poisson_sample(&rng, cfg.arrival_lambda);
            for (int k = 0; k < arrivals && spawned < cfg.process_count; k++) {
                spawn_one(cfg, sim, &rng, spawned + 1, tick, spawned);
                spawned++;
            }
            tick++;
            if (tick > 1000000) {
                break;
            }
        }
        while (spawned < cfg.process_count) {
            spawn_one(cfg, sim, &rng, spawned + 1, tick, spawned);
            spawned++;
        }
        return;
    }

    case ARRIVAL_BERNOULLI:
    case ARRIVAL_GEOMETRIC: {
        int tick = 0;
        while (spawned < cfg.process_count) {
            if ((int)(rng_next(&rng) % 100) < cfg.arrival_rate) {
                spawn_one(cfg, sim, &rng, spawned + 1, tick, spawned);
                spawned++;
            }
            tick++;
            if (tick > 100000) {
                break;
            }
        }
        while (spawned < cfg.process_count) {
            spawn_one(cfg, sim, &rng, spawned + 1, tick, spawned);
            spawned++;
        }
        return;
    }
    }
}

/* Spawn the scripted processes from a parsed scenario. Each process gets its own
   heap copy of the I/O timeline (owned by the Process, freed on sim_destroy). */
static void spawn_scripted(const Scenario *sc, Simulation *sim) {
    for (int i = 0; i < sc->process_count; i++) {
        const ScriptedProcess *sp = &sc->processes[i];
        Process *p                = process_create(i + 1, sp->arrival_tick, i);
        if (!p) {
            continue;
        }
        p->cpu_burst_total = sp->burst;
        if (sp->io_count > 0) {
            p->io_script = malloc((size_t)sp->io_count * sizeof(ScriptedIO));
            if (p->io_script) {
                memcpy(p->io_script, sp->io, (size_t)sp->io_count * sizeof(ScriptedIO));
                p->io_script_len = sp->io_count;
            }
        }
        sim_add_process(sim, p);
    }
}

/* -------------------------------------------------------------------------
 * TCP serve adapter
 *
 * emit_tcp_serve owns the Simulation and recreates it on `reset`; it calls
 * back here to (re)populate a fresh sim with the same processes main would
 * have spawned, so scripted and random scenarios both work over the wire.
 * ---------------------------------------------------------------------- */

typedef struct {
    SimConfig       cfg;
    const Scenario *scenario;
    int             scripted;
} SpawnCtx;

static void spawn_adapter(Simulation *sim, void *ctx) {
    SpawnCtx *c = (SpawnCtx *)ctx;
    if (c->scripted) {
        spawn_scripted(c->scenario, sim);
    } else {
        spawn_processes(c->cfg, sim);
    }
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(int argc, char *argv[]) {
    int error     = 0;
    SimConfig cfg = parse_args(argc, argv, &error);

    if (error == 2) {
        print_help();
        return 0;
    }
    if (error != 0) {
        fprintf(stderr, "Run with -h for usage.\n");
        return 1;
    }

    /* A scenario file supplies global config and (optionally) scripted processes.
       Its values override the defaults/flags parsed above. */
    Scenario scenario = { 0 };
    int scripted      = 0;
    if (cfg.scenario_file) {
        scenario.config = cfg;
        if (scenario_parse_file(cfg.scenario_file, &scenario) != 0) {
            fprintf(stderr, "error: failed to parse scenario '%s'\n", cfg.scenario_file);
            scenario_free(&scenario);
            return 1;
        }
        cfg      = scenario.config;
        scripted = scenario.process_count > 0;
        if (scripted) {
            cfg.process_count = scenario.process_count;
        }
    }

    /* Serve mode takes over the run loop: no local stepping, no file emit. */
    if (cfg.serve_port != 0) {
        print_header(cfg, scripted);
        SpawnCtx sctx = { cfg, &scenario, scripted };
        int rc        = emit_tcp_serve(cfg, spawn_adapter, &sctx, cfg.serve_port);
        scenario_free(&scenario);
        return rc == 0 ? 0 : 1;
    }

    FILE *emit_f = NULL;
    if (cfg.emit_file) {
        emit_f = fopen(cfg.emit_file, "w");
        if (!emit_f) {
            fprintf(stderr, "error: cannot open '%s' for writing\n", cfg.emit_file);
            scenario_free(&scenario);
            return 1;
        }
    }

    print_header(cfg, scripted);

    Simulation *sim = sim_create(cfg);
    if (!sim) {
        fprintf(stderr, "error: failed to create simulation\n");
        scenario_free(&scenario);
        return 1;
    }

    if (scripted) {
        spawn_scripted(&scenario, sim);
    } else {
        spawn_processes(cfg, sim);
    }

    if (cfg.run_mode == RUN_INTERACTIVE) {
        printf("\nPressione Enter para avançar um tick, 'q' + Enter para sair.\n");
        char line[16];
        while (!sim_is_done(sim)) {
            sim_step(sim);
            if (emit_f) { emit_file_write(emit_f, sim); }
            print_tick_trace(sim);
            printf("\n  [tick %d] > ", sim->tick);
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin)) {
                break;
            }
            if (line[0] == 'q' || line[0] == 'Q') {
                break;
            }
        }
    } else if (cfg.run_mode == RUN_STEPS) {
        for (int i = 0; i < cfg.steps; i++) {
            sim_step(sim);
            if (emit_f) { emit_file_write(emit_f, sim); }
            if (cfg.trace) {
                print_tick_trace(sim);
            }
        }
    } else { /* RUN_BATCH */
        while (!sim_is_done(sim)) {
            sim_step(sim);
            if (emit_f) { emit_file_write(emit_f, sim); }
            if (cfg.trace) {
                print_tick_trace(sim);
            }
        }
    }

    if (emit_f) { fclose(emit_f); }
    print_sim_done(sim);
    print_sim_statistics(sim);
    sim_destroy(sim);
    scenario_free(&scenario);
    return 0;
}
