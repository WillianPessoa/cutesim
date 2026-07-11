#include "cutesim/scenario.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Small in-place text helpers — operate on [start, end) ranges of the input
 * so no line buffer or fixed cap is needed.
 * ---------------------------------------------------------------------- */

static int is_ws(char c) { return c == ' ' || c == '\t' || c == '\r'; }

/* Trim leading/trailing whitespace, narrowing [*start, *end). */
static void trim(const char **start, const char **end) {
    while (*start < *end && is_ws(**start)) {
        (*start)++;
    }
    while (*end > *start && is_ws(*(*end - 1))) {
        (*end)--;
    }
}

/* True if the range [s, e) equals the NUL-terminated literal. */
static int range_eq(const char *s, const char *e, const char *lit) {
    size_t n = (size_t)(e - s);
    return strlen(lit) == n && strncmp(s, lit, n) == 0;
}

/* Parse a base-10 int from [s, e). Returns 0 and stores *out on success, -1 on junk. */
static int range_to_int(const char *s, const char *e, int *out) {
    char buf[32];
    size_t n = (size_t)(e - s);
    if (n == 0 || n >= sizeof(buf)) {
        return -1;
    }
    memcpy(buf, s, n);
    buf[n]    = '\0';
    char *end = NULL;
    long v    = strtol(buf, &end, 10);
    if (end == buf || *end != '\0') {
        return -1;
    }
    *out = (int)v;
    return 0;
}

/* Parse a duration from [s, e): "N" (fixed) or "N-M" (range). Returns 0 / -1. */
static int range_to_duration(const char *s, const char *e, Duration *out) {
    const char *dash = s;
    while (dash < e && *dash != '-') {
        dash++;
    }
    if (dash == e) {
        int v;
        if (range_to_int(s, e, &v) != 0) {
            return -1;
        }
        out->min = out->max = v;
        return 0;
    }
    int lo, hi;
    if (range_to_int(s, dash, &lo) != 0 || range_to_int(dash + 1, e, &hi) != 0) {
        return -1;
    }
    out->min = lo;
    out->max = hi;
    return 0;
}

/* Parse an I/O mode from [s, e): "queue" or "concurrent". Returns 0 / -1. */
static int range_to_io_mode(const char *s, const char *e, IoMode *out) {
    if (range_eq(s, e, "queue")) {
        *out = IO_MODE_QUEUE;
        return 0;
    }
    if (range_eq(s, e, "concurrent")) {
        *out = IO_MODE_CONCURRENT;
        return 0;
    }
    return -1;
}

/* Parse a device name from [s, e): "disk" / "tape" / "printer". Returns 0 / -1. */
static int range_to_device(const char *s, const char *e, DeviceType *out) {
    if (range_eq(s, e, "disk")) {
        *out = DEVICE_DISK;
        return 0;
    }
    if (range_eq(s, e, "tape")) {
        *out = DEVICE_TAPE;
        return 0;
    }
    if (range_eq(s, e, "printer")) {
        *out = DEVICE_PRINTER;
        return 0;
    }
    return -1;
}

/* -------------------------------------------------------------------------
 * Key dispatch — global "key = value" lines
 * ---------------------------------------------------------------------- */

