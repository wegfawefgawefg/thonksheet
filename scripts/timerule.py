#!/usr/bin/env python3
"""Why competitive learning lifts the time rows, and how far (idea 166). See timerule.sh.
docs/results-2026-11-25-timerule.md has the tables this prints."""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from timerule_lib import (PRED, ROOT, SUITE, delayed, fmt, grid, grid_table,  # noqa: E402
                          mean, per_seed_table, seeds_of)
import timerule_probe  # noqa: E402  points 3 and 4: blobprobe transfer and timewire

# ---- 1. reproduce both ----

def repro(opt):
    rows = [("suite delayed echo d2", delayed(2), SUITE),
            ("suite order", ["--task", "order"], SUITE),
            ("suite gap", ["--task", "gap"], SUITE),
            ("pred delayed echo d4", delayed(4), PRED)]
    res = grid(os.path.join(opt["out"], "repro"), rows, ["off", "on"], opt, baselines=True)
    return per_seed_table("## 1. Reproduce (scripts/timerule.sh repro)", rows, ["off", "on"],
                          res, opt)


# ---- 2. the walk from one setting to the other, one difference at a time ----

def walk_rows():
    rows = [("suite (side 3, std, spread 128, d2, off+tw, 40k)", delayed(2), SUITE)]
    one = [("side", 2), ("lattice", "pred"), ("spread", 64), ("delay", 4), ("offtw", 0),
           ("steps", 20000)]
    for k, v in one:
        rows.append(("suite, %s %s" % (k, v), delayed(v if k == "delay" else 2),
                     dict(SUITE, **{k: v})))
    rows.append(("pred (side 2, pred, spread 64, d4, off bare, 20k)", delayed(4), PRED))
    for k, _ in one:
        v = SUITE[k]
        rows.append(("pred, %s %s" % (k, v), delayed(v if k == "delay" else 4),
                     dict(PRED, **{k: v})))
    for d in (8, 16):
        rows.append(("suite, delay %d" % d, delayed(d), dict(SUITE, delay=d)))
        rows.append(("pred, delay %d" % d, delayed(d), dict(PRED, delay=d)))
    # at 2 steps the prediction sheet fails too: which part of its shape. one factor back
    # from it, and two factors changed from the suite's
    p2 = dict(PRED, delay=2)
    for k, _ in one:
        if k != "delay":
            rows.append(("pred d2, %s %s" % (k, SUITE[k]), delayed(2), dict(p2, **{k: SUITE[k]})))
    for a, b in (("side", "lattice"), ("side", "spread"), ("lattice", "spread")):
        rows.append(("suite, %s %s + %s %s" % (a, PRED[a], b, PRED[b]), delayed(2),
                     dict(SUITE, **{a: PRED[a], b: PRED[b]})))
    return rows


def walk(opt):
    rows = walk_rows()
    arms = ["off", "on", "churn"]
    res = grid(os.path.join(opt["out"], "walk"), rows, arms, opt)
    return grid_table("## 2. The walk (scripts/timerule.sh walk)", rows, arms, res, opt)


def walk8(opt):
    """the cells that carry the conclusion, on 8 seeds; and 3 steps, between 2 and 4"""
    keep = ["suite (side 3, std, spread 128, d2, off+tw, 40k)", "suite, delay 4",
            "pred (side 2, pred, spread 64, d4, off bare, 20k)", "pred, delay 2",
            "pred d2, side 3", "suite, side 2 + lattice pred"]
    rows = [r for r in walk_rows() if r[0] in keep]
    rows.insert(1, ("suite, delay 3", delayed(3), dict(SUITE, delay=3)))
    arms = ["off", "on", "churn", "oncap"]
    res = grid(os.path.join(opt["out"], "walk"), rows, arms, opt)
    return grid_table("## 2b. The walk's deciding cells on 8 seeds (scripts/timerule.sh walk8)",
                      rows, arms, res, opt)


# ---- 3. is it time: freeze, shuffle, churn, rate ----

def controls(opt):
    rows = timerule_probe.CONTROL_ROWS
    lives = grid(os.path.join(opt["out"], "lives"), rows, ["off", "on", "churn", "oncap"], opt,
                 by_args=True)
    return timerule_probe.controls(opt, lives)


# ---- 5. how far it goes; 6. with the output-gap readout ----

