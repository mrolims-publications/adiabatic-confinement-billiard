"""Data for Fig. 3(d): transverse action along the orbits of Figs. 3(a)-(c)."""

from common import H0, XM, compile_program, command, parse_args, run_all
from fig3abc import NMAX, RUNS


def main():
    args = parse_args(__doc__)
    compile_program("action", args.cc, args.dry_run)
    commands = [
        command("action", L=L, y0=0.0, vy0=vy0, nmax=NMAX, h0=H0, xm=XM)
        for L, vy0 in RUNS
    ]
    run_all(commands, args.jobs, args.dry_run)


if __name__ == "__main__":
    main()