/* Apply one global key/value to the config. Returns 0 ok, -1 unknown key / bad value. */
static int
apply_global(Scenario *out, const char *ks, const char *ke, const char *vs, const char *ve) {
    SimConfig *c = &out->config;

    if (range_eq(ks, ke, "quantum-hi")) {
        return range_to_int(vs, ve, &c->quantum_hi);
    }
    if (range_eq(ks, ke, "quantum-lo")) {
        return range_to_int(vs, ve, &c->quantum_lo);
    }
    if (range_eq(ks, ke, "process-count")) {
        return range_to_int(vs, ve, &c->process_count);
    }
    if (range_eq(ks, ke, "p-io")) {
        return range_to_int(vs, ve, &c->p_io);
    }
    if (range_eq(ks, ke, "p-disk")) {
        return range_to_int(vs, ve, &c->p_disk);
    }
    if (range_eq(ks, ke, "p-tape")) {
        return range_to_int(vs, ve, &c->p_tape);
    }
    if (range_eq(ks, ke, "p-printer")) {
        return range_to_int(vs, ve, &c->p_printer);
    }
    if (range_eq(ks, ke, "seed")) {
        int v;
        if (range_to_int(vs, ve, &v) != 0) {
            return -1;
        }
        c->seed = (unsigned)v;
        return 0;
    }
    if (range_eq(ks, ke, "disk-duration")) {
        return range_to_duration(vs, ve, &c->disk_duration);
    }
    if (range_eq(ks, ke, "tape-duration")) {
        return range_to_duration(vs, ve, &c->tape_duration);
    }
    if (range_eq(ks, ke, "printer-duration")) {
        return range_to_duration(vs, ve, &c->printer_duration);
    }
    if (range_eq(ks, ke, "service-duration")) {
        return range_to_duration(vs, ve, &c->service_duration);
    }
    if (range_eq(ks, ke, "disk-mode")) {
        return range_to_io_mode(vs, ve, &c->io_mode_disk);
    }
    if (range_eq(ks, ke, "tape-mode")) {
        return range_to_io_mode(vs, ve, &c->io_mode_tape);
    }
    if (range_eq(ks, ke, "printer-mode")) {
        return range_to_io_mode(vs, ve, &c->io_mode_printer);
    }
    return -1; /* unknown key */
}

/* Append a zeroed ScriptedProcess to the dynamic array; returns it or NULL on OOM. */
static ScriptedProcess *push_process(Scenario *out) {
    if (out->process_count >= out->process_cap) {
        int cap              = out->process_cap ? out->process_cap * 2 : 4;
        ScriptedProcess *tmp = realloc(out->processes, (size_t)cap * sizeof(*tmp));
        if (!tmp) {
            return NULL;
        }
        out->processes   = tmp;
        out->process_cap = cap;
    }
    ScriptedProcess *p = &out->processes[out->process_count++];
    memset(p, 0, sizeof(*p));
    return p;
}

/* Append a zeroed ScriptedIO to the process's dynamic timeline; NULL on OOM. */
static ScriptedIO *push_io(ScriptedProcess *p) {
    if (p->io_count >= p->io_cap) {
        int cap         = p->io_cap ? p->io_cap * 2 : 4;
        ScriptedIO *tmp = realloc(p->io, (size_t)cap * sizeof(*tmp));
        if (!tmp) {
            return NULL;
        }
        p->io     = tmp;
        p->io_cap = cap;
    }
    ScriptedIO *ev = &p->io[p->io_count++];
    memset(ev, 0, sizeof(*ev));
    return ev;
}

/* Parse one event "tick:device" or "tick:device:duration" from [s, e). */
static int parse_io_event(ScriptedProcess *p, const char *s, const char *e) {
    trim(&s, &e);

    const char *c1 = s; /* first ':' — after the tick */
    while (c1 < e && *c1 != ':') {
        c1++;
    }
    if (c1 == e) {
        return -1; /* need at least tick:device */
    }
    const char *c2 = c1 + 1; /* second ':' — before the optional duration */
    while (c2 < e && *c2 != ':') {
        c2++;
    }

    const char *ts = s, *te = c1;
    const char *ds = c1 + 1, *de = c2;
    trim(&ts, &te);
    trim(&ds, &de);

    int tick;
    DeviceType dev;
    if (range_to_int(ts, te, &tick) != 0 || range_to_device(ds, de, &dev) != 0) {
        return -1;
    }

    ScriptedIO *ev = push_io(p);
    if (!ev) {
        return -1;
    }
    ev->service_tick = tick;
    ev->device       = dev;

    if (c2 < e) { /* explicit duration override */
        const char *us = c2 + 1, *ue = e;
        trim(&us, &ue);
        if (range_to_duration(us, ue, &ev->duration) != 0) {
            return -1;
        }
        ev->has_duration = 1;
    }
    return 0;
}

