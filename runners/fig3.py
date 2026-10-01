"""Data for Fig. 3: three trajectories launched from the origin."""

from common import H0, XM, compile_program, command, parse_args, run_all

# (L, vy0 = sin(theta))
RUNS = [(10.0, 0.707107), (10.0, 0.469472), (2.0, 0.707107)]
NMAX = 300


def main():
    args = parse_args(__doc__)
    compile_program("collisions", args.cc, args.dry_run)
    commands = [command("collisions", L=L, y0=0.0, vy0=vy0, nmax=NMAX,
                        h0=H0, xm=XM)
                for L, vy0 in RUNS]
    run_all(commands, args.jobs, args.dry_run)


if __name__ == "__main__":
    main()
