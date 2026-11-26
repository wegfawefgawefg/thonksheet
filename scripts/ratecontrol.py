#!/usr/bin/env python3
"""A rate control that does not fight the rest (idea 175). See ratecontrol.sh; the tables of
docs/results-2026-11-26-rate-control.md. The five tests are in ratecontrol_tests.py."""

import os
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ratecontrol_tests as T  # noqa: E402
from timerule_lib import ROOT, fmt, mean, seeds_of  # noqa: E402

CAP = "--g.threshold_cap 1000"
BAND = "--g.rate_band_shift 11"
GAIN = "--g.sector_tiles 1 --g.sector_smooth_shift 2 --g.sector_gain 8 --g.sector_step_shift 8"
SLOW = "--g.sector_smooth_shift 8 --g.sector_step_shift 6"
PERIOD = "--g.threshold_reg_until 131072"
# name: (what it is, genome numbers). std is the standard control
CANDIDATES = {
    "std": ("the standard control", ""),
    "e": ("the cap lifted", CAP),
    "a": ("dead band, a quarter to four times, 2,048-tick windows", BAND + " " + CAP),
    "a8": ("dead band to eight times", BAND + " --g.rate_band_high 3 " + CAP),
    "b": ("critical period of 131,072 ticks, then thresholds stop", PERIOD + " " + CAP),
    "ba": ("critical period, then the dead band", PERIOD + " " + BAND + " " + CAP),
    "c": ("inhibition in proportion per tile, slow part, per-cell walk", GAIN + " " + CAP),
    "d": ("slow sector bias, per-cell walk", SLOW + " " + CAP),
    "ac": ("dead band and inhibition in proportion", BAND + " " + GAIN + " " + CAP),
    "ad": ("dead band and slow sector bias", BAND + " " + SLOW + " " + CAP),
}
TESTS = ["alive", "read", "latch", "pred", "map"]
RUN = {"alive": T.alive, "read": T.read, "latch": T.latch, "pred": T.pred, "map": T.quick_map}
DEFAULT_SEEDS = {"alive": [5, 6, 7, 8], "read": list(range(5, 13)), "latch": [5, 6, 7, 8],
                 "pred": [5, 6, 7, 8], "map": [5, 6, 7, 8]}
# the pass lines (written with the predictions, before any scoring run was read)
FROZEN = (0.9376, 0.6938)
HALF = (0.931, 0.684)


def m(v, key=None):
    got = [x.get(key) if key else x for x in v if x and (not key or x.get(key) is not None)]
    return statistics.mean(got) if got else None


# ---- what each test says, and whether it passes ----

def alive_row(res):
    """worst over settings and worlds of the 160,000-step lives; the MNIST world's gap jump"""
    worst = {"rate_lo": 9.0, "rate_hi": 0.0, "silent": 0.0, "stuck": 0.0, "cap": 0.0}
    detail, ok = [], True
    for sname in ("base", "comp", "best"):
        for world in ("far3", "delayed2", "mnist"):
            v = res.get((sname, world), [])
            rate, sil, stk, cap = m(v, "rate"), m(v, "silent"), m(v, "stuck"), m(v, "cap")
            if rate is None:
                ok = False
                continue
            detail.append((sname, world, rate, sil, stk, cap, m(v, "silent_hidden"),
                           m(v, "silent_memory"), m(v, "jump"), m(v, "late")))
            worst["rate_lo"] = min(worst["rate_lo"], rate)
            worst["rate_hi"] = max(worst["rate_hi"], rate)
            worst["cap"] = max(worst["cap"], cap)
            limit = 0.02 if sname != "best" else 0.13
            ok = ok and 0.75 <= rate <= 1.33 and cap < 0.01 and sil < limit and stk < limit
            if sname != "best":
                worst["silent"] = max(worst["silent"], sil)
                worst["stuck"] = max(worst["stuck"], stk)
    jumps = [d[8] for d in detail if d[1] == "mnist" and d[8] is not None]
    worst["jump"] = max(jumps) if jumps else None
    ok = ok and worst["jump"] is not None and worst["jump"] <= 1.5
    s0 = [x for x in res.get("stage0", []) if x]
    dead = sum(p["dead"] + p["satur"] for x in s0 for p in x.values())
    worst["stage0"] = dead
    ok = ok and bool(s0) and dead == 0
    return worst, detail, ok