/* Parse a comma-separated I/O timeline from [s, e). */
static int parse_io_list(ScriptedProcess *p, const char *s, const char *e) {
    const char *item = s;
    while (item < e) {
        const char *comma = item;
        while (comma < e && *comma != ',') {
            comma++;
        }
        if (parse_io_event(p, item, comma) != 0) {
            return -1;
        }
        item = (comma < e) ? comma + 1 : comma;
    }
    return 0;
}

/* Apply one key/value inside a [process] block. Returns 0 ok, -1 unknown / bad value. */
static int
apply_process(ScriptedProcess *p, const char *ks, const char *ke, const char *vs, const char *ve) {
    if (range_eq(ks, ke, "arrival")) {
        return range_to_int(vs, ve, &p->arrival_tick);
    }
    if (range_eq(ks, ke, "burst")) {
        return range_to_int(vs, ve, &p->burst);
    }
    if (range_eq(ks, ke, "io")) {
        return parse_io_list(p, vs, ve);
    }
    return -1; /* unknown key */
}

/* Validate scripted I/O timelines: service ticks must be strictly increasing (so the
   engine reaches them in order) and, for a finite burst, each must be < burst (so the
   process does not complete before reaching the event). Returns 0 ok, -1 on violation. */
static int validate(const Scenario *out) {
    for (int i = 0; i < out->process_count; i++) {
        const ScriptedProcess *p = &out->processes[i];
        int prev                 = -1;
        for (int j = 0; j < p->io_count; j++) {
            int t = p->io[j].service_tick;
            if (t <= prev) {
                return -1; /* not strictly increasing */
            }
            if (p->burst > 0 && t >= p->burst) {
                return -1; /* unreachable: process completes first */
            }
            prev = t;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

int scenario_parse_string(const char *text, Scenario *out) {
    const char *p        = text;
    ScriptedProcess *cur = NULL; /* current [process] block, or NULL at global scope */

    while (*p) {
        /* Line spans [line, eol) */
        const char *line = p;
        const char *eol  = line;
        while (*eol && *eol != '\n') {
            eol++;
        }
        p = (*eol == '\n') ? eol + 1 : eol;

        /* Strip trailing comment (from '#' to end of line) */
        const char *hash = line;
        while (hash < eol && *hash != '#') {
            hash++;
        }
        const char *start = line;
        const char *end   = hash;
        trim(&start, &end);

        if (start == end) {
            continue; /* blank or comment-only line */
        }

        /* Block header: [name] switches the active scope */
        if (*start == '[' && *(end - 1) == ']') {
            const char *ns = start + 1, *ne = end - 1;
            trim(&ns, &ne);
            if (range_eq(ns, ne, "process")) {
                cur = push_process(out);
                if (!cur) {
                    return -1;
                }
            } else {
                return -1; /* unknown block */
            }
            continue;
        }

        /* Split on the first '=' */
        const char *eq = start;
        while (eq < end && *eq != '=') {
            eq++;
        }
        if (eq == end) {
            return -1; /* no '=' and not a recognised construct */
        }

        const char *ks = start, *ke = eq;
        const char *vs = eq + 1, *ve = end;
        trim(&ks, &ke);
        trim(&vs, &ve);

        int rc = cur ? apply_process(cur, ks, ke, vs, ve) : apply_global(out, ks, ke, vs, ve);
        if (rc != 0) {
            return -1;
        }
    }

    return validate(out);
}

int scenario_parse_file(const char *path, Scenario *out) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return -1;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }

    char *buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return -1;
    }
    size_t n = fread(buf, 1, (size_t)size, f);
    fclose(f);
    /* n <= size by fread's contract and buf holds size + 1 bytes; the analyzer
       taints every file-derived value and cannot see that bound. */
    buf[n] = '\0'; /* NOLINT(clang-analyzer-security.ArrayBound) */

    int rc = scenario_parse_string(buf, out);
    free(buf);
    return rc;
}

void scenario_free(Scenario *out) {
    if (!out) {
        return;
    }
    for (int i = 0; i < out->process_count; i++) {
        free(out->processes[i].io);
    }
    free(out->processes);
    out->processes     = NULL;
    out->process_count = 0;
    out->process_cap   = 0;
}
