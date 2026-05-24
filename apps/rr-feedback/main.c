#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "display.h"

int main(int argc, char *argv[]) {
    int       error = 0;
    SimConfig cfg   = parse_args(argc, argv, &error);

    if (error == 2) {
        printf("Usage: rr-feedback [scenario.txt] [options]\n");
        printf("  --quantum-hi=N        high-priority queue quantum (default: 3)\n");
        printf("  --quantum-lo=N        low-priority queue quantum  (default: 6)\n");
        printf("  --process-count=N     total processes to generate\n");
        printf("  --arrival-rate=N      %% chance of new process per tick (0=all at tick 0)\n");
        printf("  --p-io=N              %% chance of I/O per tick\n");
        printf("  --p-disk/tape/printer conditional I/O device probabilities (must sum to 100)\n");
        printf("  --disk/tape/printer-duration=N or N-M\n");
        printf("  --io-mode-disk/tape/printer=concurrent|queue\n");
        printf("  --seed=N              RNG seed (default: 42)\n");
        printf("  -n N / --steps=N      run N ticks then stop\n");
        printf("  -i / --interactive    advance one tick per Enter\n");
        printf("  --trace               print state each tick\n");
        printf("  -h / --help           show this help\n");
        return 0;
    }

    if (error != 0) {
        fprintf(stderr, "Run with -h for usage.\n");
        return 1;
    }

    print_sim_summary(cfg);
    return 0;
}
