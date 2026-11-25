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

Plug flags (--babble, --balance, ...) go in the variants or after the `--`; every flag there
takes one value. With --table after the `--` the table sheet is read and its twin also gets
--t.learning_on 0. --out DIR keeps the cache elsewhere. When a run writes <stem>-clean.csv
(babbling or balance on, ideas 60 and 61) its clean windows and signal counts are cached too.
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
    "readout_balance",
    "lose_shift_steps", "win_stay_steps", "lose_shift_fires", "bad_cut_on",
    "margin_percent", "margin_win", "margin_rival", "margin_rival_shift",
    "tag_mode", "tag_hops", "tag_thin", "tag_min", "tag_shift", "tag_far", "tag_inh",
    "tag_avg_shift", "tag_bits", "tag_verdict",
    "replay_depth", "replay_shift", "replay_thin", "replay_new", "replay_quiet",
    "replay_until", "replay_reset", "replay_shuffle", "replay_strong_budget", "rule_on",
    "critic_on", "critic_reward", "critic_margin", "critic_center", "critic_average_shift",
    "critic_shift", "critic_trace_steps", "critic_fade", "actor_form", "actor_trace_steps",
    "actor_fade", "explore_shift", "explore_scale", "critic_opponent", "critic_wide",
    "critic_keep_world", "actor_good",
}
# flags with a value that only matter while learning is on (the rule table, idea 120)
LEARNING_FLAGS = {"--rule-file", "--rule-hex"}


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


def twin_args(args, extra=()):
    pairs = {k: v for k, v in genome_pairs(args).items() if k not in LEARNING_ONLY}
    # plug and task flags (not genome numbers) stay as they are
    out = []
    for i in range(0, len(args) - 1, 2):
        if not args[i].startswith("--g.") and args[i] not in LEARNING_FLAGS:
            out += [args[i], args[i + 1]]
    for k in sorted(pairs):
        out += ["--g." + k, pairs[k]]
    return out + ["--g.learning_on", "0"] + list(extra)


def edge_means(scores):
    edge = max(1, len(scores) // 10)
    return statistics.mean(scores[:edge]), statistics.mean(scores[-edge:])


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
    driver = "blob" if "blob" in head else "table"
    col = head.index(driver)
    scores = [float(l.split(",")[col]) for l in lines[1:] if l]
    early, late = edge_means(scores)
    row = {"key": key, "early": early, "late": late, "windows": scores}
    # ideas 60 and 61: windows without babbled steps, and signal counts per step
    clean = csv[:-4] + "-clean.csv"
    if os.path.exists(clean):
        lines = open(clean).read().split("\n")
        col = lines[0].split(",").index(driver)
        row["clean"] = [float(l.split(",")[col]) for l in lines[1:] if l]
        row["late_clean"] = edge_means(row["clean"])[1]
        for line in open(csv[:-4] + "-sum.csv"):
            d, _, rest = line.strip().partition(",")
            k, _, v = rest.partition(",")
            if d == driver and ("per_step" in k or "obeyed" in k or "no_move" in k):
                row[k] = float(v)
    return row


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
    global OUT, CACHE
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
    # two branches each added this; --out and --out-dir are the same setting
    ap.add_argument("--out-dir", "--out", dest="out_dir", default=OUT,
                    help="where runs and the cache go")
    ap.add_argument("--windows", default="",
                    help="A:B scores only windows A to B-1 (a phase of a longer life)")
    opt = ap.parse_args(argv[:split])
    set_out(opt.out_dir)
    task_args = argv[split + 1:]
    # the table sheet keeps its own learning switch
    twin_extra = ["--t.learning_on", "0"] if "--table" in task_args else []
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
                wanted.append(blob_cmd(twin_args(args, twin_extra), seed))
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

    lo, hi = (int(x) for x in opt.windows.split(":")) if opt.windows else (0, 0)

    def result(cmd, field):
        row = cache.get(" ".join(cmd))
        if not row:
            return None
        if not opt.windows:
            return row[field]
        part = row["windows"][lo:hi]
        edge = max(1, len(part) // 10)
        return statistics.mean(part[-edge:] if field == "late" else part[:edge])

    print(f"## {opt.name}\n")
    span = f", windows {opt.windows}" if opt.windows else ""
    print(f"`{' '.join(task_args)}`, seeds {opt.seeds}{span}\n")
    print("| variant | late | learning off | late - off | late - early | late vs base | t |")
    print("|---|---|---|---|---|---|---|")
    base_late = None
    for name, args in variants:
        late = [result(blob_cmd(args, s), "late") for s in seeds]
        early = [result(blob_cmd(args, s), "early") for s in seeds]
        off = [result(blob_cmd(twin_args(args, twin_extra), s), "late") for s in seeds]
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
