#include "cutesim/rng.h"

unsigned rng_next(unsigned *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

int rng_range(unsigned *state, int lo, int hi) {
    if (lo >= hi) {
        return lo;
    }
    return lo + (int)(rng_next(state) % (unsigned)(hi - lo + 1));
}

int rng_roll(unsigned *state, int percent) {
    if (percent <= 0) {
        return 0;
    }
    if (percent >= 100) {
        return 1;
    }
    return (int)(rng_next(state) % 100) < percent;
}
