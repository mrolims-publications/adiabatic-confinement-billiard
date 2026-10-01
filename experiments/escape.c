/*
 * escape: escape collision number of nic initial conditions drawn
 * uniformly on the section x = 0, vx > 0, with y in [-h0, h0], vy in
 * [-1, 1] and unit speed.
 *
 * Output: data/escape_h0=.._L=.._xm=.._nic=.._nmax=...dat
 * Columns: y vy n t side, with side = 1 (left aperture), 2 (right
 * aperture) or 0 (inside after nmax collisions, then n = nmax).
 */

#include "args.h"
#include "billiard.h"
#include "rng.h"
#include "util.h"
#include <stdio.h>

int main(int argc, char **argv) {
    double h0 = 1.0, L, xm = 10.0;
    long nic, nmax, i, nesc = 0;
    billiard_boundary_t b;
    billiard_solver_t sol;
    billiard_escape_t e;
    rng_t rng;
    char name[512], path[1024];
    FILE *f;

    const arg_t spec[] = {
        {"L", ARG_DOUBLE, 1, &L, NULL, "profile length"},
        {"nic", ARG_LONG, 1, &nic, NULL, "number of initial conditions"},
        {"nmax", ARG_LONG, 1, &nmax, NULL, "maximum number of collisions"},
        {"h0", ARG_DOUBLE, 0, &h0, NULL, "half-width at the centre"},
        {"xm", ARG_DOUBLE, 0, &xm, NULL, "position of the apertures"},
    };

    args_parse(argc, argv, spec, ARGS_N(spec));

    if (!check_geometry(argv[0], h0, L, xm)) {
        return 1;
    }
    if (nic < 1 || nmax < 1) {
        fprintf(stderr, "%s: need nic >= 1 and nmax >= 1\n", argv[0]);
        return 1;
    }

    args_echo(spec, ARGS_N(spec));
    printf("seed = %llu\n", RNG_DEFAULT_SEED);

    rng_init(&rng, RNG_DEFAULT_SEED);
    b = billiard_boundary(h0, L, xm);
    sol = billiard_solver_default(&b);

    snprintf(name, sizeof name,
             "escape_h0=%.5f_L=%.5f_xm=%.5f_nic=%ld_nmax=%ld.dat", h0, L, xm,
             nic, nmax);
    f = data_open(name, path, sizeof path);
    if (f == NULL) {
        fprintf(stderr, "%s: cannot open %s\n", argv[0], path);
        return 1;
    }

    for (i = 0; i < nic; i++) {
        const double y = (2.0 * rng_uniform(&rng) - 1.0) * h0;
        const double vy = 2.0 * rng_uniform(&rng) - 1.0;
        int side = 0;

        e = billiard_escape_time(billiard_section_state(y, vy), &b, &sol, nmax);
        if (e.status == BILLIARD_ESCAPED_LEFT ||
            e.status == BILLIARD_ESCAPED_RIGHT) {
            side = (int)e.status;
            nesc++;
        }
        fprintf(f, "%.17g %.17g %ld %.17g %d\n", y, vy, e.n, e.t, side);
        progress(i + 1, nic);
    }

    fputc('\n', stderr);
    fclose(f);
    fprintf(stderr, "%s: escaping fraction %.6f\n", path, (double)nesc / nic);

    return 0;
}
