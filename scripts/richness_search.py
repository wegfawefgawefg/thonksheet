#!/usr/bin/env python3
"""Search for what makes a sheet rich to read.

Learning has turned out to be mostly selection at the output, so a sheet can be judged with
no learning rule at all: is the feature readable from its neurons? This draws random sheets
(random values for about twenty genome numbers), runs the feature probe on each as born
(learning off, no reward), and writes one row per sheet. Then it says which numbers move
which feature.

    scripts/richness_search.py run --samples 300 --jobs 2      draw and measure (adds to the csv)
    scripts/richness_search.py report                           which numbers matter, best sheets
    scripts/richness_search.py check --top 10 --seeds 2,3       re-measure the best on new seeds

Each probe is `nice -n 19 build/blobprobe feature`, single-threaded. Rows go to
out/richness/samples.csv. The score for a feature is how readable it is from the tile next
to the input (1.0 easy, under 0.3 lost).
"""

import argparse
import concurrent.futures
import csv
import os
import random
import statistics
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "out", "richness")
CSV = os.path.join(OUT, "samples.csv")
FEATURES = ["which", "order", "xor", "gap"]

# name: (low, high). Pairs that must stay ordered are fixed up in draw().
RANGES = {
    "inhibitory_percent": (10, 40),
    "inhibit_gain": (2, 8),
    "threshold_min": (2, 8),
    "threshold_max": (8, 20),
    "leak_shift_min": (1, 2),
    "leak_shift_max": (2, 7),
    "cooldown_min": (1, 2),
    "cooldown_max": (2, 8),
    "local_connect_shift": (1, 4),
    "strong_shift": (1, 3),
    "far_percent": (0, 60),
    "far_reach_tiles": (2, 6),
    "local_delay_min": (1, 4),
    "local_delay_max": (1, 12),
    "branch_threshold": (1, 6),
    "branch_bonus": (0, 6),
    "noise_shift": (8, 13),
    "sector_target_per_1024": (16, 96),
    "sector_smooth_shift": (2, 7),
    "pacemakers_per_1024": (0, 16),
    "strong_log": (1, 2),
    "fan_in_max": (48, 128),
    # wiring at birth (docs/results-2026-11-25-family-wiring.md). wire_slots 9 is all of
    # them; wire_two_sheets at 0 or below is off, so about half the sheets have it
    "wire_layers": (0, 4),
    "wire_fine": (0, 6),
    "wire_slots": (3, 9),
    "wire_mirror_percent": (0, 100),
    "wire_hub_per_1024": (0, 60),
    "wire_hub_shift": (1, 3),
    "wire_stream": (0, 4),
    "wire_stream_dir": (0, 15),
    "wire_stream_turn": (0, 16),
    "wire_far_topo": (0, 2),
    "wire_far_jitter": (0, 2),
    "wire_inhib_reach": (-3, 3),
    "wire_two_sheets": (-100, 100),
}
DEFAULTS = {
    "inhibitory_percent": 25, "inhibit_gain": 5, "threshold_min": 4, "threshold_max": 12,
    "leak_shift_min": 1, "leak_shift_max": 4, "cooldown_min": 1, "cooldown_max": 4,
    "local_connect_shift": 3, "strong_shift": 2, "far_percent": 15, "far_reach_tiles": 3,
    "local_delay_min": 1, "local_delay_max": 4, "branch_threshold": 3, "branch_bonus": 2,
    "noise_shift": 10, "sector_target_per_1024": 32, "sector_smooth_shift": 4,
    "pacemakers_per_1024": 4, "strong_log": 2, "fan_in_max": 96,
    "wire_layers": 0, "wire_fine": 0, "wire_slots": 0, "wire_mirror_percent": 0,
    "wire_hub_per_1024": 0, "wire_hub_shift": 2, "wire_stream": 0, "wire_stream_dir": 0,
    "wire_stream_turn": 0, "wire_far_topo": 0, "wire_far_jitter": 0, "wire_inhib_reach": 0,
    "wire_two_sheets": 0,
}


def draw(rng):
    g = {k: rng.randint(lo, hi) for k, (lo, hi) in RANGES.items()}
    for lo, hi in (("threshold_min", "threshold_max"), ("leak_shift_min", "leak_shift_max"),
                   ("cooldown_min", "cooldown_max"), ("local_delay_min", "local_delay_max")):
        if g[hi] < g[lo]:
            g[hi] = g[lo]
    return g


def probe(genome, feature, seed, tries):
    # "hold60" is the hold feature read 60 quiet ticks after the cue
    extra = []
    if feature.startswith("hold") and feature != "hold":
        extra = ["--hold-gap", feature[4:]]
        feature = "hold"
    cmd = ["nice", "-n", "19", os.path.join(ROOT, "build", "blobprobe"), "feature",
           "--feature", feature, *extra, "--seed", str(seed), "--tries", str(tries),
           "--settle", "5000", "--label", "x", "--g.learning_on", "0"]
    for k, v in genome.items():
        cmd += ["--g." + k, str(v)]
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, timeout=300).stdout
    except subprocess.TimeoutExpired:
        return None
    for line in out.splitlines():
        parts = line.split()
        if parts and parts[0] == feature:
            return [float(x) for x in parts[2:]]
    return None


def measure(genome, seed, tries):
    row = dict(genome)
    row["seed"] = seed
    for f in FEATURES:
        values = probe(genome, f, seed, tries)
        if values is None or len(values) < 3:
            values = [0.0, 0.0, 0.0]
        row[f + "_1"] = values[1]
        row[f + "_2"] = values[2]
    return row


def columns():
    cols = list(RANGES) + ["seed"]
    for f in FEATURES:
        cols += [f + "_1", f + "_2"]
    return cols


