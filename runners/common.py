"""Shared parameters and helpers for the runner scripts."""

import argparse
import os
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "build" / "bin"
LOGS = ROOT / "logs"

H0 = 1.0
XM = 10.0

L_VALS = [0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0,
          8.0, 9.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0, 25.0, 30.0]
L_FIGS = [1.0, 3.0, 5.0, 7.0, 10.0, 20.0]


def parse_args(description, openmp=False):
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("--cc", default="gcc",
                        help="C compiler (default: gcc)")
    parser.add_argument("--jobs", type=int, default=1,
                        help="number of runs executed simultaneously "
                             "(default: 1)")
    if openmp:
        parser.add_argument("--nthreads", type=int, default=os.cpu_count(),
                            help="OpenMP threads per run "
                                 "(default: all available)")
    parser.add_argument("--dry-run", action="store_true",
                        help="print the commands without running them")

    return parser.parse_args()


def compile_program(name, cc, dry_run=False):
    if dry_run:
        return
    subprocess.run(["make", f"CC={cc}", name], cwd=ROOT, check=True)


def command(name, **kwargs):
    """Command line of build/bin/<name>.x with --key value arguments."""

    args = [f"./build/bin/{name}.x"]
    for key, value in kwargs.items():
        args += [f"--{key}", str(value)]

    return args


def run_all(commands, jobs=1, dry_run=False):
    """Run the commands from the repository root, `jobs` at a time.

    The output of each run is written to logs/.
    """

    LOGS.mkdir(exist_ok=True)

    def run(cmd):
        line = " ".join(cmd)
        print(line, flush=True)
        if dry_run:
            return
        tags = [f"{k[2:]}={v}" for k, v in zip(cmd[1::2], cmd[2::2])
                if k not in ("--h0", "--xm", "--nthreads")]
        log = LOGS / ("_".join([Path(cmd[0]).stem] + tags) + ".log")
        with open(log, "w") as f:
            subprocess.run(cmd, cwd=ROOT, stdout=f, stderr=subprocess.STDOUT,
                           check=True)

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        list(pool.map(run, commands))
