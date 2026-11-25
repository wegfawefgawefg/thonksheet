#!/usr/bin/env python3
"""The rest of the task map (docs/task-map.md tiers 9 and 10, being shown the answer, damage),
scored three ways so we see where each kind of task is lost:

  (a) readable: blobprobe linear on the full engine as born (learning off), the sheet's
      spike counts over each scored step; beside it the same reader on the task's own input
      lines over the last 16 steps and on their brightness (the shortcut check)
  (b) the working readout rule on a frozen sheet (learn mode 5, evidence counters, balance,
      128 motor cells per button), beside its learning-off twin, coin flips and the scripted
      perfect player. no-teacher worlds have nothing for it to learn: there (b) is the full
      engine's own design rule (mode 0) and its window score, and (a') the reader after a
      whole life of that rule with no reward
  (c) the minimal engine and table neurons with their default rules, each with its twin

    scripts/map.py NAME [--quick] [--resume] [--print] [--levels a,b,c] [--only w1,w2]
                   [--jobs N] [flags for levels a and b ...]

Seeds 5 to 12 (--quick: 5 and 6, short lives). Runs go to out/map/NAME/, the table to
out/map/NAME/map.csv and the screen. --print runs nothing and prints what is there (after
copying runs made elsewhere into the folder). --levels picks levels; --only picks worlds by
name (scripts/map_worlds.py). Flags after the name go to every level (a) and (b) call.
"""

import os
import re
import statistics
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from map_worlds import PROTOCOL_KEYS, PROTOCOLS, WORLDS  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLOBRUN = os.path.join(ROOT, "build", "blobrun")
BLOBPROBE = os.path.join(ROOT, "build", "blobprobe")
BASE = ["--ticks", "20", "--sensor-side", "3", "--motor-side", "3"]
RULE = ["--g.learn_mode", "5", "--g.readout_evidence", "1", "--g.readout_shift", "4",
        "--g.readout_balance", "1", "--motor-spread", "128"]
FULL = {"steps": 40000, "window": 1000, "phase_window": 200, "retest": 10000,
        "trials": 3000, "settle": 30000, "seeds": list(range(5, 13))}
QUICK = {"steps": 4000, "window": 200, "phase_window": 200, "retest": 1000,
         "trials": 600, "settle": 5000, "seeds": [5, 6]}


def no_teacher(kind):
    return kind == "no teacher"


def jobs_for(life, levels, extra, only):
    """(cost, file stem, command)"""
    s, w = str(life["steps"]), str(life["window"])
    plain = ["--steps", s, "--window", w, "--alive-ticks", "0", "--rhythm-ticks", "0"]
    phased = ["--steps", s, "--window", str(life["phase_window"]), "--retest",
              str(life["retest"]), "--alive-ticks", "0", "--rhythm-ticks", "0"]
    probe = ["--trials", str(life["trials"]), "--settle", str(life["settle"])]
    out = []
    for name, kind, _, args in WORLDS + PROTOCOLS:
        if only and name not in only:
            continue
        protocol = kind in PROTOCOL_KEYS
        life_args = phased if protocol else plain
        for seed in life["seeds"]:
            sd = ["--seed", str(seed), "--task-seed", str(seed)]
            run = [BLOBRUN] + BASE + args + life_args + sd
            if "a" in levels and not protocol:
                out.append((0.6, "probe-%s-s%d" % (name, seed),
                            [BLOBPROBE, "linear"] + BASE + ["--motor-spread", "128"] + args +
                            probe + sd + ["--label", name] + extra))
                if no_teacher(kind):
                    # the reader after a whole unrewarded life of the design rule
                    settle = str(life["steps"] * 20)
                    out.append((1.0, "learned-%s-s%d" % (name, seed),
                                [BLOBPROBE, "linear"] + BASE + ["--motor-spread", "128"] +
                                args + probe[:2] + ["--settle", settle, "--learning", "1"] +
                                sd + ["--label", name] + extra))
            if "b" in levels:
                rule = [] if no_teacher(kind) else RULE
                out.append((1.0, "rule-%s-s%d" % (name, seed), run + rule + extra))
            if "c" in levels:
                out.append((0.2, "minimal-%s-s%d" % (name, seed), run + ["--engine", "minimal"]))
                out.append((1.0, "table-%s-s%d" % (name, seed), run + ["--table"]))
    return out


def run_one(stem, cmd, outdir):
    if cmd[0] == BLOBRUN:
        cmd = cmd + ["--out", os.path.join(outdir, stem + ".csv")]
    with open(os.path.join(outdir, stem + ".txt"), "w") as f:
        f.write(" ".join(cmd) + "\n")
        f.flush()
        return subprocess.run(["nice", "-n", "19"] + cmd, stdout=f,
                              stderr=subprocess.STDOUT).returncode


