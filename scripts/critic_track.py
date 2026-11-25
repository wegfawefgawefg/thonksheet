#!/usr/bin/env python3
"""Does the critic's level know how far a fall is? Reads csvs from `blobprobe critic`.

    scripts/critic_track.py out/critic/track-s5.csv [more.csv ...]

For each csv: the critic cells' mean spikes per step, binned by steps until the next fall
(or judged dot, in catch), counting only steps that have a later fall in the log; the rank
correlation of level with log steps-to-fall (positive: higher where a fall is further off);
and how the surprise came out (share counted good, bad, neither; its spread; how often a
step the world called bad was a bad surprise). Then, for credit one step back: goods paid
in the log, the level and the surprise by steps until the next good; and where the csv has
the world's state, the level by the chain's stage and by mountain car's position and energy.
"""

import csv
import math
import statistics
import sys

BINS = [(1, 1), (2, 2), (3, 4), (5, 8), (9, 16), (17, 32), (33, 64), (65, 1 << 30)]


def ranks(values):
    order = sorted(range(len(values)), key=lambda i: values[i])
    r = [0.0] * len(values)
    i = 0
    while i < len(order):
        j = i
        while j + 1 < len(order) and values[order[j + 1]] == values[order[i]]:
            j += 1
        for k in range(i, j + 1):
            r[order[k]] = (i + j) / 2.0
        i = j + 1
    return r


def spearman(a, b):
    if len(a) < 3:
        return float("nan")
    ra, rb = ranks(a), ranks(b)
    ma, mb = statistics.mean(ra), statistics.mean(rb)
    num = sum((x - ma) * (y - mb) for x, y in zip(ra, rb))
    den = math.sqrt(sum((x - ma) ** 2 for x in ra) * sum((y - mb) ** 2 for y in rb))
    return num / den if den else float("nan")


def analyse(path):
    rows = list(csv.DictReader(open(path)))
    level = [int(r["level"]) for r in rows]
    ended = [int(r["ended"]) for r in rows]
    # steps until the step that ends in a fall (1 = this step falls)
    until = [None] * len(rows)
    nxt = None
    for i in range(len(rows) - 1, -1, -1):
        if ended[i]:
            nxt = i
        until[i] = None if nxt is None else nxt - i + 1
    out = {"path": path, "steps": len(rows), "ends": sum(ended)}
    out["bins"] = []
    for lo, hi in BINS:
        vals = [level[i] for i in range(len(rows)) if until[i] is not None and lo <= until[i] <= hi]
        out["bins"].append((lo, hi, len(vals), statistics.mean(vals) if vals else float("nan")))
    pairs = [(level[i], math.log(until[i])) for i in range(len(rows)) if until[i] is not None]
    out["rho"] = spearman([p[0] for p in pairs], [p[1] for p in pairs])
    out["level_mean"] = statistics.mean(level)
    out["level_sd"] = statistics.pstdev(level)
    sign = [int(r["sign"]) for r in rows]
    surprise = [int(r["surprise"]) for r in rows]
    val = [int(r["valence"]) for r in rows]
    n = len(rows)
    out["good"] = sum(1 for s in sign if s > 0) / n
    out["bad"] = sum(1 for s in sign if s < 0) / n
    out["surprise_mean"] = statistics.mean(surprise)
    out["surprise_sd"] = statistics.pstdev(surprise)
    # the surprise for a step is computed one step later (it needs the next level)
    agree = [(val[i], sign[i + 1]) for i in range(n - 1) if val[i] != 0]
    out["bad_heard"] = (sum(1 for v, s in agree if v < 0 and s < 0) /
                        max(1, sum(1 for v, s in agree if v < 0)))
    out["good_heard"] = (sum(1 for v, s in agree if v > 0 and s > 0) /
                         max(1, sum(1 for v, s in agree if v > 0)))
    out["explored"] = sum(int(r["explored"]) for r in rows) / n
    return out


