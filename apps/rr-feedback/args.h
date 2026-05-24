#pragma once

#include "cutesim/config.h"

/* Parse argc/argv into a SimConfig.
 * Sets *error to 0 on success, 1 on invalid input, 2 when help was requested. */
SimConfig parse_args(int argc, char **argv, int *error);
