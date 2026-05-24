#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "display.h"

int main(int argc, char *argv[]) {
    int       error = 0;
    SimConfig cfg   = parse_args(argc, argv, &error);
    (void)cfg;
    (void)error;
    return 0;
}