def read_sum(path):
    out = {}
    if os.path.exists(path):
        with open(path) as f:
            next(f, None)
            for line in f:
                parts = line.strip().split(",")
                if len(parts) == 3:
                    out.setdefault(parts[0], {})[parts[1]] = float(parts[2])
    return out


PROBE_WHOLE = re.compile(r"whole sheet ([0-9.]+)\s+motors' view ([0-9.-]+)")
PROBE_LINES = re.compile(r"last \d+ steps ([0-9.]+)\s+brightness, last \d+ steps ([0-9.]+)")


def read_probe(path):
    """whole sheet, motors' view, input lines, brightness (None where missing)"""
    if not os.path.exists(path):
        return None
    text = open(path).read()
    m, l = PROBE_WHOLE.search(text), PROBE_LINES.search(text)
    if not m:
        return None
    view = float(m.group(2)) if m.group(2) != "-" else None
    return (float(m.group(1)), view, float(l.group(1)) if l else None,
            float(l.group(2)) if l else None)


def avg(values):
    values = [v for v in values if v is not None]
    return statistics.mean(values) if values else None


def sd(values):
    values = [v for v in values if v is not None]
    return statistics.stdev(values) if len(values) > 1 else None


def collect(outdir, seeds, only):
    rows = []
    for name, kind, what, _ in WORLDS + PROTOCOLS:
        if only and name not in only:
            continue
        key = PROTOCOL_KEYS[kind][-1] if kind in PROTOCOL_KEYS else "late"
        r = dict(name=name, kind=kind, what=what, key=key)
        cols = {}
        for seed in seeds:
            rule = read_sum(os.path.join(outdir, "rule-%s-s%d-sum.csv" % (name, seed)))
            mini = read_sum(os.path.join(outdir, "minimal-%s-s%d-sum.csv" % (name, seed)))
            table = read_sum(os.path.join(outdir, "table-%s-s%d-sum.csv" % (name, seed)))
            probe = read_probe(os.path.join(outdir, "probe-%s-s%d.txt" % (name, seed)))
            learned = read_probe(os.path.join(outdir, "learned-%s-s%d.txt" % (name, seed)))
            pairs = [("coin", rule, "coin"), ("perfect", rule, "perfect"),
                     ("rule", rule, "blob"), ("rule_off", rule, "learn_off"),
                     ("minimal", mini, "blob"), ("minimal_off", mini, "learn_off"),
                     ("table", table, "table"), ("table_off", table, "table_off")]
            for col, sums, driver in pairs:
                cols.setdefault(col, []).append(sums.get(driver, {}).get(key))
            if kind in PROTOCOL_KEYS:
                for k in PROTOCOL_KEYS[kind]:
                    for col, sums, driver in pairs[2:]:
                        cols.setdefault(col + ":" + k, []).append(sums.get(driver, {}).get(k))
            for i, col in enumerate(("sheet", "view", "lines", "bright")):
                cols.setdefault(col, []).append(probe[i] if probe else None)
            cols.setdefault("learned", []).append(learned[0] if learned else None)
        for col, vals in cols.items():
            r[col] = avg(vals)
            r[col + "_sd"] = sd(vals)
            r[col + "_n"] = len([v for v in vals if v is not None])
        # the minimal engine keeps no predictions of its input: its no-teacher scores mean
        # nothing (prediction share reads 1.0, fill and surprise lift 0)
        if no_teacher(kind):
            r["minimal"] = r["minimal_off"] = None
        rows.append(r)
    return rows


def f(v, width=6):
    return ("%*.2f" % (width, v)) if v is not None else "%*s" % (width, "-")


