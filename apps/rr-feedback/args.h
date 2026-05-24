#pragma once

#include "cutesim/config.h"

/* Parse argc/argv into a SimConfig.
 * Sets *error to 1 on failure, 0 on success. */
SimConfig parse_args(int argc, char **argv, int *error);
