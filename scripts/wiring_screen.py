#!/usr/bin/env python3
"""Screen wiring variants by how readable the sheet is, with no learning rule.

For each variant (a line `name: --g.field value ...` in a variants file; the first line is
the baseline) and each seed: the feature probe (which, order, xor, gap, hold 20 and 60
ticks) read from the tile next to the input, the carry probe (readable at 3 tiles, and
reach), and `blobprobe wiring` (rate against target, silent, stuck). Sheet as born,
learning off; `--life N` instead turns learning on (the default inner rules, no reward)
and measures after N ticks of the probe's own input.

    scripts/wiring_screen.py --variants scripts/variants/wiring-screen.txt --seeds 1-4 --jobs 2
    scripts/wiring_screen.py --variants F --life 600000 --name life

Runs are cached in out/wiring/<name>-cache.jsonl, keyed by the command, so a rerun only
runs what is missing. `--table-only` prints from the cache.
"""

import argparse
import concurrent.futures
import json
import os
import statistics
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROBE = os.path.join(ROOT, "build", "blobprobe")
FEATURES = ["which", "order", "xor", "gap", "hold20", "hold60"]
MEASURES = [f + "1" for f in FEATURES] + ["carry3", "reach", "rate", "silent", "stuck"]


def parse_seeds(text):
    seeds = []
    for part in text.split(","):
        if "-" in part:
            lo, hi = part.split("-")
            seeds += list(range(int(lo), int(hi) + 1))
        else:
            seeds.append(int(part))
    return seeds


def read_variants(path):
    out = []
    for line in open(path):
        line = line.strip()
        if line and not line.startswith("#"):
            name, _, args = line.partition(":")
            out.append((name.strip(), args.split()))
    return out


def commands(args, seed, opt):
    """(measure group, command) for one variant and seed."""
    base = ["--seed", str(seed)] + args
    if opt.life:
        learn = ["--settle", str(opt.life)]
        age = ["--age", str(opt.life)]
    else:
        learn = ["--settle", str(opt.settle), "--g.learning_on", "0"]
        age = ["--g.learning_on", "0"]
    cmds = []
    for f in FEATURES:
        extra = ["--feature", f]
        if f.startswith("hold"):
            extra = ["--feature", "hold", "--hold-gap", f[4:]]
        cmds.append((f, ["feature"] + extra + ["--tries", str(opt.tries)] + learn + base))
    carry_settle = ["--settle", str(opt.life)] if opt.life else ["--g.learning_on", "0"]
    cmds.append(("carry", ["carry"] + carry_settle + base))
    cmds.append(("wiring", ["wiring", "--ticks", "20000"] + age + base))
    return cmds


def parse(group, out):
    lines = [l for l in out.splitlines() if l.strip()]
    if group == "carry":
        parts = lines[-1].split()
        nums = [float(x) for x in parts[1:-2]]
        return {"carry3": nums[3], "reach": float(parts[-1])}
    if group == "wiring":
        stats = lines[0].split()
        alive = lines[1].split()
        row = {"rate": float(alive[2]), "silent": float(alive[-3].rstrip(",")),
               "stuck": float(alive[-1])}
        keys = ["fan", "own", "slots", "dist", "recip", "flow", "upx", "upy", "ownI", "ownE",
                "fartiles", "cross", "fwd", "back"]
        for k in keys:
            if k in stats:
                v = stats[stats.index(k) + 1]
                row["w_" + k] = float(v.split("+")[0]) if k != "upx" and k != "upy" else float(v)
        return row
    parts = lines[0].split()
    nums = [float(x) for x in parts[2:]]
    return {group + "1": nums[1], group + "2": nums[2]}


def run(cmd):
    full = ["nice", "-n", "19", PROBE] + cmd + ["--label", "x"]
    done = subprocess.run(full, capture_output=True, text=True)
    if done.returncode != 0:
        raise RuntimeError(" ".join(cmd) + "\n" + done.stderr)
    return done.stdout


