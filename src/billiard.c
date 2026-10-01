#include "billiard.h"
#include <math.h>

/*
 * Along the free flight x(tau) = x + vx tau, y(tau) = y + vy tau, the wall
 * is the zero set of g(tau) = |y(tau)| - h(x(tau)), negative inside the
 * billiard and positive outside.
 */

billiard_boundary_t billiard_boundary(double h0, double L, double xm) {
    billiard_boundary_t b;
    const double um = xm / L;

    b.h0 = h0;
    b.L = L;
    b.xm = xm;
    b.h_max = h0;
    b.h_min = h0 / (1.0 + um * um);

    return b;
}

double billiard_h(double x, const billiard_boundary_t *b) {
    const double u = x / b->L;

    return b->h0 / (1.0 + u * u);
}

double billiard_dh_dx(double x, const billiard_boundary_t *b) {
    const double u = x / b->L;
    const double d = 1.0 + u * u;

    return -2.0 * b->h0 * u / (b->L * d * d);
}

billiard_solver_t billiard_solver_default(const billiard_boundary_t *b) {
    billiard_solver_t sol;

    sol.ds = 0.125 * b->h_min;
    sol.s_min = 1.0e-10;
    sol.tol = 1.0e-13;
    sol.max_iter = 100;

    return sol;
}

billiard_state_t billiard_section_state(double y, double vy) {
    billiard_state_t s;

    s.x = 0.0;
    s.y = y;
    s.vx = sqrt(1.0 - vy * vy);
    s.vy = vy;
    s.t = 0.0;
    s.status = BILLIARD_COLLISION;

    return s;
}

static double gap(double tau, const billiard_state_t *s,
                  const billiard_boundary_t *b) {
    const double x = s->x + s->vx * tau;
    const double y = s->y + s->vy * tau;

    return fabs(y) - billiard_h(x, b);
}

static double gap_prime(double tau, const billiard_state_t *s,
                        const billiard_boundary_t *b) {
    const double x = s->x + s->vx * tau;
    const double y = s->y + s->vy * tau;
    const double sg = (y >= 0.0) ? 1.0 : -1.0;

    return sg * s->vy - billiard_dh_dx(x, b) * s->vx;
}

/* Refine a bracket with g(lo) <= 0 < g(hi).  Returns lo, which is always
 * inside the billiard. */
static double refine(double lo, double hi, const billiard_state_t *s,
                     const billiard_boundary_t *b, const billiard_solver_t *sol,
                     double tol_tau) {
    double tau = 0.5 * (lo + hi);
    int i;

    for (i = 0; i < sol->max_iter && hi - lo > tol_tau; i++) {
        const double g = gap(tau, s, b);
        const double gp = gap_prime(tau, s, b);
        double next;

        if (g > 0.0) {
            hi = tau;
        } else {
            lo = tau;
        }

        next = (gp != 0.0) ? tau - g / gp : 0.5 * (lo + hi);
        if (!(next > lo && next < hi)) {
            next = 0.5 * (lo + hi);
        }
        tau = next;
    }

    return lo;
}

/* Specular reflection v' = v - 2 (v.n) n / |n|^2, with n = (-h'(x), sgn(y)). */
static void reflect(billiard_state_t *s, const billiard_boundary_t *b) {
    const double sg = (s->y >= 0.0) ? 1.0 : -1.0;
    const double hx = billiard_dh_dx(s->x, b);
    const double nx = -hx;
    const double ny = sg;
    const double nn = nx * nx + ny * ny;
    const double rn = -hx * s->vx + sg * s->vy;
    const double c = 2.0 * rn / nn;

    s->vx -= c * nx;
    s->vy -= c * ny;
}

void billiard_map(billiard_state_t *s, const billiard_boundary_t *b,
                  const billiard_solver_t *sol) {
    double v, dtau, tau_min, tau_stop, tau_prev, tau, tol_tau;
    int armed, exiting;

    if (s->status == BILLIARD_ESCAPED_LEFT ||
        s->status == BILLIARD_ESCAPED_RIGHT) {
        return;
    }

    if (fabs(s->x) > b->xm || fabs(s->y) > billiard_h(s->x, b)) {
        s->status = BILLIARD_OUTSIDE;
        return;
    }

    v = hypot(s->vx, s->vy);
    if (v == 0.0) {
        s->status = BILLIARD_NO_HIT;
        return;
    }

    dtau = sol->ds / v;
    tau_min = sol->s_min / v;
    tol_tau = sol->tol / v;

    /* The scan stops at the aperture the particle is heading for.  A
     * vertical flight must hit a wall before (h_max + |y|)/|vy|. */
    if (s->vx > 0.0) {
        tau_stop = (b->xm - s->x) / s->vx;
        exiting = 1;
    } else if (s->vx < 0.0) {
        tau_stop = (-b->xm - s->x) / s->vx;
        exiting = -1;
    } else {
        tau_stop = (b->h_max + fabs(s->y)) / fabs(s->vy) + dtau;
        exiting = 0;
    }

    /* The state usually starts on a wall, where g = 0, so a crossing is
     * only accepted after the trajectory has entered the interior. */
    tau_prev = (tau_min < tau_stop) ? tau_min : tau_stop;
    armed = (gap(tau_prev, s, b) < 0.0);

    for (tau = tau_prev; tau < tau_stop;) {
        double g;

        tau = tau_prev + dtau;
        if (tau > tau_stop) {
            tau = tau_stop;
        }
        g = gap(tau, s, b);

        if (armed && g > 0.0) {
            const double tau_hit = refine(tau_prev, tau, s, b, sol, tol_tau);

            s->x += s->vx * tau_hit;
            s->y += s->vy * tau_hit;
            s->t += tau_hit;
            reflect(s, b);
            s->status = BILLIARD_COLLISION;
            return;
        }
        if (!armed && g < 0.0) {
            armed = 1;
        }

        tau_prev = tau;
    }

    /* Never inside: the state was on a wall with an outward velocity. */
    if (!armed) {
        s->status = BILLIARD_OUTSIDE;
        return;
    }

    if (exiting == 0) {
        s->status = BILLIARD_NO_HIT;
        return;
    }

    s->x = (exiting > 0) ? b->xm : -b->xm;
    s->y += s->vy * tau_stop;
    s->t += tau_stop;
    s->status = (exiting > 0) ? BILLIARD_ESCAPED_RIGHT : BILLIARD_ESCAPED_LEFT;
}

billiard_escape_t billiard_escape_time(billiard_state_t s,
                                       const billiard_boundary_t *b,
                                       const billiard_solver_t *sol,
                                       long nmax) {
    billiard_escape_t e;
    const double t0 = s.t;

    for (e.n = 0; e.n < nmax;) {
        billiard_map(&s, b, sol);
        e.n++;
        if (s.status != BILLIARD_COLLISION) {
            break;
        }
    }

    e.t = s.t - t0;
    e.status = s.status;

    return e;
}
