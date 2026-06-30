#pragma once

#include <stdio.h>

#include "cutesim/simulation.h"

/* Serialize the current simulation state and append one JSON line to f.
   Returns 0 on success, -1 on write failure. */
int emit_file_write(FILE *f, const Simulation *s);
