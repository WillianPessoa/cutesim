#pragma once

#include <stddef.h>

#include "cutesim/simulation.h"

/* Serialize the simulation state to a single JSON object (no trailing newline).
   Returns the number of bytes that would be written excluding the null terminator.
   The return value may exceed bufsz if the buffer is too small; the string is
   always null-terminated when bufsz > 0.
   Callers can call with buf=NULL and bufsz=0 to measure the required size. */
int snapshot_to_json(const Simulation *s, char *buf, size_t bufsz);
