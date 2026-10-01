"""Data for Fig. 9: uncertainty fraction of the escape basins."""

from common import H0, L_VALS, XM, compile_program, command, parse_args, run_all

NIC = 10000
NMAX = 100000
EPS_MIN = 1e-12
EPS_MAX = 1e-2
N_EPS = 100
N_RUNS = 10


def main():
    args = parse_args(__doc__, openmp=True)
    compile_program("uncertainty", args.cc, args.dry_run)
    commands = [command("uncertainty", L=L, nic=NIC, nmax=NMAX,
                        eps_min=EPS_MIN, eps_max=EPS_MAX, n_eps=N_EPS,
                        run=run, h0=H0, xm=XM, nthreads=args.nthreads)
                for L in L_VALS if L <= 8.0
                for run in range(N_RUNS)]
    run_all(commands, args.jobs, args.dry_run)


if __name__ == "__main__":
    main()
