#!/usr/bin/env python3
"""Run one task over N seeds for a list of named genome variants and print one table.

    scripts/sweep.py --name echo2 --seeds 11-18 --variants scripts/variants/ablate.txt \
        -- --task echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000

A variants file has one variant per line, `name: --g.field value ...`; the first line is the
baseline every other row is compared with, seed by seed. Lines starting with # are skipped.

Each run is `nice -n 19 build/blobrun ... --no-baselines`, single-threaded, at most --jobs
(4) at once. The learning-off twin is the same command with --g.learning_on 0; variants that
differ only in learning numbers share one twin. Results go to out/sweep/cache.jsonl, keyed by
the command, so a rerun only runs what is missing. Score is the task's window score: late is
the mean of the last tenth of the windows, early the first tenth.
"""

import argparse
import concurrent.futures
import hashlib
import json
import os
import statistics
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "out", "sweep")
CACHE = os.path.join(OUT, "cache.jsonl")

# genome numbers that only matter while learning is on: the twin ignores them
LEARNING_ONLY = {
    "learning_on", "recent_ticks", "predict_shift", "reward_shift", "reward_ticks",
    "grow_shift", "prune_shift", "prune_bit_shift", "far_redraw_shift", "surprise_boost",
    "surprise_high_per_1024", "predict_on", "good_on", "bad_on", "grow_on", "prune_on",
    "far_redraw_on", "learn_mode", "flip_count", "flip_scope", "flip_window_ticks",
    "flip_average_shift", "noise_learn_shift", "noise_trace_ticks", "reward_average_shift",
    "readout_shift", "readout_trace_ticks", "readout_counts", "readout_know_button",
    "readout_good", "readout_new", "readout_budget", "readout_far",
    "readout_center", "readout_evidence", "readout_evidence_bits", "readout_evidence_strong",
}


def set_out(path):
    global OUT, CACHE
    OUT = os.path.abspath(path)
    CACHE = os.path.join(OUT, "cache.jsonl")


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
    variants = []
    for line in open(path):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, _, args = line.partition(":")
        variants.append((name.strip(), args.split()))
    return variants


def genome_pairs(args):
    pairs = {}
    for i in range(0, len(args) - 1, 2):
        if args[i].startswith("--g."):
            pairs[args[i][4:]] = args[i + 1]
    return pairs


def twin_args(args):
    pairs = {k: v for k, v in genome_pairs(args).items() if k not in LEARNING_ONLY}
    # plug and task flags (not genome numbers) stay as they are
    out = []
    for i in range(0, len(args) - 1, 2):
        if not args[i].startswith("--g."):
            out += [args[i], args[i + 1]]
    for k in sorted(pairs):
        out += ["--g." + k, pairs[k]]
    return out + ["--g.learning_on", "0"]


def load_cache():
    cache = {}
    if os.path.exists(CACHE):
        for line in open(CACHE):
            row = json.loads(line)
            cache[row["key"]] = row
    return cache