def stats(values):
    m = statistics.mean(values)
    sd = statistics.stdev(values) if len(values) > 1 else 0.0
    return m, sd


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--variants", required=True)
    ap.add_argument("--seeds", default="1-4")
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--tries", type=int, default=3000)
    ap.add_argument("--settle", type=int, default=10000)
    ap.add_argument("--life", type=int, default=0)
    ap.add_argument("--name", default="screen")
    ap.add_argument("--table-only", action="store_true")
    ap.add_argument("--only", default="", help="comma list of variant names to run")
    opt = ap.parse_args()
    seeds = parse_seeds(opt.seeds)
    variants = read_variants(opt.variants)
    out_dir = os.path.join(ROOT, "out", "wiring")
    os.makedirs(out_dir, exist_ok=True)
    cache_path = os.path.join(out_dir, opt.name + "-cache.jsonl")
    cache = {}
    if os.path.exists(cache_path):
        for line in open(cache_path):
            r = json.loads(line)
            cache[r["key"]] = r["row"]

    only = set(opt.only.split(",")) if opt.only else None
    todo = []
    for name, args in variants:
        if only and name not in only and name != variants[0][0]:
            continue
        for s in seeds:
            for group, cmd in commands(args, s, opt):
                key = " ".join(cmd)
                if key not in cache and key not in {" ".join(c) for _, c in todo}:
                    todo.append((group, cmd))
    if todo and not opt.table_only:
        print(f"{len(todo)} probes to run, {opt.jobs} at a time", file=sys.stderr)
        with concurrent.futures.ThreadPoolExecutor(max_workers=opt.jobs) as pool:
            futs = {pool.submit(run, cmd): (group, cmd) for group, cmd in todo}
            for n, fut in enumerate(concurrent.futures.as_completed(futs)):
                group, cmd = futs[fut]
                row = parse(group, fut.result())
                key = " ".join(cmd)
                cache[key] = row
                with open(cache_path, "a") as f:
                    f.write(json.dumps({"key": key, "row": row}) + "\n")
                if (n + 1) % 50 == 0:
                    print(f"  {n + 1}/{len(todo)}", file=sys.stderr, flush=True)

    # one row of measures per variant and seed
    table = {}
    for name, args in variants:
        rows = []
        for s in seeds:
            row = {}
            for group, cmd in commands(args, s, opt):
                got = cache.get(" ".join(cmd))
                if got is None:
                    row = None
                    break
                row.update(got)
            if row is None:
                break
            rows.append(row)
        if len(rows) == len(seeds):
            table[name] = rows
    base_name = variants[0][0]
    if base_name not in table:
        print("baseline missing")
        return
    base = table[base_name]
    life = f", after {opt.life} ticks learning on" if opt.life else ", as born (learning off)"
    print(f"## {opt.name}: seeds {opt.seeds}{life}\n")
    print("Mean over seeds (sd). Features read from the tile next to the input; carry3 is the "
          "cue read 3 tiles away.\n")
    print("| variant | " + " | ".join(MEASURES) + " |")
    print("|---" * (len(MEASURES) + 1) + "|")
    for name, _ in variants:
        if name not in table:
            continue
        cells = []
        for m in MEASURES:
            mean, sd = stats([r[m] for r in table[name]])
            cells.append(f"{mean:.3f}" if m in ("silent", "stuck") else f"{mean:.2f} ({sd:.2f})")
        print(f"| {name} | " + " | ".join(cells) + " |")
    print("\nChange against the baseline, seed by seed: difference (t). |t| under 2 is noise; "
          "** marks |t| >= 2 with a change of at least 10%.\n")
    print("| variant | " + " | ".join(MEASURES[:8]) + " |")
    print("|---" * 9 + "|")
    for name, _ in variants[1:]:
        if name not in table:
            continue
        cells = []
        for m in MEASURES[:8]:
            diffs = [a[m] - b[m] for a, b in zip(table[name], base)]
            d = statistics.mean(diffs)
            se = statistics.stdev(diffs) / len(diffs) ** 0.5 if len(diffs) > 1 else 0.0
            t = d / se if se > 0 else 0.0
            bm = statistics.mean(b[m] for b in base)
            mark = "**" if abs(t) >= 2 and bm and abs(d) >= 0.1 * abs(bm) else ""
            cells.append(f"{mark}{d:+.2f} ({t:+.1f}){mark}")
        print(f"| {name} | " + " | ".join(cells) + " |")
    print()
    wkeys = ["w_own", "w_recip", "w_flow", "w_upx", "w_fartiles", "w_cross"]
    print("Wiring (seed mean): " + ", ".join(k[2:] for k in wkeys))
    for name, _ in variants:
        if name in table:
            vals = [statistics.mean(r.get(k, 0.0) for r in table[name]) for k in wkeys]
            print(f"  {name:22s} " + " ".join(f"{v:8.3f}" for v in vals))


if __name__ == "__main__":
    main()
