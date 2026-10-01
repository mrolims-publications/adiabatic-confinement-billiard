/*
 * section: Poincare section x = 0, vx > 0.  Initial conditions are drawn
 * uniformly in y in [-h0, h0] and vy in [0, 1], with unit speed.
 *
 * Output: data/section_h0=.._L=.._xm=.._nic=.._ncross=.._seed=...dat
 * Columns: i y vy side, with i the index of the initial condition and
 * side = 1 (left aperture), 2 (right aperture) or 0 (inside after ncross
 * crossings).
 */

#include "args.h"
#include "billiard.h"
#include "rng.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    double h0 = 1.0, L, xm = 10.0;
    long nic, ncross, seed = (long)RNG_DEFAULT_SEED, i, j, k;
    billiard_boundary_t b;
    billiard_solver_t sol;
    billiard_state_t s, prev;
    rng_t rng;
    char name[512], path[1024];
    double *ys, *vys;
    FILE *f;
    int side;

    const arg_t spec[] = {
        {"L", ARG_DOUBLE, 1, &L, NULL, "profile length"},
        {"nic", ARG_LONG, 1, &nic, NULL, "number of initial conditions"},
        {"ncross", ARG_LONG, 1, &ncross, NULL,
         "crossings recorded per initial condition"},
        {"seed", ARG_LONG, 0, &seed, NULL, "seed of the random generator"},
        {"h0", ARG_DOUBLE, 0, &h0, NULL, "half-width at the centre"},
        {"xm", ARG_DOUBLE, 0, &xm, NULL, "position of the apertures"},
    };

    args_parse(argc, argv, spec, ARGS_N(spec));

    if (!check_geometry(argv[0], h0, L, xm)) {
        return 1;
    }
    if (nic < 1 || ncross < 1 || seed < 0) {
        fprintf(stderr, "%s: need nic >= 1, ncross >= 1 and seed >= 0\n",
                argv[0]);
        return 1;
    }

    args_echo(spec, ARGS_N(spec));

    rng_init(&rng, (unsigned long long)seed);
    b = billiard_boundary(h0, L, xm);
    sol = billiard_solver_default(&b);

    snprintf(name, sizeof name,
             "section_h0=%.5f_L=%.5f_xm=%.5f_nic=%ld_ncross=%ld_"
             "seed=%ld.dat",
             h0, L, xm, nic, ncross, seed);
    f = data_open(name, path, sizeof path);
    if (f == NULL) {
        fprintf(stderr, "%s: cannot open %s\n", argv[0], path);
        return 1;
    }

    /* The crossings of one orbit are buffered until its fate is known. */
    ys = malloc((size_t)ncross * sizeof *ys);
    vys = malloc((size_t)ncross * sizeof *vys);
    if (ys == NULL || vys == NULL) {
        fprintf(stderr, "%s: out of memory\n", argv[0]);
        return 1;
    }

    for (i = 0; i < nic; i++) {
        const double vy = rng_uniform(&rng);
        const double y = (2.0 * rng_uniform(&rng) - 1.0) * h0;

        s = billiard_section_state(y, vy);
        ys[0] = s.y;
        vys[0] = s.vy;

        for (k = 1; k < ncross;) {
            prev = s;
            billiard_map(&s, &b, &sol);
            if (s.status == BILLIARD_OUTSIDE || s.status == BILLIARD_NO_HIT) {
                break;
            }
            /* Crossing of x = 0 with vx > 0, located by interpolation. */
            if (prev.x < 0.0 && s.x > 0.0) {
                const double u = -prev.x / (s.x - prev.x);

                ys[k] = prev.y + u * (s.y - prev.y);
                vys[k] = prev.vy;
                k++;
            }
            if (s.status != BILLIARD_COLLISION) {
                break;
            }
        }

        side = (s.status == BILLIARD_ESCAPED_LEFT)    ? 1
               : (s.status == BILLIARD_ESCAPED_RIGHT) ? 2
                                                      : 0;

        for (j = 0; j < k; j++) {
            fprintf(f, "%ld %.17g %.17g %d\n", i, ys[j], vys[j], side);
        }
        progress(i + 1, nic);
    }

    fputc('\n', stderr);
    fclose(f);
    free(ys);
    free(vys);
    fprintf(stderr, "%s\n", path);

    return 0;
}