def run_one(blobrun, cmd_args):
    key = " ".join(cmd_args)
    csv = os.path.join(OUT, hashlib.sha1(key.encode()).hexdigest()[:16] + ".csv")
    cmd = ["nice", "-n", "19", blobrun] + cmd_args + ["--no-baselines", "--out", csv]
    done = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    if done.returncode != 0:
        raise RuntimeError(key + "\n" + done.stderr)
    lines = open(csv).read().split("\n")
    head = lines[0].split(",")
    col = head.index("blob")
    scores = [float(l.split(",")[col]) for l in lines[1:] if l]
    edge = max(1, len(scores) // 10)
    return {
        "key": key,
        "early": statistics.mean(scores[:edge]),
        "late": statistics.mean(scores[-edge:]),
        "windows": scores,
    }


def mean_sd(values):
    if not values:
        return "-"
    m = statistics.mean(values)
    sd = statistics.stdev(values) if len(values) > 1 else 0.0
    big = max(abs(m), sd) >= 10
    return f"{m:.1f} ±{sd:.1f}" if big else f"{m:.3f} ±{sd:.3f}"


def paired(values, base):
    diffs = [a - b for a, b in zip(values, base)]
    m = statistics.mean(diffs)
    se = statistics.stdev(diffs) / len(diffs) ** 0.5 if len(diffs) > 1 else 0.0
    t = m / se if se > 0 else 0.0
    big = abs(m) >= 10 or se >= 10
    text = f"{m:+.1f} ±{se:.1f}" if big else f"{m:+.3f} ±{se:.3f}"
    return text, t


def main():
    argv = sys.argv[1:]
    split = argv.index("--") if "--" in argv else len(argv)
    ap = argparse.ArgumentParser()
    ap.add_argument("--name", required=True)
    ap.add_argument("--seeds", default="1-8")
    ap.add_argument("--variants", required=True)
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--blobrun", default=os.path.join(ROOT, "build", "blobrun"))
    ap.add_argument("--no-twin", action="store_true")
    ap.add_argument("--table-only", action="store_true")
    ap.add_argument("--out-dir", default=OUT, help="where runs and the cache go")
    opt = ap.parse_args(argv[:split])
    set_out(opt.out_dir)
    task_args = argv[split + 1:]
    seeds = parse_seeds(opt.seeds)
    variants = read_variants(opt.variants)
    os.makedirs(OUT, exist_ok=True)

    # every command this table needs
    def blob_cmd(args, seed):
        return task_args + ["--seed", str(seed), "--task-seed", str(seed)] + args

    wanted = []
    for name, args in variants:
        for seed in seeds:
            wanted.append(blob_cmd(args, seed))
            if not opt.no_twin:
                wanted.append(blob_cmd(twin_args(args), seed))
    cache = load_cache()
    todo = []
    for cmd in wanted:
        key = " ".join(cmd)
        if key not in cache and key not in {" ".join(c) for c in todo}:
            todo.append(cmd)

    if todo and not opt.table_only:
        print(f"{opt.name}: {len(todo)} runs to do, {opt.jobs} at a time", file=sys.stderr)
        with concurrent.futures.ThreadPoolExecutor(max_workers=opt.jobs) as pool:
            futures = [pool.submit(run_one, opt.blobrun, cmd) for cmd in todo]
            for n, fut in enumerate(concurrent.futures.as_completed(futures)):
                row = fut.result()
                cache[row["key"]] = row
                with open(CACHE, "a") as f:
                    f.write(json.dumps(row) + "\n")
                print(f"  {n + 1}/{len(todo)}", file=sys.stderr, flush=True)

    def result(cmd, field):
        row = cache.get(" ".join(cmd))
        return row[field] if row else None

    print(f"## {opt.name}\n")
    print(f"`{' '.join(task_args)}`, seeds {opt.seeds}\n")
    print("| variant | late | learning off | late - off | late - early | late vs base | t |")
    print("|---|---|---|---|---|---|---|")
    base_late = None
    for name, args in variants:
        late = [result(blob_cmd(args, s), "late") for s in seeds]
        early = [result(blob_cmd(args, s), "early") for s in seeds]
        off = [result(blob_cmd(twin_args(args), s), "late") for s in seeds]
        if None in late:
            print(f"| {name} | missing runs |")
            continue
        has_off = None not in off
        gain = [a - b for a, b in zip(late, off)] if has_off else []
        rise = [a - b for a, b in zip(late, early)]
        if base_late is None:
            base_late = late
            vs, t = "", ""
        else:
            vs, tv = paired(late, base_late)
            t = f"{tv:+.1f}"
        print(f"| {name} | {mean_sd(late)} | {mean_sd(off) if has_off else '-'} | "
              f"{mean_sd(gain)} | {mean_sd(rise)} | {vs} | {t} |")
    print()
    print("per seed, late:")
    for name, args in variants:
        late = [result(blob_cmd(args, s), "late") for s in seeds]
        cells = " ".join("-" if v is None else (f"{v:.2f}" if v < 10 else f"{v:.0f}")
                         for v in late)
        print(f"  {name:<18} {cells}")
    print()


if __name__ == "__main__":
    main()
