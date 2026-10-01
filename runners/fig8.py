"""Data for Fig. 8: escape collision numbers and escape basins."""

from common import H0, L_FIGS, XM, compile_program, command, parse_args, run_all

GRID_SIZE = 1000
NMAX = 1000000


def main():
    args = parse_args(__doc__, openmp=True)
    compile_program("basin", args.cc, args.dry_run)
    commands = [command("basin", L=L, grid_size=GRID_SIZE, nmax=NMAX,
                        h0=H0, xm=XM, nthreads=args.nthreads)
                for L in L_FIGS]
    run_all(commands, args.jobs, args.dry_run)


if __name__ == "__main__":
    main()