# the map and time rows (docs/curriculum.md; scripts/suite.py, scripts/map_worlds.py) and
# the GRU reference's late score from docs/scoreboard.md (gru-fixed) where it has the row
REACH_ROWS = [
    ("delayed echo, 2 steps", delayed(2), 1.00), ("delayed echo, 3 steps", delayed(3), None),
    ("delayed echo, 4 steps", delayed(4), None),
    ("delayed echo, 8 steps", delayed(8), None), ("delayed echo, 16 steps", delayed(16), None),
    ("beat, every 4 steps", ["--task", "beat", "--period", "4"], 1.00),
    ("sequence, 8 lines x 4", ["--task", "sequence"], 0.50),
    ("copy 2 of 2", ["--task", "copy", "--k", "2", "--length", "2"], 1.00),
    ("copy 2 to 3 of 2", ["--task", "copy", "--k", "2", "--length", "3"], None),
    ("recall, 1 between", ["--task", "recall", "--k", "2", "--fill", "1"], 1.00),
    ("recall, 2 between", ["--task", "recall", "--k", "2", "--fill", "2"], None),
    ("1-back, 4 symbols", ["--task", "n-back", "--k", "4", "--back", "1"], 0.97),
    ("2-back, 4 symbols", ["--task", "n-back", "--k", "4", "--back", "2"], 0.94),
    ("same or different in turn", ["--task", "same-then", "--item-lines", "4", "--item-on", "1"],
     0.96),
    ("order", ["--task", "order"], 1.00),
    ("gap", ["--task", "gap"], 1.00),
]


def reach_table(title, arms, opt, pair):
    rows = [(label, args, SUITE) for label, args, _ in REACH_ROWS]
    res = grid(os.path.join(opt["out"], "lives"), rows, arms, opt, by_args=True)
    gru = {label: g for label, _, g in REACH_ROWS}
    lines = [title, "seeds %s, the suite's sheet and life (40,000 steps); late score, rate in "
             "brackets; GRU: docs/scoreboard.md gru-fixed" % ",".join(map(str, opt["seeds"])), "",
             "| row | " + " | ".join(arms) + " | %s - %s | seeds up | GRU |" % pair,
             "|" + "---|" * (len(arms) + 4)]
    for label, _, _ in rows:
        cells = ["%s (%s)" % (fmt(mean([x[0] for x in res[label, a]])),
                              fmt(mean([x[1] for x in res[label, a]]), 0)) for a in arms]
        d = [x[0] - y[0] for x, y in zip(res[label, pair[0]], res[label, pair[1]])
             if x[0] is not None and y[0] is not None]
        lines.append("| %s | %s | %s | %s | %s |" % (
            label, " | ".join(cells), "%+.3f" % (sum(d) / len(d)) if d else "",
            "%d/%d" % (sum(x > 0 for x in d), len(d)) if d else "", fmt(gru[label], 2)))
    return "\n".join(lines)


def reach(opt):
    return reach_table("## 5. How far it goes (scripts/timerule.sh reach)",
                       ["off", "on", "churn", "oncap"], opt, ("on", "off"))


def readout(opt):
    return reach_table("## 6. With the output-gap readout (scripts/timerule.sh readout): "
                       "--g.readout_evidence 2 --g.readout_good 2 --g.margin_percent 20 "
                       "--g.margin_win 2", ["off", "on", "goodon"], opt,
                       ("goodon", "on"))


MODES = {"repro": repro, "walk": walk, "walk8": walk8, "controls": controls,
         "wiring": timerule_probe.wiring, "reach": reach, "readout": readout}


def main():
    argv = sys.argv[1:]
    if not argv or argv[0].startswith("-"):
        sys.stderr.write(open(os.path.join(ROOT, "scripts", "timerule.sh")).read().split("set -")[0])
        return 1
    opt = {"mode": argv[0], "seeds": None, "jobs": 3, "quick": False, "table_only": False,
           "extra": []}
    i = 1
    while i < len(argv):
        a = argv[i]
        if a == "--quick":
            opt["quick"] = True
        elif a == "--table-only":
            opt["table_only"] = True
        elif a == "--seeds":
            opt["seeds"] = seeds_of(argv[i + 1])
            i += 1
        elif a == "--arms":
            opt["arms"] = argv[i + 1].split(",")
            i += 1
        elif a == "--jobs":
            opt["jobs"] = int(argv[i + 1])
            i += 1
        else:
            opt["extra"].append(a)
        i += 1
    opt["out"] = os.path.join(ROOT, "out", "timerule" + ("-quick" if opt["quick"] else ""))
    modes = list(MODES) if opt["mode"] == "all" else opt["mode"].split(",")
    texts = []
    for m in modes:
        if m not in MODES:
            print("timerule: modes are %s, all" % ", ".join(MODES))
            return 1
        o = dict(opt)
        o["seeds"] = opt["seeds"] or ([5, 6] if opt["quick"] else DEFAULT_SEEDS.get(m, [5, 6, 7, 8]))
        texts.append(MODES[m](o))
        os.makedirs(opt["out"], exist_ok=True)
        with open(os.path.join(opt["out"], m + ".md"), "w") as f:
            f.write(texts[-1] + "\n")
        print(texts[-1] + "\n", flush=True)
    return 0


DEFAULT_SEEDS = {"repro": [5, 6], "walk": [5, 6, 7, 8], "walk8": list(range(5, 13)), "controls": list(range(5, 13)),
                 "wiring": [5, 6, 7, 8], "reach": list(range(5, 13)),
                 "readout": list(range(5, 13))}

if __name__ == "__main__":
    sys.exit(main())
