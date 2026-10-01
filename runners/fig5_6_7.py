"""Data for Figs. 5, 6 and 7: escape collision numbers."""

from common import H0, L_VALS, XM, compile_program, command, parse_args, run_all

NIC = 1000000
NMAX = 1000000


def main():
    args = parse_args(__doc__)
    compile_program("escape", args.cc, args.dry_run)
    commands = [command("escape", L=L, nic=NIC, nmax=NMAX, h0=H0, xm=XM)
                for L in L_VALS]
    run_all(commands, args.jobs, args.dry_run)


if __name__ == "__main__":
    main()
