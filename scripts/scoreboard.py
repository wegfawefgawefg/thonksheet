#!/usr/bin/env python3
"""Join every scorecard in out/scorecards/ into docs/scoreboard.md, one column per name.

    scripts/scoreboard.py [name ...]     only these names; default all but *-quick

Each cell is the mean over the report seeds, then the spread, then the learning-off twin's
mean in brackets. The chance column is coin flips from the first scorecard that has it.
"""

import csv
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CARDS = os.path.join(ROOT, "out", "scorecards")
OUT = os.path.join(ROOT, "docs", "scoreboard.md")


def num(text):
    try:
        return float(text)
    except ValueError:
        return None


def short(v):
    if v is None:
        return ""
    if abs(v) >= 100:
        return "%.0f" % v
    if abs(v) >= 10:
        return "%.1f" % v
    return "%.2f" % v


def read_card(path):
    about, rows = {}, []
    with open(path) as f:
        for r in csv.DictReader(f):
            if r["property"] == "about":
                about[r["test"]] = r["setting"]
            else:
                rows.append(r)
    return about, rows


def cell(r):
    mean = num(r["mean"])
    if mean is None:
        return "n/a"
    text = short(mean)
    spread = num(r["spread"])
    if spread is not None:
        text += " ±" + short(spread)
    off = num(r["learn_off"])
    if off is not None:
        text += " (%s)" % short(off)
    return text


def main():
    wanted = sys.argv[1:]
    if not os.path.isdir(CARDS):
        print("no scorecards in %s yet; run scripts/suite.sh first" % CARDS)
        return 1
    names = sorted(f[:-4] for f in os.listdir(CARDS) if f.endswith(".csv"))
    names = [n for n in names if (n in wanted if wanted else not n.endswith("-quick"))]
    if not names:
        print("no scorecards to join")
        return 1
    cards = {n: read_card(os.path.join(CARDS, n + ".csv")) for n in names}

    # rows in the order of the first scorecard, then any only later ones have
    order, props, chance = [], {}, {}
    for n in names:
        for r in cards[n][1]:
            if r["test"] not in props:
                order.append(r["test"])
                props[r["test"]] = r["property"]
            if r["test"] not in chance and num(r["chance"]) is not None:
                chance[r["test"]] = num(r["chance"])
    by_test = {n: {r["test"]: r for r in cards[n][1]} for n in names}

    lines = ["# Scoreboard", "",
             "Written by `scripts/scoreboard.sh` from `out/scorecards/`; do not edit by hand. "
             "One column per engine or variant (`scripts/suite.sh <name>`). A cell is the "
             "mean over the report seeds, ± the spread (standard deviation), and in brackets "
             "the same engine with learning off. Chance is coin flips. Scores are the late "
             "score, the mean of the last tenth of the life's windows, unless the test says "
             "otherwise. What each test is: `docs/bench.md`, `docs/curriculum.md`, "
             "`docs/notes-world.md`.", ""]
    for n in names:
        about = cards[n][0]
        lines.append("- **%s**: flags `%s`; seeds %s; commit %s, %s; %s; frozen readout: %s." % (
            n, about.get("flags", "") or "(none)", about.get("seeds", "?"),
            about.get("commit", "?"), about.get("date", "?"), about.get("wall", "?"),
            about.get("frozen readout", "?")))
    lines.append("")
    lines.append("Speed figures: " + cards[names[0]][0].get("speed", "") + ".")
    lines.append("")
    lines.append("| test | chance | " + " | ".join(names) + " |")
    lines.append("| --- | --- |" + " --- |" * len(names))
    prop = None
    for test in order:
        if props[test] != prop:
            prop = props[test]
            lines.append("| **%s** | |" % prop + " |" * len(names))
        cells = [cell(by_test[n][test]) if test in by_test[n] else "" for n in names]
        lines.append("| %s | %s | %s |" % (test, short(chance.get(test)), " | ".join(cells)))
    lines.append("")
    with open(OUT, "w") as f:
        f.write("\n".join(lines))
    print("wrote %s: %s" % (os.path.relpath(OUT, ROOT), ", ".join(names)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
