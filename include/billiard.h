#ifndef BILLIARD_H
#define BILLIARD_H

/*
 * Open billiard |y| <= h(x), |x| <= xm, with the Lorentzian half-width
 *
 *     h(x) = h0 / (1 + (x/L)^2).
 *
 * The segments x = +-xm are apertures: a particle that reaches one of
 * them escapes.
 */

typedef enum {
    BILLIARD_COLLISION = 0, /* on a wall, velocity outgoing   */
    BILLIARD_ESCAPED_LEFT,  /* on the aperture x = -xm         */
    BILLIARD_ESCAPED_RIGHT, /* on the aperture x = +xm         */
    BILLIARD_OUTSIDE,       /* input state was not admissible  */
    BILLIARD_NO_HIT         /* no wall crossing was found      */
} billiard_status_t;

typedef struct {
    double x, y;   /* position                    */
    double vx, vy; /* velocity                    */
    double t;      /* elapsed time                */
    billiard_status_t status;
} billiard_state_t;

typedef struct {
    double h0;    /* half-width at the centre              */
    double L;     /* profile length, h(L) = h0/2           */
    double xm;    /* apertures at x = +-xm                 */
    double h_min; /* h(xm), the narrowest half-width       */
    double h_max; /* h(0), the widest half-width           */
} billiard_boundary_t;

billiard_boundary_t billiard_boundary(double h0, double L, double xm);

double billiard_h(double x, const billiard_boundary_t *b);
double billiard_dh_dx(double x, const billiard_boundary_t *b);

/*
 * Root finder for the next collision.  The trajectory is scanned with
 * step ds (a length) until it leaves the billiard, and the crossing is
 * refined by Newton steps safeguarded by bisection.  ds must be smaller
 * than the narrowest chord of the billiard.
 */
typedef struct {
    double ds;    /* scan step                                      */
    double s_min; /* distance at which the scan starts              */
    double tol;   /* bracket width at which the refinement stops    */
    int max_iter; /* maximum number of refinement iterations        */
} billiard_solver_t;

/* ds = h_min/8, s_min = 1e-10, tol = 1e-13, max_iter = 100. */
billiard_solver_t billiard_solver_default(const billiard_boundary_t *b);

/*
 * Advance s by one collision, in place.  On BILLIARD_COLLISION the state is
 * on the wall with the reflected velocity; on BILLIARD_ESCAPED_* it is on
 * the aperture with the velocity unchanged.
 */
void billiard_map(billiard_state_t *s, const billiard_boundary_t *b,
                  const billiard_solver_t *sol);

typedef struct {
    long n;                   /* number of map iterations                  */
    double t;                 /* elapsed time                              */
    billiard_status_t status; /* ESCAPED_LEFT/RIGHT, or COLLISION if the
                               orbit is still inside after nmax steps   */
} billiard_escape_t;

/* Iterate s until it escapes or nmax collisions are done. */
billiard_escape_t billiard_escape_time(billiard_state_t s,
                                       const billiard_boundary_t *b,
                                       const billiard_solver_t *sol, long nmax);

/* Initial condition on the section x = 0, vx > 0, with unit speed. */
billiard_state_t billiard_section_state(double y, double vy);

#endif /* BILLIARD_H */
