#!/usr/bin/env python3
"""Idea 53 helper: read sweep.py's cache and split a shuffled-echo k=2 table into straight
and crossed lives, plus late-life stability. Runs nothing; run sweep.py first.

    python3 scripts/variants/lose_shift_split.py --seeds 11-24 \
        --variants scripts/variants/lose-shift.txt -- --task shuffled-echo --k 2 --ticks 20 \
        --sensor-side 3 --motor-side 3 --steps 40000 --window 1000

A life is crossed when the task's draw put cue 1 on button 0 (k=2 only): the world's
splitmix64 seeded with the task seed, one draw at seed time, the next one picks the swap.
--stable adds, per variant, the lowest window of the last tenth and the lowest window after
the life first reached 0.9.
"""

import json
import os
import statistics
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CACHE = os.path.join(ROOT, "out", "sweep", "cache.jsonl")
M = (1 << 64) - 1


def splitmix(state):
    state = (state + 0x9E3779B97F4A7C15) & M
    z = state
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & M
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & M
    return state, z ^ (z >> 31)


def crossed(seed):
    state, _ = splitmix(seed)
    state, word = splitmix(state)
    return ((word >> 32) * 2) >> 32 == 0


def parse_seeds(text):
    out = []
    for part in text.split(","):
        if "-" in part:
            lo, hi = part.split("-")
            out += list(range(int(lo), int(hi) + 1))
        else:
            out.append(int(part))
    return out


def read_variants(path):
    out = []
    for line in open(path):
        line = line.strip()
        if line and not line.startswith("#"):
            name, _, args = line.partition(":")
            out.append((name.strip(), args.split()))
    return out


def mean_se(v):
    if not v:
        return "-"
    se = statistics.stdev(v) / len(v) ** 0.5 if len(v) > 1 else 0.0
    return f"{statistics.mean(v):.3f} ±{se:.3f}"


def main():
    argv = sys.argv[1:]
    split = argv.index("--")
    opts, task_args = argv[:split], argv[split + 1:]
    seeds = parse_seeds(opts[opts.index("--seeds") + 1])
    variants = read_variants(opts[opts.index("--variants") + 1])
    stable = "--stable" in opts
    no_split = "--no-split" in opts
    cache = {}
    for line in open(CACHE):
        row = json.loads(line)
        cache[row["key"]] = row

    def row(args, s):
        return cache.get(" ".join(task_args + ["--seed", str(s), "--task-seed", str(s)] + args))

    cross = [s for s in seeds if crossed(s)]
    straight = [s for s in seeds if not crossed(s)]
    if not no_split:
        print(f"straight seeds {straight}, crossed seeds {cross}\n")
        print("| variant | straight | crossed | worst crossed | crossed vs base | t |")
        print("|---|---|---|---|---|---|")
    base = None
    for name, args in variants:
        rows = {s: row(args, s) for s in seeds}
        if None in rows.values():
            print(f"| {name} | missing runs |")
            continue
        late = {s: rows[s]["late"] for s in seeds}
        if base is None:
            base = late
        if not no_split:
            c = [late[s] for s in cross]
            d = [late[s] - base[s] for s in cross]
            se = statistics.stdev(d) / len(d) ** 0.5 if len(d) > 1 else 0.0
            t = statistics.mean(d) / se if se > 0 else 0.0
            vs = f"{statistics.mean(d):+.3f} ±{se:.3f}" if late is not base else ""
            tt = f"{t:+.1f}" if late is not base else ""
            print(f"| {name} | {mean_se([late[s] for s in straight])} | {mean_se(c)} | "
                  f"{min(c) if c else 0:.2f} | {vs} | {tt} |")
    if stable:
        print("\n| variant | last tenth | lowest window, last tenth | lowest window after first "
              "0.9 | windows under 0.8 after first 0.9 |")
        print("|---|---|---|---|---|")
        for name, args in variants:
            rows = [row(args, s) for s in seeds]
            if None in rows:
                continue
            tenth, low_tenth, low_after, dips = [], [], [], []
            for r in rows:
                w = r["windows"]
                edge = max(1, len(w) // 10)
                tenth.append(r["late"])
                low_tenth.append(min(w[-edge:]))
                first = next((i for i, x in enumerate(w) if x >= 0.9), None)
                if first is not None:
                    low_after.append(min(w[first:]))
                    dips.append(sum(1 for x in w[first:] if x < 0.8))
            reached = f"{len(low_after)}/{len(rows)} reached 0.9"
            print(f"| {name} | {mean_se(tenth)} | {mean_se(low_tenth)} (min "
                  f"{min(low_tenth):.2f}) | {mean_se(low_after)} ({reached}) | "
                  f"{statistics.mean(dips) if dips else 0:.1f} |")


if __name__ == "__main__":
    main()