def toward_good(rows, level):
    """credit one step back: how many goods the world paid in the log, and whether the level
    (and the surprise, one step later) is higher the nearer the next good is"""
    val = [int(r["valence"]) for r in rows]
    sign = [int(r["sign"]) for r in rows]
    n = len(rows)
    until = [None] * n
    nxt = None
    for i in range(n - 1, -1, -1):
        if val[i] > 0:
            nxt = i
        until[i] = None if nxt is None else nxt - i + 1
    goods = sum(1 for v in val if v > 0)
    pairs = [(level[i], math.log(until[i])) for i in range(n) if until[i] is not None]
    rho = spearman([p[0] for p in pairs], [p[1] for p in pairs])
    parts, sparts = [], []
    for lo, hi in BINS:
        idx = [i for i in range(n - 1) if until[i] is not None and lo <= until[i] <= hi]
        if idx:
            parts.append(f"{lo}-{hi if hi < 1 << 20 else '+'}: "
                         f"{statistics.mean(level[i] for i in idx):.1f}")
            sparts.append(f"{lo}-{hi if hi < 1 << 20 else '+'}: "
                          f"{statistics.mean(sign[i + 1] for i in idx):+.2f}")
    print(f"  goods paid in the log {goods} of {n} steps; rank corr of level with log "
          f"steps-to-next-good {rho:+.3f} (negative: higher nearer a good)")
    print("  level by steps to the next good: " + "  ".join(parts))
    print("  surprise sign (one step later) by steps to the next good: " + "  ".join(sparts))


def world_state(path, rows, level):
    """the level against the world's own state, where the csv has it: the chain's stage,
    mountain car's position and energy (16.16)"""
    if not rows or "phase" not in rows[0]:
        return
    phase = [int(r["phase"]) for r in rows]
    if len(set(phase)) > 1:
        parts = []
        for p in sorted(set(phase)):
            vals = [level[i] for i in range(len(rows)) if phase[i] == p]
            parts.append(f"stage {p + 1}: {statistics.mean(vals):.1f} (n {len(vals)})")
        print("  level by the chain's stage: " + "  ".join(parts))
    pos = [int(r["pos"]) / 65536 for r in rows]
    if len(set(pos)) > 10:
        vel = [int(r["vel"]) / 65536 for r in rows]
        # mountain car's height term, as task_classic.cpp: energy = v^2/2 + sin(3x) g/3
        energy = [v * v / 2 + math.sin(3 * x) * 0.0025 / 3 for x, v in zip(pos, vel)]
        print(f"  rank corr of level with position {spearman(level, pos):+.3f}, "
              f"with energy {spearman(level, energy):+.3f}")
        edges = [-1.2, -0.9, -0.6, -0.4, -0.2, 0.0, 0.2, 0.4, 0.6]
        parts = []
        for lo, hi in zip(edges, edges[1:]):
            vals = [level[i] for i in range(len(rows)) if lo <= pos[i] < hi]
            if vals:
                parts.append(f"{lo:+.1f}..{hi:+.1f}: {statistics.mean(vals):.1f} (n {len(vals)})")
        print("  level by position: " + "  ".join(parts))
        qs = sorted(energy)
        cuts = [qs[len(qs) * k // 5] for k in range(1, 5)]
        parts = []
        for k in range(5):
            lo = cuts[k - 1] if k > 0 else -1e9
            hi = cuts[k] if k < 4 else 1e9
            vals = [level[i] for i in range(len(rows)) if lo <= energy[i] < hi]
            if vals:
                parts.append(f"q{k + 1}: {statistics.mean(vals):.1f}")
        print("  level by energy fifth (low to high): " + "  ".join(parts))


def main():
    for path in sys.argv[1:]:
        o = analyse(path)
        print(f"{o['path']}: {o['steps']} steps, {o['ends']} ends, level {o['level_mean']:.1f} "
              f"sd {o['level_sd']:.1f}, rank corr with log steps-to-end {o['rho']:+.3f}")
        print("  steps to end: " + "  ".join(
            f"{lo}-{hi if hi < 1 << 20 else '+'}: {m:.1f} (n {c})" for lo, hi, c, m in o["bins"]))
        print(f"  surprise mean {o['surprise_mean']:+.1f} sd {o['surprise_sd']:.1f}; good "
              f"{o['good']:.2f} bad {o['bad']:.2f}; world bad heard as bad {o['bad_heard']:.2f}, "
              f"good as good {o['good_heard']:.2f}; random presses {o['explored']:.3f}")
        rows = list(csv.DictReader(open(path)))
        toward_good(rows, [int(r["level"]) for r in rows])
        world_state(path, rows, [int(r["level"]) for r in rows])


if __name__ == "__main__":
    main()