def read_row(res):
    mn, pr = m(res["mnist"], "reader"), m(res["pair"], "reader")
    ok = mn is not None and pr is not None and mn >= HALF[0] and pr >= HALF[1]
    return (mn, pr, m(res["mnist"], "corr"), m(res["mnist"], "rate")), ok


def latch_row(res, name):
    traces = res["traces"]
    key = "loop inputs all strong, cap 1000" if name == "std" else list(traces)[-1]
    p = [x for x in traces[key] if x["vote"]]
    vote = lambda b: mean([x["vote"][b] for x in p if len(x["vote"]) > b])  # noqa: E731
    rate = mean([x["state"][0] for x in p if "state" in x])
    capped = mean([x["state"][2] for x in p if "state" in x])
    echo = {k: mean([x[0] for x in v]) for k, v in res["echo"].items()}
    v1, v8, v16 = vote(1), vote(8), vote(16)
    ok = (v8 is not None and v8 >= 0.90 and v1 is not None and v1 >= 0.95 and rate is not None
          and rate <= 1.33 * 32 and (capped or 0) < 0.01)
    e2 = echo.get("delayed echo, 2 steps")
    ok = ok and e2 is not None and e2 >= 0.913 - 0.03
    return (v1, vote(4), v8, v16, rate, echo), ok


def pred_row(res):
    p = [x for x in res if "state" in x]
    if not p:
        return None, False
    st = [mean([x["state"][i] for x in p]) for i in range(6)]
    ok = st[0] <= 1.25 * 32 and st[2] < 0.01 and st[4] < 0.5
    return st, ok


def map_row(res, rows):
    out, ok = {}, True
    for col in T.MAP_COLS:
        n = {}
        for tag in ("without", "with"):
            got = [res.get((col, tag, r)) for r in rows]
            n[tag] = None if any(g is None or g[1] is None for g in got) else sum(
                g[1] for g in got)
        diffs = {}
        for r in T.MUST:
            a, b = res.get((col, "with", r)), res.get((col, "without", r))
            if a and b and None not in a[0] and None not in b[0]:
                diffs[r] = statistics.mean(a[0]) - statistics.mean(b[0])
        out[col] = (n["with"], n["without"], diffs)
        ok = ok and n["with"] is not None and n["without"] is not None and n["with"] >= n[
            "without"] and all(d > -0.03 for d in diffs.values()) and len(diffs) == len(T.MUST)
    return out, ok


# ---- tables ----

def run_tests(name, opt, tests):
    cand = CANDIDATES[name][1].split()
    out = {}
    for t in tests:
        o = dict(opt)
        o["seeds"] = opt["seeds"] or ([5] if opt["quick"] else DEFAULT_SEEDS[t])
        out[t] = RUN[t](name, cand, o)
    return out


