#!/usr/bin/env python3
"""The fixed reference networks against the home-grown engines, row by row of the suite.

    scripts/reference_compare.py [--ref mlp-fixed,gru-fixed] [--home blob-main,...]

Reads out/scorecards/*.csv. A row is "learned" when the mean beats both the learning-off
twin and chance by at least 0.05 and by two standard errors (seeds); for the cartpole rows
(steps per fall) by 10% instead of 0.05. Prints one markdown table per property: the best
reference, the best home-grown engine, and which is ahead. Rows a reference cannot run (the
sheet's own measurements: reach, room, aliveness, speed) are left out.
"""

import argparse
import csv
import math
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CARDS = os.path.join(ROOT, "out", "scorecards")
HOME = ["blob-main", "blob-readout", "minimal", "table"]
REF = ["mlp-fixed", "gru-fixed"]
# the sheet's own measurements, or rows that need a frozen sheet
SKIP_PROPS = {"room", "stays alive", "fits the machine"}
SKIP_TESTS = ("cue readable", "frozen sheet", "capacity: largest k", "surprise falling")


def load(name):
    path = os.path.join(CARDS, name + ".csv")
    rows = {}
    if not os.path.exists(path):
        return None
    with open(path) as f:
        for r in csv.DictReader(f):
            if r["property"] == "about":
                continue
            rows[r["test"]] = r
    return rows


def num(r, key):
    try:
        return float(r[key])
    except (KeyError, ValueError, TypeError):
        return None


def per_fall(test):
    return "steps per fall" in test


def learned(r, test):
    """mean minus the better of learning-off and chance, and whether that is clear"""
    mean, spread, off, chance = (num(r, k) for k in ("mean", "spread", "learn_off", "chance"))
    if mean is None:
        return None, False
    floor = max(v for v in (off, chance) if v is not None) if (off, chance) != (None, None) \
        else None
    if floor is None:
        return None, False
    n = max(1.0, num(r, "seeds") or 1.0)
    se = (spread or 0.0) / math.sqrt(n)
    gain = mean - floor
    need = 0.10 * floor if per_fall(test) else 0.05
    return gain, gain > need and gain > 2 * se


def fmt(v, test):
    if v is None:
        return "n/a"
    return "%.1f" % v if per_fall(test) or abs(v) >= 10 else "%.2f" % v


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ref", default=",".join(REF))
    ap.add_argument("--home", default=",".join(HOME))
    opt = ap.parse_args()
    refs = [(n, load(n)) for n in opt.ref.split(",")]
    homes = [(n, load(n)) for n in opt.home.split(",")]
    refs = [(n, c) for n, c in refs if c]
    homes = [(n, c) for n, c in homes if c]
    tests, props = [], {}
    for _, card in refs:
        for test, r in card.items():
            if test not in props:
                tests.append(test)
                props[test] = r["property"]
    prop = None
    tally = {"ref": 0, "home": 0, "both": 0, "neither": 0}
    for test in tests:
        if props[test] in SKIP_PROPS or any(s in test for s in SKIP_TESTS):
            continue
        if props[test] != prop:
            prop = props[test]
            print("\n### %s\n" % prop)
            print("| world | chance | " + " | ".join(n for n, _ in refs) +
                  " | best home-grown | ahead |")
            print("|---|---|" + "---|" * len(refs) + "---|---|")
        cells, best_ref, ref_ok = [], None, False
        chance = None
        for name, card in refs:
            r = card.get(test)
            if not r:
                cells.append("n/a")
                continue
            chance = num(r, "chance") if chance is None else chance
            gain, ok = learned(r, test)
            mean = num(r, "mean")
            cells.append(fmt(mean, test) + (" **" if ok else ""))
            if mean is not None and (best_ref is None or mean > best_ref):
                best_ref = mean
            ref_ok = ref_ok or ok
        best_home, home_name, home_ok, home_spread = None, "", False, 0.0
        for name, card in homes:
            r = card.get(test)
            if not r or num(r, "mean") is None:
                continue
            gain, ok = learned(r, test)
            if best_home is None or num(r, "mean") > best_home:
                best_home, home_name = num(r, "mean"), name
                home_spread = num(r, "spread") or 0.0
            home_ok = home_ok or ok
        # ahead: only one side learned it, or both did and one is clearly higher
        if best_home is None:
            ahead = "no home-grown row"
        else:
            margin = 0.10 * max(abs(best_home), 1.0) if per_fall(test) else 0.05
            diff = (best_ref or 0.0) - best_home
            if ref_ok and home_ok:
                ahead = ("reference" if diff > margin else
                         "home-grown" if diff < -margin else "even")
            else:
                ahead = ("reference" if ref_ok else "home-grown" if home_ok else "neither")
            key = ("both" if ref_ok and home_ok else "ref" if ref_ok else
                   "home" if home_ok else "neither")
            tally[key] += 1
        home_cell = "n/a" if best_home is None else "%s %s%s" % (
            fmt(best_home, test), home_name, " **" if home_ok else "")
        print("| %s | %s | %s | %s | %s |" % (test, fmt(chance, test), " | ".join(cells),
                                             home_cell, ahead))
    print("\n** learned: beats learning-off and chance clearly (see the top of this script).")
    print("rows learned by: a reference only %d, a home-grown engine only %d, both %d, "
          "neither %d" % (tally["ref"], tally["home"], tally["both"], tally["neither"]))


if __name__ == "__main__":
    main()
