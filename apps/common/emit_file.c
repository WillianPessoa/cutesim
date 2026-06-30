#include "emit_file.h"

#include <stdlib.h>
#include <string.h>

#include "snapshot.h"

#define STACK_BUF_SIZE 16384

int emit_file_write(FILE *f, const Simulation *s) {
    char stack[STACK_BUF_SIZE];
    int n = snapshot_to_json(s, stack, sizeof(stack));

    const char *line = stack;
    char *heap       = NULL;

    if (n >= STACK_BUF_SIZE) {
        /* snapshot larger than stack buffer — allocate exact size */
        heap = malloc((size_t)n + 1);
        if (!heap) {
            return -1;
        }
        snapshot_to_json(s, heap, (size_t)n + 1);
        line = heap;
    }

    int rc = 0;
    if ((int)fwrite(line, 1, (size_t)n, f) != n) {
        rc = -1;
    }
    if (fputc('\n', f) == EOF) {
        rc = -1;
    }

    free(heap);
    return rc;
}
