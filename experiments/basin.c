/*
 * basin: escape basin on a grid_size x grid_size grid of initial
 * conditions on the section x = 0, vx > 0, with unit speed.  The grid
 * covers y in [yini, yend] and vy in [vyini, vyend]; grid points are cell
 * centres.
 *
 * Output: data/basin_h0=.._L=.._xm=.._grid_size=.._yini=..
 *         _yend=.._vyini=.._vyend=.._nmax=...dat
 * Columns: y vy n t side, with side = 1 (left aperture), 2 (right
 * aperture) or 0 (inside after nmax collisions).  Rows run vy fastest.
 */

#include "args.h"
#include "billiard.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif

int main(int argc, char **argv) {
    double h0 = 1.0, L, xm = 10.0;
    double yini = -1.0, yend = 1.0, vyini = -1.0, vyend = 1.0;
    long grid_size, nmax, nthreads = 1, i, j, done = 0;
    billiard_boundary_t b;
    billiard_solver_t sol;
    char name[512], path[1024];
    long *out_n;
    double *out_t;
    int *out_side;
    FILE *f;

#ifdef _OPENMP
    nthreads = omp_get_max_threads();
#endif

    const arg_t spec[] = {
        {"L", ARG_DOUBLE, 1, &L, NULL, "profile length"},
        {"grid_size", ARG_LONG, 1, &grid_size, NULL,
         "number of grid points along each axis"},
        {"nmax", ARG_LONG, 1, &nmax, NULL, "maximum number of collisions"},
        {"h0", ARG_DOUBLE, 0, &h0, NULL, "half-width at the centre"},
        {"xm", ARG_DOUBLE, 0, &xm, NULL, "position of the apertures"},
        {"yini", ARG_DOUBLE, 0, &yini, NULL, "lower end of the grid in y"},
        {"yend", ARG_DOUBLE, 0, &yend, NULL, "upper end of the grid in y"},
        {"vyini", ARG_DOUBLE, 0, &vyini, NULL, "lower end of the grid in vy"},
        {"vyend", ARG_DOUBLE, 0, &vyend, NULL, "upper end of the grid in vy"},
        {"nthreads", ARG_LONG, 0, &nthreads, NULL, "number of OpenMP threads"},
    };

    args_parse(argc, argv, spec, ARGS_N(spec));

    if (!check_geometry(argv[0], h0, L, xm)) {
        return 1;
    }
    if (grid_size < 1 || nmax < 1 || nthreads < 1) {
        fprintf(stderr, "%s: need grid_size, nmax and nthreads >= 1\n",
                argv[0]);
        return 1;
    }
    if (yini < -h0 || yend > h0 || yend <= yini) {
        fprintf(stderr, "%s: need -h0 <= yini < yend <= h0\n", argv[0]);
        return 1;
    }
    if (vyini < -1.0 || vyend > 1.0 || vyend <= vyini) {
        fprintf(stderr, "%s: need -1 <= vyini < vyend <= 1\n", argv[0]);
        return 1;
    }

    args_echo(spec, ARGS_N(spec));
    fflush(stdout);

    b = billiard_boundary(h0, L, xm);
    sol = billiard_solver_default(&b);

    out_n = malloc((size_t)grid_size * (size_t)grid_size * sizeof *out_n);
    out_t = malloc((size_t)grid_size * (size_t)grid_size * sizeof *out_t);
    out_side = malloc((size_t)grid_size * (size_t)grid_size * sizeof *out_side);
    if (out_n == NULL || out_t == NULL || out_side == NULL) {
        fprintf(stderr, "%s: out of memory\n", argv[0]);
        return 1;
    }

#ifdef _OPENMP
    omp_set_num_threads((int)nthreads);
#endif
#pragma omp parallel for schedule(dynamic) private(j)
    for (i = 0; i < grid_size; i++) {
        long seen;

        for (j = 0; j < grid_size; j++) {
            const double y =
                yini + (yend - yini) * ((double)i + 0.5) / grid_size;
            const double vy =
                vyini + (vyend - vyini) * ((double)j + 0.5) / grid_size;
            const long k = i * grid_size + j;
            const billiard_escape_t e = billiard_escape_time(
                billiard_section_state(y, vy), &b, &sol, nmax);

            out_n[k] = e.n;
            out_t[k] = e.t;
            out_side[k] = (int)e.status;
        }

#pragma omp atomic capture
        seen = ++done;

#pragma omp critical(progress)
        progress(seen, grid_size);
    }
    fputc('\n', stderr);

    snprintf(name, sizeof name,
             "basin_h0=%.5f_L=%.5f_xm=%.5f_grid_size=%ld_"
             "yini=%.5f_yend=%.5f_vyini=%.5f_vyend=%.5f_nmax=%ld.dat",
             h0, L, xm, grid_size, yini, yend, vyini, vyend, nmax);
    f = data_open(name, path, sizeof path);
    if (f == NULL) {
        fprintf(stderr, "%s: cannot open %s\n", argv[0], path);
        return 1;
    }

    for (i = 0; i < grid_size; i++) {
        const double y = yini + (yend - yini) * ((double)i + 0.5) / grid_size;

        for (j = 0; j < grid_size; j++) {
            const double vy =
                vyini + (vyend - vyini) * ((double)j + 0.5) / grid_size;
            const long k = i * grid_size + j;

            fprintf(f, "%.17g %.17g %ld %.17g %d\n", y, vy, out_n[k], out_t[k],
                    out_side[k]);
        }
    }

    fclose(f);
    free(out_n);
    free(out_t);
    free(out_side);
    fprintf(stderr, "%s\n", path);

    return 0;
}
