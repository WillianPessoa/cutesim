#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "display.h"

int main(int argc, char *argv[]) {
    int       error = 0;
    SimConfig cfg   = parse_args(argc, argv, &error);

    if (error == 2) {
        print_help();
        return 0;
    }

    if (error != 0) {
        fprintf(stderr, "Run with -h for usage.\n");
        return 1;
    }

    print_sim_summary(cfg);
    return 0;
}
