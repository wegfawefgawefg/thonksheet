#!/usr/bin/env python3
"""The ladder of worlds (idea 63): every world for every engine and rule, beside its
learning-off twin and coin flips, at the standard life or a long one.

    scripts/ladder.py run   [--long] [--rules R,...] [--worlds W,...] [--seeds 5-12]
    scripts/ladder.py table [--rules R,...] [--worlds W,...] [--seeds 5-12]

run: one blobrun call per world, rule and seed, 2 at a time under nice, into
out/ladder/<life>/ (or --out-dir). A call whose -sum.csv exists is skipped, so a stopped
run picks up where it was. The standard life is 40,000 steps with windows of 1,000 and
runs the twin and coin flips in the same call; --long is 1,000,000 steps in 100 windows
with no baselines (the twin cannot change with learning off, so the long table uses the
standard life's twin and coin on the same seed).

table: one markdown table per life length found: late score (mean of the last tenth of
the windows) over the seeds, the twin's, coin's, the paired gain over the twin with its
t, and episodes per life.

Rules: minimal (--engine minimal), table (--table), blob (the full engine as it is), and
readout (the full engine with idea 58's rule: learn_mode 5, evidence counters, balance,
128 spread motor cells per button).
"""

import argparse
import os
import statistics
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLOBRUN = os.path.join(ROOT, "build", "blobrun")
BASE = ["--ticks", "20", "--sensor-side", "3", "--motor-side", "3"]
PLACE2 = ["--sensor-side", "2", "--number-code", "place"]
CART = ["--task", "cartpole"] + PLACE2

# name -> (blobrun flags, what it is)
WORLDS = {
    "bandit2": (["--task", "bandit"], "bandit, 2 buttons, 80% / 20%"),
    "bandit4": (["--task", "bandit", "--k", "4"], "bandit, 4 buttons"),
    "switch2": (["--task", "switch-bandit"], "switching bandit, 2 buttons, every 1,000"),
    "chain": (["--task", "chain"], "chain A then B, stage shown"),
    "chain-nocue": (["--task", "chain", "--chain-cue", "0"], "chain, stage not shown"),
    "track": (["--task", "track"], "track 5, nearer good / further bad"),
    "track-arrive": (["--task", "track", "--track-reward", "arrive"], "track 5, arrival only"),
    "forage": (["--task", "forage", "--sensor-side", "2"], "forage 5x5, warmer"),
    "forage-cold": (["--task", "forage", "--forage-warmer", "0", "--sensor-side", "2"],
                    "forage 5x5, arrival only"),
    "mcar": (["--task", "mountain-car"] + PLACE2, "mountain car, energy reward"),
    "mcar-goal": (["--task", "mountain-car", "--classic-reward", "goal"] + PLACE2,
                  "mountain car, goal only"),
    "pend": (["--task", "pendulum"] + PLACE2, "pendulum, energy reward"),
    "pend-goal": (["--task", "pendulum", "--classic-reward", "goal"] + PLACE2,
                  "pendulum, upright only"),
    "catch": (["--task", "catch", "--catch-size", "4", "4"], "catch 4x4"),
    "cartpole": (CART + ["--cart-reward", "lean"], "cartpole, lean reward"),
    "cartpole-wide": (CART + ["--cart-reward", "lean", "--cart-start", "wide"],
                      "cartpole, lean reward, wide starts"),
    "cartpole-rel": (CART + ["--cart-reward", "relative"], "cartpole, relative reward"),
    "cartpole-semi": (CART + ["--cart-reward", "lean", "--cart-integrator", "semi"],
                      "cartpole, lean reward, semi-implicit (a step's reward judges its press)"),
}

RULES = {
    "minimal": ["--engine", "minimal"],
    "table": ["--table"],
    "blob": [],
    "readout": ["--g.learn_mode", "5", "--g.readout_evidence", "1", "--g.readout_shift", "4",
                "--g.readout_balance", "1", "--motor-spread", "128"],
}
# rough cost of one life against the minimal engine's, for ordering the work
COST = {"minimal": 1, "table": 5.5, "blob": 5, "readout": 6.5}

LIVES = {
    "standard": ["--steps", "40000", "--window", "1000"],
    "long": ["--steps", "1000000", "--windows", "100", "--no-baselines"],
}


def parse_seeds(text):
    seeds = []
    for part in text.split(","):
        lo, _, hi = part.partition("-")
        seeds += list(range(int(lo), int(hi or lo) + 1))
    return seeds


def read_sum(path):
    out = {}
    if not os.path.exists(path):
        return out
    with open(path) as f:
        next(f, None)
        for line in f:
            parts = line.strip().split(",")
            if len(parts) == 3:
                out.setdefault(parts[0], {})[parts[1]] = float(parts[2])
    # the table engine has its own driver names
    if "table" in out:
        out["blob"] = out.pop("table")
        if "table_off" in out:
            out["learn_off"] = out.pop("table_off")
    return out


def stem(out_dir, life, world, rule, seed):
    return os.path.join(out_dir, life, "%s-%s-s%d" % (world, rule, seed))


