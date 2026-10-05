/*
 * action: transverse action along one orbit launched from the section
 * x = 0, vx > 0 with unit speed, until it escapes or nmax is reached.
 *
 * At each collision the action is evaluated as
 *
 *     J_n = (2/pi) |vy_n| h(x_n),
 *
 * with vy_n the velocity after the reflection, and compared with its
 * value at the initial condition, J_0 = (2/pi) |vy0| h0.  The final
 * crossing of an aperture is not a collision and is not written.
 *
 * Output: data/action_h0=.._L=.._xm=.._y0=.._vy0=.._nmax=...dat
 * Columns: n t (J_n - J_0)/J_0, with n = 0 the initial condition.
 */

#include "args.h"
#include "billiard.h"
#include "constants.h"
#include "util.h"
#include <math.h>
#include <stdio.h>

static double action(const billiard_state_t *s, const billiard_boundary_t *b) {
    return 2.0 / PI * fabs(s->vy) * billiard_h(s->x, b);
}

int main(int argc, char **argv) {
    double h0 = 1.0, L, xm = 10.0, y0, vy0;
    double J0;
    long nmax, n;
    billiard_boundary_t b;
    billiard_solver_t sol;
    billiard_state_t s;
    char name[512], path[1024];
    FILE *f;

    const arg_t spec[] = {
        {"L", ARG_DOUBLE, 1, &L, NULL, "profile length"},
        {"y0", ARG_DOUBLE, 1, &y0, NULL, "initial y on the section"},
        {"vy0", ARG_DOUBLE, 1, &vy0, NULL, "initial vy on the section"},
        {"nmax", ARG_LONG, 1, &nmax, NULL, "maximum number of collisions"},
        {"h0", ARG_DOUBLE, 0, &h0, NULL, "half-width at the centre"},
        {"xm", ARG_DOUBLE, 0, &xm, NULL, "position of the apertures"},
    };

    args_parse(argc, argv, spec, ARGS_N(spec));

    if (!check_geometry(argv[0], h0, L, xm)) {
        return 1;
    }
    if (fabs(y0) > h0 || fabs(vy0) > 1.0 || vy0 == 0.0 || nmax < 1) {
        fprintf(stderr,
                "%s: need |y0| <= h0, 0 < |vy0| <= 1 and nmax >= 1\n",
                argv[0]);
        return 1;
    }

    args_echo(spec, ARGS_N(spec));

    b = billiard_boundary(h0, L, xm);
    sol = billiard_solver_default(&b);

    snprintf(name, sizeof name,
             "action_h0=%.5f_L=%.5f_xm=%.5f_y0=%.5f_"
             "vy0=%.5f_nmax=%ld.dat",
             h0, L, xm, y0, vy0, nmax);
    f = data_open(name, path, sizeof path);
    if (f == NULL) {
        fprintf(stderr, "%s: cannot open %s\n", argv[0], path);
        return 1;
    }

    s = billiard_section_state(y0, vy0);
    J0 = action(&s, &b);
    fprintf(f, "%d %.17g %.17g\n", 0, s.t, 0.0);

    for (n = 1; n <= nmax; n++) {
        billiard_map(&s, &b, &sol);
        if (s.status != BILLIARD_COLLISION) {
            break;
        }
        fprintf(f, "%ld %.17g %.17g\n", n, s.t, (action(&s, &b) - J0) / J0);
    }

    fclose(f);
    fprintf(stderr, "%s: %s\n", path,
            s.status == BILLIARD_COLLISION ? "still inside" : "escaped");

    return 0;
}