def print_table(rows, out_csv):
    head = ("%-17s %5s %5s | %5s %5s %5s %5s %5s | %5s %5s %4s | %5s %5s | %5s %5s" %
            ("world", "coin", "perf", "sheet", "view", "lines", "brite", "learn",
             "rule", "twin", "sd", "minim", "twin", "table", "twin"))
    print("(a) linear reader, as born: sheet, motors' view, input lines (16 steps), brightness;")
    print("    learn: the reader after an unrewarded life of the design rule (no teacher only)")
    print("(b) readout rule on a frozen sheet (no teacher: the design rule's own score), twin")
    print("(c) minimal engine and table neurons, default rules, each with its twin")
    print(head)
    kind = None
    lines = ["world,kind,what,key,coin,perfect,sheet,view,lines,brightness,learned,rule,"
             "rule_sd,rule_off,minimal,minimal_off,table,table_off,seeds"]
    for r in rows:
        if r["kind"] != kind:
            kind = r["kind"]
            print("-- " + kind)
        print("%-17s %s %s | %s %s %s %s %s | %s %s %s | %s %s | %s %s" % (
            r["name"][:17], f(r.get("coin"), 5), f(r.get("perfect"), 5), f(r.get("sheet"), 5),
            f(r.get("view"), 5), f(r.get("lines"), 5), f(r.get("bright"), 5),
            f(r.get("learned"), 5), f(r.get("rule"), 5), f(r.get("rule_off"), 5),
            f(r.get("rule_sd"), 4), f(r.get("minimal"), 5), f(r.get("minimal_off"), 5),
            f(r.get("table"), 5), f(r.get("table_off"), 5)))
        if r["kind"] in PROTOCOL_KEYS:
            for k in PROTOCOL_KEYS[r["kind"]][:-1]:
                print("  %-15s%s%s %s %s | %s %s | %s %s" % (
                    k, " " * 47, f(r.get("rule:" + k), 5), f(r.get("rule_off:" + k), 5),
                    " " * 4, f(r.get("minimal:" + k), 5), f(r.get("minimal_off:" + k), 5),
                    f(r.get("table:" + k), 5), f(r.get("table_off:" + k), 5)))
        cells = [r["name"], r["kind"], '"%s"' % r["what"], r["key"]]
        for col in ("coin", "perfect", "sheet", "view", "lines", "bright", "learned", "rule",
                    "rule_sd", "rule_off", "minimal", "minimal_off", "table", "table_off"):
            v = r.get(col)
            cells.append("" if v is None else "%.4f" % v)
        cells.append(str(r.get("rule_n", 0)))
        lines.append(",".join(cells))
        if r["kind"] in PROTOCOL_KEYS:
            for k in PROTOCOL_KEYS[r["kind"]][:-1]:
                sub = [r["name"] + ":" + k, r["kind"], '"%s"' % r["what"], k, "", "", "", "",
                       "", "", ""]
                for col in ("rule", "rule_sd", "rule_off", "minimal", "minimal_off", "table",
                            "table_off"):
                    v = r.get(col + ":" + k) if not col.endswith("_sd") else None
                    sub.append("" if v is None else "%.4f" % v)
                sub.append(str(r.get("rule_n", 0)))
                lines.append(",".join(sub))
    with open(out_csv, "w") as fh:
        fh.write("\n".join(lines) + "\n")


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 1
    name = args[0]
    rest = args[1:]
    quick = "--quick" in rest
    resume = "--resume" in rest
    print_only = "--print" in rest
    levels, only, jobs_at_once = "abc", None, 2
    extra = []
    i = 0
    while i < len(rest):
        a = rest[i]
        if a in ("--quick", "--resume", "--print"):
            pass
        elif a == "--levels":
            levels = rest[i + 1].replace(",", "")
            i += 1
        elif a == "--only":
            only = set(rest[i + 1].split(","))
            i += 1
        elif a == "--jobs":
            jobs_at_once = int(rest[i + 1])
            i += 1
        else:
            extra.append(a)
        i += 1
    life = QUICK if quick else FULL
    card = name + ("-quick" if quick else "")
    outdir = os.path.join(ROOT, "out", "map", card)
    os.makedirs(outdir, exist_ok=True)
    started = time.time()
    failed = []
    if not print_only:
        todo = sorted(jobs_for(life, levels, extra, only), key=lambda j: -j[0])
        if resume:
            todo = [j for j in todo if not os.path.exists(os.path.join(
                outdir, j[1] + ("-sum.csv" if j[2][0] == BLOBRUN else ".txt")))]
        print("map %s: %d runs, %d at a time, seeds %s, levels %s" % (
            card, len(todo), jobs_at_once, life["seeds"], levels), flush=True)
        done = 0
        with ThreadPoolExecutor(max_workers=jobs_at_once) as pool:
            for (cost, stem, cmd), code in zip(todo, pool.map(
                    lambda j: run_one(j[1], j[2], outdir), todo)):
                done += 1
                if code != 0:
                    failed.append(stem)
                if done % 20 == 0 or done == len(todo):
                    print("  %d of %d runs done, %.0f min" % (
                        done, len(todo), (time.time() - started) / 60), flush=True)
    rows = collect(outdir, life["seeds"], only)
    print_table(rows, os.path.join(outdir, "map.csv"))
    print("table: %s (%.0f min)" % (os.path.relpath(os.path.join(outdir, "map.csv"), ROOT),
                                    (time.time() - started) / 60))
    if failed:
        print("FAILED runs (see their .txt in %s): %s" % (os.path.relpath(outdir, ROOT),
                                                         " ".join(failed)))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
