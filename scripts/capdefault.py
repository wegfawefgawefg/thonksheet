#!/usr/bin/env python3
"""The threshold cap lifted: decide the default (idea 180;
docs/results-2026-11-26-cap-default.md). See capdefault.sh. The chain and the rate parts run
`blobprobe capsplit` (src/probe/cap_split.cpp): a quick-map life with the cap split by kind
of cell, and what the output does step by step. The map, the inside test and the speed check
are in capdefault_more.py."""

import os
import re
import statistics
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oneset as O  # noqa: E402

ROOT = O.ROOT
PROBE = os.path.join(ROOT, "build", "blobprobe")
CAP = "--g.threshold_cap 1000"
# an honest higher target: the per-cell walk has its own target, up chance over down chance
# (2^-4 a spike against 2^-9 a tick is 32 per 1,024), so the sector target and the walk's
# down chance move together. only powers of two can be asked for
T64 = "--g.sector_target_per_1024 64 --g.threshold_down_shift 8"
T128 = "--g.sector_target_per_1024 128 --g.threshold_down_shift 7"
T16 = "--g.sector_target_per_1024 16 --g.threshold_down_shift 10"
# arm: (what, genome numbers, the probe's own flags, world flags)
ARMS = {
    "c64": ("cap 64 (the default)", "", "", ""),
    "c1000": ("cap 1000", CAP, "", ""),
    "motorfree": ("cap 1000 on motor cells, 64 on the rest", CAP,
                  "--cap-hidden 64 --cap-memory 64", ""),
    "hiddenfree": ("cap 64 on motor cells, 1000 on the rest", CAP, "--cap-motor 64", ""),
    "holdmotor": ("cap 1000, motor cells' thresholds held as born (no walk on them)", CAP,
                  "--hold-motor 1", ""),
    "holdmotor64": ("cap 64, motor cells' thresholds held as born", "", "--hold-motor 1", ""),
    "bias64": ("cap 64, the rate held by the sector bias (room to 64)",
               "--g.sector_bias_max 64", "", ""),
    "t64": ("cap 1000, target 64", CAP + " " + T64, "", ""),
    "t128": ("cap 1000, target 128", CAP + " " + T128, "", ""),
    "s64": ("cap 1000, sector target 64 alone (the walk still at 32)",
            CAP + " --g.sector_target_per_1024 64", "", ""),
    "c64t64": ("cap 64, target 64", T64, "", ""),
    "c64t16": ("cap 64, target 16", T16, "", ""),
    "c128": ("cap 128", "--g.threshold_cap 128", "", ""),
    "c256": ("cap 256", "--g.threshold_cap 256", "", ""),
    "k3-64": ("three buttons, cap 64", "", "", "--k 3"),
    "k3-1000": ("three buttons, cap 1000", CAP, "", "--k 3"),
    "k4-1000": ("four buttons, cap 1000", CAP, "", "--k 4"),
    "nocue-64": ("no stage line, cap 64", "", "", "--chain-cue 0"),
    "nocue-1000": ("no stage line, cap 1000", CAP, "", "--chain-cue 0"),
}
COLS = ["base", "a2rucomp", "best"]
# the parts that run the probe: (rows, columns, arms, seeds)
CHAIN = [
    (["chain"], COLS, ["c64", "c1000", "motorfree", "hiddenfree", "holdmotor", "bias64",
                       "k3-64", "k3-1000", "k4-1000", "nocue-64", "nocue-1000"],
     list(range(5, 13))),
    (["chain"], COLS, ["c64", "c1000", "k3-1000"], [1, 2, 3, 4]),
    (["chain"], ["critic"], ["c64", "c1000", "k3-64", "k3-1000"], list(range(5, 13))),
]
RATE = [
    (["mnist", "far3", "far5"], COLS, ["c64", "c1000", "t64", "t128", "s64", "bias64",
                                       "hiddenfree", "motorfree"], [5, 6, 7, 8]),
]


def seeds_of(text):
    out = []
    for part in text.split(","):
        if "-" in part:
            a, b = part.split("-")
            out += list(range(int(a), int(b) + 1))
        elif part:
            out.append(int(part))
    return out


def mean(v):
    got = [x for x in v if x is not None]
    return statistics.mean(got) if got else None


def fmt(v, d=3):
    return "-" if v is None else ("%." + str(d) + "f") % v


