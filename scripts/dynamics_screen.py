#!/usr/bin/env python3
"""Screen sheet mechanisms with no learning: how readable each feature is, and is it alive.

For every variant in a variants file (`name: --g.field value ...`, first line the baseline)
and every seed: the feature probe on the sheet as born (learning off, no reward) for each
feature, and a short learning-off life for the aliveness line (rate against target, silent
and stuck neurons). Rows go to <out>/screen.csv; a rerun only runs what is missing.

    scripts/dynamics_screen.py run --jobs 2              measure (seeds 1-4)
    scripts/dynamics_screen.py table                     change against the baseline

The score is how readable a feature is from the tile next to the input (1.0 easy, under 0.3
lost). The table shows each variant's mean over seeds less the baseline's, seed by seed,
with its standard error; * marks a change of 2 standard errors or more.
"""

import argparse
import concurrent.futures
import csv
import os
import statistics
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FEATURES = ["which", "order", "xor", "gap", "hold20", "hold60", "hold150"]


def read_variants(path):
    out = []
    for line in open(path):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, _, args = line.partition(":")
        out.append((name.strip(), args.split()))
    return out


def probe(args, feature, seed, tries, settle):
    extra = []
    name = feature
    if feature.startswith("hold"):
        extra = ["--hold-gap", feature[4:]]
        name = "hold"
    cmd = ["nice", "-n", "19", os.path.join(ROOT, "build", "blobprobe"), "feature",
           "--feature", name, *extra, "--seed", str(seed), "--tries", str(tries),
           "--settle", str(settle), "--label", "x", "--g.learning_on", "0", *args]
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    for line in out.splitlines():
        parts = line.split()
        if parts and parts[0] == name:
            return [float(x) for x in parts[2:5]]
    return None


def alive(args, seed, out_dir):
    runs = os.path.join(out_dir, "runs")
    os.makedirs(runs, exist_ok=True)
    tag = str(abs(hash(" ".join(args) + str(seed))))
    cmd = ["nice", "-n", "19", os.path.join(ROOT, "build", "blobrun"), "--task", "echo",
           "--steps", "3000", "--seed", str(seed), "--no-baselines",
           "--out", os.path.join(runs, tag + ".csv"),
           "--g.learning_on", "0", *args]
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    for line in out.splitlines():
        if " alive: " in line:
            w = line.split()
            # rate, target, silent, stuck, rhythm strength
            return [float(w[2]), float(w[9].rstrip("),")), float(w[11].rstrip(",")),
                    float(w[13].rstrip(",")), float(w[18])]
    return None


def load(path):
    rows = {}
    if os.path.exists(path):
        for r in csv.DictReader(open(path)):
            if r["a"] == "":
                continue  # failed: run again
            rows[(r["variant"], int(r["seed"]), r["feature"])] = r
    return rows


def cmd_run(opt):
    variants = read_variants(opt.variants)
    path = os.path.join(opt.out, "screen.csv")
    os.makedirs(opt.out, exist_ok=True)
    have = load(path)
    seeds = [int(s) for s in opt.seeds.split(",")]
    jobs = []
    for name, args in variants:
        for seed in seeds:
            for f in FEATURES + ["alive"]:
                if (name, seed, f) not in have:
                    jobs.append((name, args, seed, f))
    print(f"{len(jobs)} runs to do, {opt.jobs} at a time", flush=True)

    def work(job):
        name, args, seed, f = job
        if f == "alive":
            v = alive(args, seed, opt.out)
        else:
            v = probe(args, f, seed, opt.tries, opt.settle)
        return name, seed, f, v

    new = not os.path.exists(path)
    with open(path, "a", newline="") as fh:
        w = csv.writer(fh)
        if new:
            w.writerow(["variant", "seed", "feature", "a", "b", "c", "d", "e"])
        with concurrent.futures.ThreadPoolExecutor(max_workers=opt.jobs) as pool:
            for n, (name, seed, f, v) in enumerate(pool.map(work, jobs)):
                v = (v or []) + [""] * (5 - len(v or []))
                w.writerow([name, seed, f] + v)
                fh.flush()
                if (n + 1) % 50 == 0:
                    print(f"  {n + 1}/{len(jobs)}", flush=True)


def cmd_table(opt):
    variants = read_variants(opt.variants)
    rows = load(os.path.join(opt.out, "screen.csv"))
    seeds = [int(s) for s in opt.seeds.split(",")]
    base = variants[0][0]
    col = opt.distance  # a, b, c: tiles 0, 1, 2 from the cue

    def values(name, f, field):
        out = []
        for s in seeds:
            r = rows.get((name, s, f))
            out.append(float(r[field]) if r and r[field] != "" else None)
        return out

    field = "abc"[col]
    print(f"Readable from {col} tile(s) from the cue, mean over seeds {opt.seeds}; then each "
          f"variant less {base}, seed by seed, +- standard error; * is 2 SE or more.\n")
    print("| variant | " + " | ".join(FEATURES) + " | rate | silent | stuck |")
    print("|---" * (len(FEATURES) + 4) + "|")
    for name, _ in variants:
        cells = []
        for f in FEATURES:
            v = values(name, f, field)
            b = values(base, f, field)
            if None in v:
                cells.append("-")
                continue
            if name == base:
                sd = statistics.stdev(v) if len(v) > 1 else 0
                cells.append(f"{statistics.mean(v):.2f} ±{sd:.2f}")
                continue
            d = [x - y for x, y in zip(v, b)]
            m = statistics.mean(d)
            se = statistics.stdev(d) / len(d) ** 0.5 if len(d) > 1 else 0
            star = "*" if se > 0 and abs(m) >= 2 * se else ""
            cells.append(f"{m:+.2f} ±{se:.2f}{star}")
        al = [rows.get((name, s, "alive")) for s in seeds]
        if None in al or any(r["a"] == "" for r in al):
            cells += ["-", "-", "-"]
        else:
            cells.append(f"{statistics.mean(float(r['a']) for r in al):.1f}")
            cells.append(f"{statistics.mean(float(r['c']) for r in al):.3f}")
            cells.append(f"{statistics.mean(float(r['d']) for r in al):.3f}")
        print(f"| {name} | " + " | ".join(cells) + " |")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("cmd", choices=["run", "table"])
    p.add_argument("--variants", default=os.path.join(ROOT, "scripts", "variants",
                                                      "dynamics-screen.txt"))
    p.add_argument("--out", default=os.path.join(ROOT, "out", "dynamics"))
    p.add_argument("--seeds", default="1,2,3,4")
    p.add_argument("--jobs", type=int, default=2)
    p.add_argument("--tries", type=int, default=2000)
    p.add_argument("--settle", type=int, default=5000)
    p.add_argument("--distance", type=int, default=1)
    opt = p.parse_args()
    {"run": cmd_run, "table": cmd_table}[opt.cmd](opt)


if __name__ == "__main__":
    sys.exit(main())
