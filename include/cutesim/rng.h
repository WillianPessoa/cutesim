#pragma once

/* Linear congruential generator (Numerical Recipes constants).
   The caller owns the state word, so independent streams stay independent:
   the simulation engine and the arrival generator each keep their own state. */

/* Advance the generator and return the new raw 32-bit value. */
unsigned rng_next(unsigned *state);

/* Return a uniformly distributed integer in [lo, hi] (inclusive).
   Returns lo when lo >= hi. */
int rng_range(unsigned *state, int lo, int hi);

/* Return 1 with probability percent/100, else 0.
   percent <= 0 -> always 0; percent >= 100 -> always 1. */
int rng_roll(unsigned *state, int percent);