def run_job(job):
    """(command, output file): written to .part and renamed when the run finished"""
    cmd, out = job
    if os.path.exists(out):
        return
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out + ".part", "w") as f:
        f.write(" ".join(cmd[3:]) + "\n")
        f.flush()
        r = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT)
    if r.returncode == 0:
        os.replace(out + ".part", out)


def pool(jobs, n):
    todo = [j for j in jobs if not os.path.exists(j[1])]
    if todo:
        print("capdefault: %d runs, %d at a time" % (len(todo), n), file=sys.stderr, flush=True)
    with ThreadPoolExecutor(n) as ex:
        list(ex.map(run_job, todo))


# ---- the probe ----

def look_file(opt, row, col, arm, seed):
    return os.path.join(opt["out"], "look", "%s-%s-%s-s%d.txt" % (row, col, arm, seed))


def look_job(opt, rows, cols, row, col, arm, seed):
    _, genome, probe, world = ARMS[arm]
    flags = O.merged(O.LIFE, rows[row], world.split(), cols[col] + genome.split())
    if opt["quick"]:
        flags = O.merged(flags, ["--steps", "600" if row == "mnist" else "2000", "--window",
                                 "200"])
    own, life = [], []
    for i in range(0, len(flags) - 1, 2):
        (own if flags[i].startswith("--g.") else life).extend(flags[i:i + 2])
    cmd = (["nice", "-n", "19", PROBE, "capsplit", "--seed", str(seed), "--life", " ".join(life)]
           + (["--look-steps", "400"] if opt["quick"] else []) + probe.split() + own)
    return cmd, look_file(opt, row, col, arm, seed)


def parse_look(path):
    if not os.path.exists(path):
        return None
    text = open(path).read()
    out = {}
    m = re.search(r"^life: late ([0-9.]+)", text, re.M)
    if not m:
        return None
    out["late"] = float(m.group(1))
    m = re.search(r"^look: \d+ steps, rate ([0-9.]+)", text, re.M)
    out["rate"] = float(m.group(1)) if m else None
    for k, cells, rate, sil, stuck, thr, top, high in re.findall(
            r"^kind (\w+): (\d+) cells, rate ([0-9.]+) per 1024 cell-ticks, silent ([0-9.]+), "
            r"stuck ([0-9.]+), threshold mean ([0-9.]+), highest (\d+), at 64 or more ([0-9.]+)",
            text, re.M):
        out[k] = {"rate": float(rate), "silent": float(sil), "stuck": float(stuck),
                  "thr": float(thr), "top": int(top), "high": float(high)}
    m = re.search(r"^sector bias mean (-?[0-9.]+), at \+max ([0-9.]+) \(target (\d+)", text, re.M)
    if m:
        out["bias"], out["biastop"], out["target"] = float(m.group(1)), float(m.group(2)), int(
            m.group(3))
    out["button"] = {int(b): float(t) for b, t in re.findall(
        r"^button (\d+): \d+ cells, threshold mean ([0-9.]+)", text, re.M)}
    m = re.search(r"^chain: A (\d+) B (\d+)", text, re.M)
    if m:
        out["A"], out["B"] = int(m.group(1)), int(m.group(2))
    out["right"] = {}
    for r, steps, pressed, spikes in re.findall(
            r"^right (-?\d+): (\d+) steps; pressed(.*?); spikes(.*)$", text, re.M):
        out["right"][int(r)] = {
            "steps": int(steps),
            "pressed": {int(a): float(b) for a, b in re.findall(r"(-?\d+):([0-9.]+)", pressed)},
            "spikes": {int(a): float(b) for a, b in re.findall(r"(\d+):([0-9.]+)", spikes)}}
    m = re.search(r"alike \(cosine\) ([0-9.]+); cells firing 4 times more on one than the other "
                  r"([0-9.]+)", text)
    if m:
        out["alike"], out["stagecells"] = float(m.group(1)), float(m.group(2))
    return out