def run(opt, worlds, rules, seeds):
    life = "long" if opt.long else "standard"
    os.makedirs(os.path.join(opt.out_dir, life), exist_ok=True)
    todo = []
    for world in worlds:
        for rule in rules:
            for seed in seeds:
                s = stem(opt.out_dir, life, world, rule, seed)
                # a finished call has the blob's late score in its summary
                if "late" not in read_sum(s + "-sum.csv").get("blob", {}):
                    todo.append((COST[rule], world, rule, seed, s))
    # cheap engines first, so the first answers come early
    todo.sort(key=lambda t: t[0])
    print("ladder %s: %d runs, %d at a time" % (life, len(todo), opt.jobs), flush=True)

    def launch(item):
        _, world, rule, seed, s = item
        cmd = (["nice", "-n", "19", opt.blobrun] + BASE + LIVES[life] + WORLDS[world][0] +
               RULES[rule] + ["--seed", str(seed), "--task-seed", str(seed), "--out",
                              s + ".csv"])
        with open(s + ".txt", "w") as f:
            f.write(" ".join(cmd) + "\n")
            f.flush()
            code = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT).returncode
        if code != 0 and os.path.exists(s + "-sum.csv"):
            os.remove(s + "-sum.csv")
        return world, rule, seed, code

    started = time.time()
    failed = []
    with ThreadPoolExecutor(max_workers=opt.jobs) as pool:
        for n, (world, rule, seed, code) in enumerate(pool.map(launch, todo)):
            if code != 0:
                failed.append("%s-%s-s%d" % (world, rule, seed))
            print("  %d/%d %s %s s%d  %.0f min" % (n + 1, len(todo), world, rule, seed,
                                                  (time.time() - started) / 60), flush=True)
    if failed:
        print("FAILED: " + " ".join(failed))
        return 1
    return 0


def fmt(v):
    if v is None:
        return "-"
    return "%.1f" % v if abs(v) >= 10 else "%.3f" % v


def table(opt, worlds, rules, seeds):
    for life in ("standard", "long"):
        if not os.path.isdir(os.path.join(opt.out_dir, life)):
            continue
        print("## %s life (%s), seeds %s\n" % (life, " ".join(LIVES[life]), opt.seeds))
        print("| world | rule | late | twin | coin | late - twin | t | late - early | "
              "episodes | n |")
        print("|---|---|---|---|---|---|---|---|---|---|")
        for world in worlds:
            for rule in rules:
                late, twin, coin, rise, eps = [], [], [], [], []
                for seed in seeds:
                    s = read_sum(stem(opt.out_dir, life, world, rule, seed) + "-sum.csv")
                    if "blob" not in s:
                        continue
                    # the long life's twin and coin are the standard life's, same seed
                    b = read_sum(stem(opt.out_dir, "standard", world, rule, seed) + "-sum.csv")
                    late.append(s["blob"]["late"])
                    rise.append(s["blob"]["late_minus_early"])
                    eps.append(s["blob"].get("episodes", 0))
                    twin.append(b.get("learn_off", {}).get("late"))
                    coin.append(b.get("coin", {}).get("late"))
                if not late:
                    continue
                pairs = [(a, b) for a, b in zip(late, twin) if b is not None]
                gain, t = None, None
                if len(pairs) > 1:
                    d = [a - b for a, b in pairs]
                    gain = statistics.mean(d)
                    se = statistics.stdev(d) / len(d) ** 0.5
                    t = gain / se if se > 0 else 0.0
                twin = [v for v in twin if v is not None]
                coin = [v for v in coin if v is not None]
                sd = statistics.stdev(late) if len(late) > 1 else 0.0
                print("| %s | %s | %s ±%s | %s | %s | %s | %s | %s | %.0f | %d |" % (
                    world, rule, fmt(statistics.mean(late)), fmt(sd),
                    fmt(statistics.mean(twin) if twin else None),
                    fmt(statistics.mean(coin) if coin else None),
                    "-" if gain is None else ("%+.3f" % gain if abs(gain) < 10 else
                                              "%+.1f" % gain),
                    "-" if t is None else "%+.1f" % t, fmt(statistics.mean(rise)),
                    statistics.mean(eps), len(late)))
        print()
        print("per seed, late:\n")
        for world in worlds:
            for rule in rules:
                cells = []
                for seed in seeds:
                    s = read_sum(stem(opt.out_dir, life, world, rule, seed) + "-sum.csv")
                    cells.append(fmt(s["blob"]["late"]) if "blob" in s else "-")
                if any(c != "-" for c in cells):
                    print("    %-14s %-8s %s" % (world, rule, " ".join(cells)))
        print()
    return 0


def main():
    ap = argparse.ArgumentParser(usage=__doc__)
    ap.add_argument("what", choices=["run", "table"])
    ap.add_argument("--long", action="store_true")
    ap.add_argument("--rules", default=",".join(RULES))
    ap.add_argument("--worlds", default=",".join(WORLDS))
    ap.add_argument("--seeds", default="5-12")
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--out-dir", default=os.path.join(ROOT, "out", "ladder"))
    ap.add_argument("--blobrun", default=BLOBRUN, help="a frozen copy, to rebuild meanwhile")
    opt = ap.parse_args()
    opt.out_dir = os.path.abspath(opt.out_dir)
    worlds = opt.worlds.split(",")
    rules = opt.rules.split(",")
    for w in worlds:
        if w not in WORLDS:
            sys.exit("no world %s; worlds: %s" % (w, " ".join(WORLDS)))
    for r in rules:
        if r not in RULES:
            sys.exit("no rule %s; rules: %s" % (r, " ".join(RULES)))
    seeds = parse_seeds(opt.seeds)
    if opt.what == "run":
        return run(opt, worlds, rules, seeds)
    return table(opt, worlds, rules, seeds)


if __name__ == "__main__":
    sys.exit(main())
