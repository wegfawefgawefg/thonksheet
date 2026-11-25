#!/usr/bin/env python3
"""Score tables for idea 57 from out/sign/<engine>/<version>/<task>/s<seed>.log.

scripts/sign_summary.py table|blob|minimal. Late score = the mean of the last 4 of 40 windows,
as blobrun prints it. Shuffled echo is split into straight lives (cue 0 to button 0) and
crossed ones by the task seed, which is the seed here: seeds 6, 7 and 11 of 5-12 are crossed.
"""

import math
import os
import sys

SEEDS = range(5, 13)
CROSSED = {6, 7, 11}
TASKS = ["echo2", "echo4", "shuffled", "apart2"]


def late(path, row):
    try:
        with open(path) as f:
            for line in f:
                parts = line.split()
                if len(parts) == 4 and parts[0] == row:
                    return float(parts[2])
    except OSError:
        pass
    return None


def mean_sd(xs):
    if not xs:
        return None, None
    m = sum(xs) / len(xs)
    sd = math.sqrt(sum((x - m) ** 2 for x in xs) / (len(xs) - 1)) if len(xs) > 1 else 0.0
    return m, sd


def fmt(m):
    return "" if m is None else f"{m:.2f}"


def main():
    engine = sys.argv[1]
    root = f"out/sign/{engine}"
    on, off = ("table", "table_off") if engine == "table" else ("blob", "learn_off")
    versions = sorted(os.listdir(root), key=lambda v: (v != "default", v))
    scores = {}  # (version, task) -> {seed: (on, off)}
    for v in versions:
        for t in TASKS:
            per = {}
            for s in SEEDS:
                p = f"{root}/{v}/{t}/s{s}.log"
                a = late(p, on)
                if a is not None:
                    per[s] = (a, late(p, off))
            scores[(v, t)] = per

    cols = [("echo k=2", "echo2", None), ("echo k=4", "echo4", None),
            ("shuffled, straight", "shuffled", False), ("shuffled, crossed", "shuffled", True),
            ("2 rows apart", "apart2", None)]
    print(f"\n### {engine}: late score, mean over seeds 5-12 (learning-off twin)\n")
    print("| version | " + " | ".join(c[0] for c in cols) + " |")
    print("|---" * (len(cols) + 1) + "|")
    for v in versions:
        cells = []
        for _, t, crossed in cols:
            per = scores[(v, t)]
            seeds = [s for s in per if crossed is None or (s in CROSSED) == crossed]
            m, _ = mean_sd([per[s][0] for s in seeds])
            offs = [per[s][1] for s in seeds if per[s][1] is not None]
            mo, _ = mean_sd(offs)
            cell = fmt(m)
            if mo is not None:
                cell += f" ({mo:.2f})"
            if seeds and len(seeds) < (3 if crossed else 5 if crossed is False else 8):
                cell += f" [{len(seeds)} lives]"
            cells.append(cell)
        print(f"| {v} | " + " | ".join(cells) + " |")

    print(f"\nPaired difference to `default`, seed by seed: mean (t = mean / standard error).\n")
    print("| version | " + " | ".join(c[0] for c in cols) + " |")
    print("|---" * (len(cols) + 1) + "|")
    for v in versions:
        if v == "default":
            continue
        cells = []
        for _, t, crossed in cols:
            a, b = scores[(v, t)], scores[("default", t)]
            seeds = [s for s in a if s in b and (crossed is None or (s in CROSSED) == crossed)]
            d = [a[s][0] - b[s][0] for s in seeds]
            m, sd = mean_sd(d)
            if m is None:
                cells.append("")
            elif len(d) < 2 or sd == 0:
                cells.append(f"{m:+.2f}")
            else:
                cells.append(f"{m:+.2f} ({m / (sd / math.sqrt(len(d))):+.1f})")
        print(f"| {v} | " + " | ".join(cells) + " |")

    print("\nLate score per seed, 5 to 12 (crossed shuffled lives are 6, 7, 11):\n")
    print("```")
    for t in TASKS:
        for v in versions:
            per = scores[(v, t)]
            row = " ".join(f"{per[s][0]:.2f}" if s in per else "  - " for s in SEEDS)
            print(f"{t:9s} {v:16s} {row}")
        twin_v = [v for v in versions if any(x[1] is not None for x in scores[(v, t)].values())]
        for v in twin_v:
            per = scores[(v, t)]
            row = " ".join(f"{per[s][1]:.2f}" if s in per and per[s][1] is not None else "  - "
                           for s in SEEDS)
            print(f"{t:9s} {'off (' + v + ')':16s} {row}")
    print("```")


if __name__ == "__main__":
    main()