def chain_numbers(p):
    """press A on stage 0, press B on stage 0, another button on stage 0, B on stage 1"""
    if not p or "A" not in p:
        return None
    a, b = p["A"], p["B"]
    s0 = p["right"].get(a, {"pressed": {}, "spikes": {}})["pressed"]
    s1 = p["right"].get(b, {"pressed": {}, "spikes": {}})["pressed"]
    other = sum(v for k, v in s0.items() if k not in (a, b, -1))
    return {"a0": s0.get(a), "b0": s0.get(b), "other0": other if s0 else None, "b1": s1.get(b),
            "thrA": p["button"].get(a), "thrB": p["button"].get(b),
            "votes0": p["right"].get(a, {}).get("spikes", {}),
            "votes1": p["right"].get(b, {}).get("spikes", {})}


def run_part(part, opt):
    rows, cols = O.read_map(O.MORE)
    rows = dict(rows)
    jobs = []
    for rnames, cnames, arms, seeds in part:
        seeds = [5] if opt["quick"] else [s for s in seeds if not opt["seeds"] or s in opt["seeds"]]
        arms = arms[:2] if opt["quick"] else arms
        for row in rnames[:1] if opt["quick"] else rnames:
            for col in cnames[:1] if opt["quick"] else cnames:
                for arm in arms:
                    for s in seeds:
                        jobs.append(look_job(opt, rows, cols, row, col, arm, s))
    # each run once; the long lives first
    jobs = list({j[1]: j for j in jobs}.values())
    jobs.sort(key=lambda j: "mnist" not in j[1])
    if not opt["table_only"]:
        pool(jobs, opt["jobs"])


def looks(opt, row, col, arm, seeds):
    return [parse_look(look_file(opt, row, col, arm, s)) for s in seeds]


def chain_table(opt):
    lines = []
    for group, (rnames, cnames, arms, seeds) in enumerate(CHAIN):
        seeds = [5] if opt["quick"] else seeds
        lines += ["", "The chain, seeds %s. late: the quick map's score; A on 0: share of "
                  "stage-0 steps with A pressed (B on 0, other on 0 the same for B and for any "
                  "other button); B on 1: share of stage-1 steps with B pressed; thresholds: "
                  "mean over A's and over B's motor cells at the end; motor, hidden at 64+: "
                  "share of those cells with a threshold of 64 or more; rate: spikes per 1,024 "
                  "cell-ticks, sensor cells left out." % ",".join(map(str, seeds)), "",
                  "| column | arm | late (lowest-highest) | A on 0 | B on 0 | other on 0 | B on 1 "
                  "| threshold A / B | motor, hidden at 64+ | rate | seeds |",
                  "|---|---|---|---|---|---|---|---|---|---|---|"]
        for col in cnames:
            for arm in arms:
                got = [p for p in looks(opt, "chain", col, arm, seeds) if p]
                if not got:
                    continue
                c = [chain_numbers(p) for p in got]
                late = [p["late"] for p in got]
                lines.append("| %s | %s | %s (%s-%s) | %s | %s | %s | %s | %s / %s | %s, %s | %s "
                             "| %d |" % (
                                 col, arm, fmt(mean(late)), fmt(min(late), 2), fmt(max(late), 2),
                                 fmt(mean([x["a0"] for x in c]), 2),
                                 fmt(mean([x["b0"] for x in c]), 2),
                                 fmt(mean([x["other0"] for x in c]), 2),
                                 fmt(mean([x["b1"] for x in c]), 2),
                                 fmt(mean([x["thrA"] for x in c]), 0),
                                 fmt(mean([x["thrB"] for x in c]), 0),
                                 fmt(mean([p.get("motor", {}).get("high") for p in got]), 2),
                                 fmt(mean([p.get("hidden", {}).get("high") for p in got]), 3),
                                 fmt(mean([p["rate"] for p in got]), 1), len(got)))
        if group == 0:
            lines += ["", "Per seed (5-12), late score:", ""]
            for col in cnames:
                for arm in arms:
                    got = looks(opt, "chain", col, arm, seeds)
                    if any(got):
                        lines.append("- %s %s: %s" % (col, arm, " ".join(
                            fmt(p["late"], 2) if p else "-" for p in got)))
    lines += ["", "Arms: " + "; ".join("%s: %s" % (a, ARMS[a][0]) for a in ARMS)]
    return "\n".join(lines)