def summary(names, opt, tests):
    head = ["| candidate | 1 alive: rate (lowest-highest of target), silent / stuck (i, ii), at "
            "cap, gap jump, stage 0 | 2 read: MNIST / pairs (self-corr, rate) | 3 latch: vote "
            "b1 / b4 / b8 / b16, rate; delayed echo 2 / 3 / 4 / 8 | 4 pred: rate, at cap, bias "
            "at +max | 5 map: rows passed with (without) base / a2rucomp / best; worst of the "
            "four | passes |", "|---|---|---|---|---|---|---|"]
    detail = []
    for name in names:
        r = run_tests(name, dict(opt, table_only=True), tests)
        cells, passes = [], 0
        if "alive" in r:
            w, d, ok = alive_row(r["alive"])
            passes += ok
            cells.append("%s-%s, %s / %s, %s, %s, %s%s" % (
                fmt(w["rate_lo"], 2), fmt(w["rate_hi"], 2), fmt(w["silent"], 3),
                fmt(w["stuck"], 3), fmt(w["cap"], 3), fmt(w["jump"], 2), w["stage0"],
                " **pass**" if ok else ""))
            detail.append((name, d))
        if "read" in r:
            (mn, pr, corr, rate), ok = read_row(r["read"])
            passes += ok
            cells.append("%s / %s (%s, %s)%s" % (fmt(mn, 4), fmt(pr, 4), fmt(corr), fmt(rate, 0),
                                                 " **pass**" if ok else ""))
        if "latch" in r:
            (v1, v4, v8, v16, rate, echo), ok = latch_row(r["latch"], name)
            passes += ok
            cells.append("%s / %s / %s / %s, %s; %s%s" % (
                fmt(v1, 2), fmt(v4, 2), fmt(v8, 2), fmt(v16, 2), fmt(rate, 1),
                " / ".join(fmt(echo.get("delayed echo, %d steps" % k), 2) for k in (2, 3, 4, 8)),
                " **pass**" if ok else ""))
        if "pred" in r:
            st, ok = pred_row(r["pred"])
            passes += ok
            cells.append("-" if not st else "%s, %s, %s%s" % (
                fmt(st[0], 1), fmt(st[2], 3), fmt(st[4], 2), " **pass**" if ok else ""))
        if "map" in r:
            cols, ok = map_row(*r["map"])
            passes += ok
            worst = min([d for c in cols.values() for d in c[2].values()] or [0])
            cells.append("%s; %+.3f%s" % (" / ".join(
                "%s (%s)" % (c[0], c[1]) for c in cols.values()), worst, " **pass**" if ok else ""))
        head.append("| %s | %s | %d |" % (name, " | ".join(cells), passes))
    lines = head + ["", "Alive in detail (mean of the seeds): setting, world, rate over target, "
                    "silent, stuck, at cap, silent plain hidden, silent integrator and time "
                    "cells, gap jump, late", ""]
    for name, d in detail:
        for x in d:
            lines.append("- %s %s %s: %s" % (name, x[0], x[1], ", ".join(fmt(v, 3) for v in x[2:])))
    return "\n".join(lines)


def main():
    argv = sys.argv[1:]
    if not argv or argv[0].startswith("-"):
        sys.stderr.write(open(os.path.join(ROOT, "scripts", "ratecontrol.sh")).read().split(
            "set -")[0])
        return 1
    opt = {"seeds": None, "jobs": 3, "quick": False, "table_only": False, "extra": [],
           "tests": TESTS}
    what, i = argv[0], 1
    while i < len(argv):
        a = argv[i]
        if a in ("--quick", "--table-only"):
            opt[a[2:].replace("-", "_")] = True
        elif a in ("--seeds", "--jobs", "--tests"):
            v = argv[i + 1]
            opt[a[2:]] = seeds_of(v) if a == "--seeds" else int(v) if a == "--jobs" else v.split(",")
            i += 1
        else:
            print("ratecontrol: unknown option %s" % a)
            return 1
        i += 1
    opt["out"] = os.path.join(ROOT, "out", "ratecontrol" + ("-quick" if opt["quick"] else ""))
    if what == "smoke":
        names = ["std", "ac"]
    elif what in ("table", "all"):
        names = list(CANDIDATES)
    else:
        names = what.split(",")
    bad = [n for n in names if n not in CANDIDATES]
    if bad or any(t not in TESTS for t in opt["tests"]):
        print("ratecontrol: candidates are %s; tests %s" % (", ".join(CANDIDATES), ", ".join(TESTS)))
        return 1
    if what != "table" and not opt["table_only"]:
        for name in names:
            print("ratecontrol: %s (%s)" % (name, CANDIDATES[name][0]), flush=True)
            run_tests(name, opt, opt["tests"])
    text = summary(names, opt, opt["tests"])
    os.makedirs(opt["out"], exist_ok=True)
    with open(os.path.join(opt["out"], "table-%s.md" % what.replace(",", "+")), "w") as f:
        f.write(text + "\n")
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