def append_rows(rows, path=None):
    path = path or CSV
    os.makedirs(OUT, exist_ok=True)
    new = not os.path.exists(path)
    with open(path, "a", newline="") as f:
        w = csv.DictWriter(f, fieldnames=columns())
        if new:
            w.writeheader()
        for r in rows:
            w.writerow(r)


def load(path=None):
    path = path or CSV
    with open(path) as f:
        return [{k: float(v) for k, v in r.items()} for r in csv.DictReader(f)]


def cmd_run(args):
    rng = random.Random(args.draw_seed)
    genomes = [dict(DEFAULTS)] + [draw(rng) for _ in range(args.samples)]
    done = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(measure, g, args.seed, args.tries) for g in genomes]
        for fut in concurrent.futures.as_completed(futures):
            append_rows([fut.result()])
            done += 1
            if done % 20 == 0:
                print(f"{done} of {len(genomes)} sheets measured", flush=True)
    print(f"done: {len(genomes)} sheets in {CSV}")


def ranks(values):
    order = sorted(range(len(values)), key=lambda i: values[i])
    out = [0.0] * len(values)
    for rank, i in enumerate(order):
        out[i] = rank
    return out


def rank_correlation(a, b):
    ra, rb = ranks(a), ranks(b)
    ma, mb = statistics.mean(ra), statistics.mean(rb)
    num = sum((x - ma) * (y - mb) for x, y in zip(ra, rb))
    den = (sum((x - ma) ** 2 for x in ra) * sum((y - mb) ** 2 for y in rb)) ** 0.5
    return num / den if den else 0.0


def cmd_report(args):
    rows = load()
    print(f"{len(rows)} sheets. Score: readable from the tile next to the input.\n")
    print("How each number moves each feature (rank correlation; |0.15| or more shown):")
    print(f"{'number':24s}" + "".join(f"{f:>8s}" for f in FEATURES))
    for name in RANGES:
        xs = [r[name] for r in rows]
        cells = []
        for f in FEATURES:
            c = rank_correlation(xs, [r[f + "_1"] for r in rows])
            cells.append(f"{c:+8.2f}" if abs(c) >= 0.15 else f"{'.':>8s}")
        print(f"{name:24s}" + "".join(cells))
    print("\nSpread over all sheets (lowest, median, highest), and the default sheet:")
    default = rows[0]
    for f in FEATURES:
        v = sorted(r[f + "_1"] for r in rows)
        print(f"  {f:6s} {v[0]:6.2f} {v[len(v) // 2]:6.2f} {v[-1]:6.2f}   default {default[f + '_1']:.2f}")
    # the best all-rounders: mean of each feature's rank
    all_ranks = {f: ranks([r[f + "_1"] for r in rows]) for f in FEATURES}
    score = [sum(all_ranks[f][i] for f in FEATURES) for i in range(len(rows))]
    best = sorted(range(len(rows)), key=lambda i: -score[i])[:args.top]
    print(f"\nThe {args.top} best all-round sheets:")
    for i in best:
        r = rows[i]
        feats = "  ".join(f"{f} {r[f + '_1']:.2f}" for f in FEATURES)
        flags = " ".join(f"--g.{k} {int(r[k])}" for k in RANGES if int(r[k]) != DEFAULTS[k])
        print(f"  {feats}\n      {flags}")


def cmd_check(args):
    rows = load()
    all_ranks = {f: ranks([r[f + "_1"] for r in rows]) for f in FEATURES}
    score = [sum(all_ranks[f][i] for f in FEATURES) for i in range(len(rows))]
    best = [0] + sorted(range(len(rows)), key=lambda i: -score[i])[:args.top]
    seeds = [int(s) for s in args.seeds.split(",")]
    path = os.path.join(OUT, os.path.basename(CSV)[:-4] + "-check.csv")
    jobs = [({k: int(rows[i][k]) for k in RANGES}, s) for i in best for s in seeds]
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(lambda j: measure(j[0], j[1], args.tries), jobs))
    append_rows(results, path)
    print(f"{'sheet':8s}" + "".join(f"{f:>8s}" for f in FEATURES) + "   (mean over seeds " + args.seeds + ")")
    for n, i in enumerate(best):
        mine = results[n * len(seeds):(n + 1) * len(seeds)]
        name = "default" if n == 0 else f"best {n}"
        print(f"{name:8s}" + "".join(f"{statistics.mean(r[f + '_1'] for r in mine):8.2f}" for f in FEATURES))


def main():
    global FEATURES, CSV
    p = argparse.ArgumentParser()
    p.add_argument("--name", default="samples", help="which search: out/richness/<name>.csv")
    p.add_argument("--features", default=",".join(FEATURES),
                   help="comma list: which, order, xor, gap, holdN (N quiet ticks)")
    sub = p.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run")
    r.add_argument("--samples", type=int, default=300)
    r.add_argument("--jobs", type=int, default=2)
    r.add_argument("--seed", type=int, default=1)
    r.add_argument("--tries", type=int, default=1500)
    r.add_argument("--draw-seed", type=int, default=1)
    r.set_defaults(fn=cmd_run)
    q = sub.add_parser("report")
    q.add_argument("--top", type=int, default=8)
    q.set_defaults(fn=cmd_report)
    c = sub.add_parser("check")
    c.add_argument("--top", type=int, default=10)
    c.add_argument("--seeds", default="2,3")
    c.add_argument("--jobs", type=int, default=2)
    c.add_argument("--tries", type=int, default=1500)
    c.set_defaults(fn=cmd_check)
    args = p.parse_args()
    FEATURES = args.features.split(",")
    CSV = os.path.join(OUT, args.name + ".csv")
    args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
