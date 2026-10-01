"""Data for Fig. 4: Poincare sections."""

from common import H0, L_FIGS, XM, compile_program, command, parse_args, run_all

NIC = 500
NCROSS = 10000
SEED = 1312


def main():
    args = parse_args(__doc__)
    compile_program("section", args.cc, args.dry_run)
    commands = [command("section", L=L, nic=NIC, ncross=NCROSS, seed=SEED,
                        h0=H0, xm=XM)
                for L in L_FIGS]
    run_all(commands, args.jobs, args.dry_run)


if __name__ == "__main__":
    main()