def rate_table(opt):
    lines = []
    for rnames, cnames, arms, seeds in RATE:
        seeds = [5] if opt["quick"] else seeds
        lines += ["", "Seeds %s. late: the quick map's score; rate: spikes per 1,024 cell-ticks "
                  "over the last 2,000 steps, sensor cells left out, and apart for hidden and "
                  "motor cells; at 64+: share of thresholds at 64 or more; stuck: share of motor "
                  "cells firing on over a quarter of the ticks; bias at +max: share of sectors "
                  "with the bias at its limit." % ",".join(map(str, seeds)), "",
                  "| row | column | arm | late (lowest-highest) | rate | hidden rate | motor rate "
                  "| hidden at 64+ | motor at 64+ | motor stuck | bias at +max | seeds |",
                  "|---|---|---|---|---|---|---|---|---|---|---|---|"]
        for row in rnames:
            for col in cnames:
                for arm in opt.get("arms") or arms + EXTRA_RATE:
                    got = [p for p in looks(opt, row, col, arm, seeds) if p]
                    if not got:
                        continue
                    late = [p["late"] for p in got]
                    k = lambda kind, key: fmt(mean([p.get(kind, {}).get(key) for p in got]),  # noqa: E731
                                              1 if key == "rate" else 3)
                    lines.append("| %s | %s | %s | %s (%s-%s) | %s | %s | %s | %s | %s | %s | %s "
                                 "| %d |" % (
                                     row, col, arm, fmt(mean(late)), fmt(min(late), 2),
                                     fmt(max(late), 2), fmt(mean([p["rate"] for p in got]), 1),
                                     k("hidden", "rate"), k("motor", "rate"), k("hidden", "high"),
                                     k("motor", "high"), k("motor", "stuck"),
                                     fmt(mean([p.get("biastop") for p in got]), 2), len(got)))
    lines += ["", "Arms: " + "; ".join("%s: %s" % (a, ARMS[a][0]) for a in ARMS)]
    return "\n".join(lines)


# arms added to the rate part after its first table was read (run with --arms)
EXTRA_RATE = ["holdmotor", "holdmotor64", "c64t64", "c64t16", "c128", "c256"]


def main():
    argv = sys.argv[1:]
    if not argv or argv[0].startswith("-"):
        sys.stderr.write(open(os.path.join(ROOT, "scripts", "capdefault.sh")).read().split(
            "set -")[0])
        return 1
    opt = {"seeds": None, "jobs": 3, "quick": False, "table_only": False, "arms": None,
           "rows": None, "cols": None}
    what, i = argv[0], 1
    while i < len(argv):
        a = argv[i]
        if a in ("--quick", "--table-only"):
            opt[a[2:].replace("-", "_")] = True
        elif a in ("--seeds", "--jobs", "--arms", "--rows", "--cols"):
            v = argv[i + 1]
            opt[a[2:]] = seeds_of(v) if a == "--seeds" else int(v) if a == "--jobs" else v.split(",")
            i += 1
        else:
            print("capdefault: unknown option %s" % a)
            return 1
        i += 1
    opt["out"] = os.path.join(ROOT, "out", "capdefault" + ("-quick" if opt["quick"] else ""))
    os.makedirs(opt["out"], exist_ok=True)
    import capdefault_more as M
    parts = {"chain": lambda: (run_part(CHAIN, opt), chain_table(opt))[1],
             "rate": lambda: (run_part(rate_part(opt), opt), rate_table(opt))[1],
             "map": lambda: M.quick_map(opt), "inside": lambda: M.inside_test(opt),
             "speed": lambda: M.speed(opt)}
    names = list(parts) if what in ("smoke", "table", "all") else what.split(",")
    if what == "smoke":
        opt["quick"] = True
        opt["out"] = os.path.join(ROOT, "out", "capdefault-quick")
        os.makedirs(opt["out"], exist_ok=True)
    if what == "table":
        opt["table_only"] = True
    if any(n not in parts for n in names):
        print("capdefault: parts are %s (or smoke, table, all)" % ", ".join(parts))
        return 1
    for n in names:
        text = parts[n]()
        with open(os.path.join(opt["out"], "table-%s.md" % n), "w") as f:
            f.write(text + "\n")
        print("## %s\n%s\n" % (n, text), flush=True)
    return 0


def rate_part(opt):
    """the rate part, cut down by --arms, --rows, --cols when given"""
    rnames, cnames, arms, seeds = RATE[0]
    return [(opt["rows"] or rnames, opt["cols"] or cnames, opt["arms"] or arms, seeds)]


if __name__ == "__main__":
    sys.exit(main())
