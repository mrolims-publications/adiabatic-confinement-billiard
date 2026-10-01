/*
 * uncertainty: uncertainty fraction f(eps) of the boundary between the
 * left- and right-escape basins on the section x = 0, vx > 0.
 *
 * For each eps, nic usable pairs are drawn: a reference point uniform in
 * y in [-h0, h0], vy in [-1, 1], and a second point at distance eps in a
 * uniformly random direction.  Reference points within eps of the
 * boundary of the domain are rejected, and so are pairs in which either
 * orbit is still inside after nmax collisions.  A pair is uncertain when
 * the two orbits leave through different apertures.
 *
 * eps is swept logarithmically from eps_min to eps_max.  Each pair has
 * its own random stream, seeded from (run, pair index), so the result
 * does not depend on the number of threads.  Independent realizations
 * are obtained with different values of run.
 *
 * Output: data/uncertainty_h0=.._L=.._xm=.._nic=.._eps_min=..
 *         _eps_max=.._n_eps=.._run=.._nmax=...dat
 * Columns: eps f
 */

#include "args.h"
#include "billiard.h"
#include "constants.h"
#include "rng.h"
#include "util.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif

/* Maximum number of draws per usable pair. */
#define DRAW_MAX 100000L

/* 1 left aperture, 2 right aperture, 0 still inside after nmax. */
static int aperture(double y, double vy, const billiard_boundary_t *b,
                    const billiard_solver_t *sol, long nmax) {
    const billiard_escape_t e =
        billiard_escape_time(billiard_section_state(y, vy), b, sol, nmax);

    if (e.status == BILLIARD_ESCAPED_LEFT) {
        return 1;
    }
    if (e.status == BILLIARD_ESCAPED_RIGHT) {
        return 2;
    }

    return 0;
}

int main(int argc, char **argv) {
    double h0 = 1.0, L, xm = 10.0;
    double eps_min = 1e-8, eps_max = 1e-2;
    long nic, nmax, n_eps = 20, run = 0, nthreads = 1, k, idx, total;
    long done = 0;
    int exhausted = 0;
    billiard_boundary_t b;
    billiard_solver_t sol;
    char name[512], path[1024];
    double *eps;
    long *unc;
    FILE *f;

#ifdef _OPENMP
    nthreads = omp_get_max_threads();
#endif

    const arg_t spec[] = {
        {"L", ARG_DOUBLE, 1, &L, NULL, "profile length"},
        {"nic", ARG_LONG, 1, &nic, NULL, "usable pairs for each eps"},
        {"nmax", ARG_LONG, 1, &nmax, NULL, "maximum number of collisions"},
        {"h0", ARG_DOUBLE, 0, &h0, NULL, "half-width at the centre"},
        {"xm", ARG_DOUBLE, 0, &xm, NULL, "position of the apertures"},
        {"eps_min", ARG_DOUBLE, 0, &eps_min, "%.3e", "smallest eps"},
        {"eps_max", ARG_DOUBLE, 0, &eps_max, "%.3e", "largest eps"},
        {"n_eps", ARG_LONG, 0, &n_eps, NULL, "number of values of eps"},
        {"run", ARG_LONG, 0, &run, NULL, "index of the realization"},
        {"nthreads", ARG_LONG, 0, &nthreads, NULL, "number of OpenMP threads"},
    };

    args_parse(argc, argv, spec, ARGS_N(spec));

    if (!check_geometry(argv[0], h0, L, xm)) {
        return 1;
    }
    if (nic < 1 || nmax < 1 || n_eps < 1 || run < 0 || nthreads < 1) {
        fprintf(stderr,
                "%s: need nic, nmax, n_eps, nthreads >= 1 and "
                "run >= 0\n",
                argv[0]);
        return 1;
    }
    if (eps_min <= 0.0 || eps_max < eps_min || eps_max >= h0 ||
        eps_max >= 1.0) {
        fprintf(stderr, "%s: need 0 < eps_min <= eps_max < min(h0, 1)\n",
                argv[0]);
        return 1;
    }

    args_echo(spec, ARGS_N(spec));
    printf("seed = %llu\n", RNG_DEFAULT_SEED);

    b = billiard_boundary(h0, L, xm);
    sol = billiard_solver_default(&b);

    eps = malloc((size_t)n_eps * sizeof *eps);
    unc = calloc((size_t)n_eps, sizeof *unc);
    if (eps == NULL || unc == NULL) {
        fprintf(stderr, "%s: out of memory\n", argv[0]);
        return 1;
    }
    for (k = 0; k < n_eps; k++) {
        eps[k] = (n_eps == 1) ? eps_min
                              : eps_min * pow(eps_max / eps_min,
                                              (double)k / (n_eps - 1));
    }

    total = n_eps * nic;
#ifdef _OPENMP
    omp_set_num_threads((int)nthreads);
#endif
#pragma omp parallel for schedule(dynamic)
    for (idx = 0; idx < total; idx++) {
        const long j = idx / nic;
        const double e = eps[j];
        rng_t rng;
        long draw, current;
        int kept = 0;

        rng_init_stream(&rng, RNG_DEFAULT_SEED + (unsigned long long)run,
                        (unsigned long long)idx);

        for (draw = 0; draw < DRAW_MAX && !kept; draw++) {
            const double y = (2.0 * rng_uniform(&rng) - 1.0) * h0;
            const double vy = 2.0 * rng_uniform(&rng) - 1.0;
            double phi;
            int s0, s1;

            if (fabs(y) > h0 - e || fabs(vy) > 1.0 - e) {
                continue;
            }

            phi = TWO_PI * rng_uniform(&rng);

            s0 = aperture(y, vy, &b, &sol, nmax);
            if (s0 == 0) {
                continue;
            }
            s1 = aperture(y + e * cos(phi), vy + e * sin(phi), &b, &sol, nmax);
            if (s1 == 0) {
                continue;
            }

            if (s0 != s1) {
#pragma omp atomic
                unc[j]++;
            }
            kept = 1;
        }

        if (!kept) {
#pragma omp atomic write
            exhausted = 1;
        }

#pragma omp atomic capture
        current = ++done;

        if (current % 1000 == 0 || current == total) {
#pragma omp critical(progress)
            progress(current, total);
        }
    }
    fputc('\n', stderr);

    if (exhausted) {
        fprintf(stderr, "%s: no usable pair found after %ld draws\n", argv[0],
                DRAW_MAX);
        return 1;
    }

    snprintf(name, sizeof name,
             "uncertainty_h0=%.5f_L=%.5f_xm=%.5f_nic=%ld_"
             "eps_min=%.3e_eps_max=%.3e_n_eps=%ld_run=%ld_nmax=%ld.dat",
             h0, L, xm, nic, eps_min, eps_max, n_eps, run, nmax);
    f = data_open(name, path, sizeof path);
    if (f == NULL) {
        fprintf(stderr, "%s: cannot open %s\n", argv[0], path);
        return 1;
    }

    for (k = 0; k < n_eps; k++) {
        fprintf(f, "%.17g %.17g\n", eps[k], (double)unc[k] / nic);
    }

    fclose(f);
    free(eps);
    free(unc);
    fprintf(stderr, "%s\n", path);

    return 0;
}
