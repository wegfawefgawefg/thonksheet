#!/usr/bin/env python3
"""The suite (docs/bench.md): every row of the bench table for one engine or variant.

    scripts/suite.py NAME [--quick] [--resume] [--only JOB,...] [blobrun flags ...]

Runs each test on the report seeds (5 to 12; --quick: 5 and 6 with short lives), each life
beside its learning-off twin and coin flips, at most 2 runs at a time under nice. Writes
out/scorecards/NAME.csv and prints it. Everything after NAME that is not --quick or
--resume goes to every blobrun call, last, so it can pick an engine (--table, --engine) or
change genome numbers. Nothing here knows which engine runs: all of it goes through
blobrun, and every number comes from the -sum.csv each blobrun call writes.

--only runs just the named jobs (see jobs_for; `--only worlds` is the more-worlds rows of
idea 63) and writes a scorecard with only their rows.
"""

import csv
import os
import statistics
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from map_worlds import PROTOCOL_KEYS, PROTOCOLS, WORLDS  # noqa: E402  the rest of the map

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLOBRUN = os.path.join(ROOT, "build", "blobrun")
JOBS_AT_ONCE = 2

# the standard life (scripts/task.sh): 20 ticks a step, 3x3 groups
BASE = ["--ticks", "20", "--sensor-side", "3", "--motor-side", "3"]
FULL = {"steps": 40000, "window": 1000, "phase_window": 200, "retest": 10000,
        "idle": 400000, "reach": ["--reach-settle", "30000", "--reach-blocks", "4000"],
        "speed_ms": 2000, "alive": 16384, "seeds": list(range(5, 13))}
QUICK = {"steps": 4000, "window": 200, "phase_window": 200, "retest": 1000,
         "idle": 40000, "reach": ["--reach-settle", "4000", "--reach-blocks", "400"],
         "speed_ms": 300, "alive": 4096, "seeds": [5, 6]}

# more worlds (idea 63, docs/curriculum.md): job, blobrun flags, row label, what the score is
PLACE2 = ["--sensor-side", "2", "--number-code", "place"]
MORE_WORLDS = [
    ("bandit2", ["--task", "bandit"], "bandit, 2 buttons (share on the best)"),
    ("bandit4", ["--task", "bandit", "--k", "4"], "bandit, 4 buttons (share on the best)"),
    ("switch2", ["--task", "switch-bandit"],
     "switching bandit, every 1,000 steps (best share, late in each block)"),
    ("chain", ["--task", "chain"], "chain A then B, stage shown (2 x chains / steps)"),
    ("chain-nocue", ["--task", "chain", "--chain-cue", "0"],
     "chain A then B, stage not shown (2 x chains / steps)"),
    ("track", ["--task", "track"], "track 5, nearer good (share of steps nearer)"),
    ("track-arrive", ["--task", "track", "--track-reward", "arrive"],
     "track 5, arrival only (share of steps nearer)"),
    ("forage", ["--task", "forage", "--sensor-side", "2"],
     "forage 5x5, warmer (share of steps nearer)"),
    ("forage-cold", ["--task", "forage", "--forage-warmer", "0", "--sensor-side", "2"],
     "forage 5x5, arrival only (share of steps nearer)"),
    ("mcar", ["--task", "mountain-car"] + PLACE2,
     "mountain car, energy reward (share of episodes at the goal)"),
    ("pend", ["--task", "pendulum"] + PLACE2,
     "pendulum swing-up, energy reward (share of steps upright)"),
    ("cartpole-wide", ["--task", "cartpole", "--cart-reward", "lean", "--cart-start",
                       "wide"] + PLACE2, "cartpole, lean reward, wide starts (steps per fall)"),
    ("cartpole-rel", ["--task", "cartpole", "--cart-reward", "relative"] + PLACE2,
     "cartpole, relative reward (steps per fall)"),
    ("cartpole-semi", ["--task", "cartpole", "--cart-reward", "lean", "--cart-integrator",
                       "semi"] + PLACE2,
     "cartpole, lean reward, semi-implicit: a step's reward judges its press (steps per fall)"),
]

FEATURES = [("order", "order: which of two cues came on first"),
            ("both-not-either", "both-not-either: exactly one of two cues"),
            ("gap", "gap: short or long gap between two blinks"),
            ("place", "place: shape at a place never rewarded")]

# the function battery (scripts/functions.sh), a few rungs of its ladder
FUNCTIONS = [
    ("fn-parity3", "points: parity of 3 lines",
     ["--task", "points", "--fn", "parity", "--fn-lines", "3"]),
    ("fn-parity4", "points: parity of 4 lines",
     ["--task", "points", "--fn", "parity", "--fn-lines", "4"]),
    ("fn-majority5", "points: majority of 5 lines",
     ["--task", "points", "--fn", "majority", "--fn-lines", "5"]),
    ("fn-count4", "points: how many of 4 lines are on (5 buttons)",
     ["--task", "points", "--fn", "count", "--fn-lines", "4"]),
    ("fn-sine-held", "number: band of sine(x), 16 lines, 4 bands, x never rewarded",
     ["--task", "number", "--fn", "sine", "--fn-lines", "16", "--fn-bands", "4",
      "--fn-hold", "3", "--sensor-side", "2"]),
    ("fn-product", "two numbers: product above or below half, 8 lines each",
     ["--task", "two-numbers", "--fn", "product", "--fn-lines", "8", "--fn-bands", "2",
      "--sensor-side", "2"]),
    ("fn-recall", "past: the first of 2 items, 2 blank steps between",
     ["--task", "past", "--fn", "recall", "--fn-gap", "2"]),
    ("fn-same", "past: the last 2 items the same, back to back",
     ["--task", "past", "--fn", "same", "--fn-gap", "0"]),
]


def jobs_for(life, readout):
    """name -> (blobrun args, rough cost in lives of the standard length)"""
    s, w = str(life["steps"]), str(life["window"])
    plain = ["--steps", s, "--window", w]
    jobs = {
        "capacity": (["--capacity"] + plain, 4),
        "shuffled": (["--task", "shuffled-echo"] + plain, 1),
        "delayed": (["--task", "delayed-echo", "--delay", "2"] + plain, 1),
        "beat": (["--task", "beat", "--period", "4"] + plain, 1),
        "sequence": (["--task", "sequence"] + plain, 1),
        "catch": (["--task", "catch", "--catch-size", "4", "4"] + plain, 1),
        # 24 number lines need 2x2 sensor groups. lean rewards every step; sparse only every
        # 25 steps up, the slow-credit row
        "cartpole": (["--task", "cartpole", "--sensor-side", "2", "--cart-reward", "lean",
                      "--number-code", "place"] + plain, 1),
        "cartpole-sparse": (["--task", "cartpole", "--sensor-side", "2", "--cart-reward",
                             "sparse", "--number-code", "place"] + plain, 1),
        "reach": (["--reach"] + life["reach"], 0.2),
        "retention": (["--task", "echo", "--retention", "echo", "--steps", s, "--window",
                       str(life["phase_window"]), "--retest", str(life["retest"])], 2.3),
        "idle": (["--task", "echo", "--idle", str(life["idle"]), "--steps", s, "--window",
                  str(life["phase_window"]), "--retest", str(life["retest"])], 1.8),
    }
    for d in (1, 2, 3, 5):
        jobs["distance%d" % d] = (["--task", "echo", "--distance", str(d)] + plain, 1)
    for task, _ in FEATURES:
        jobs["full-" + task] = (["--task", task] + plain, 1)
        if readout:
            jobs["readout-" + task] = (["--task", task, "--frozen-readout"] + plain, 1)
    if readout:
        # the control: a readout that can learn at all, where the input hands over the answer
        jobs["readout-echo"] = (["--task", "echo", "--frozen-readout"] + plain, 1)
    # the rest of the map (scripts/map_worlds.py): tiers 9 and 10, being shown, damage
    for name, kind, _, args in WORLDS:
        jobs["map-" + name] = (args + plain, 1)
    for name, kind, _, args in PROTOCOLS:
        jobs["map-" + name] = (args + ["--steps", s, "--window", str(life["phase_window"]),
                                       "--retest", str(life["retest"])], 1.3)
    for job, _, args in FUNCTIONS:
        jobs[job] = (args + plain, 1)
    for job, flags, _ in MORE_WORLDS:
        jobs[job] = (flags + plain, 1)
    return jobs


# (property, test, job, key). key "late" is the late score: the mean of the last tenth of
# the windows. the learning-off and chance columns are the same key for those drivers.
def rows_for():
    rows = [
        ("learns at all", "echo k=2", "capacity", "k2_late"),
        ("learns at all", "echo k=4", "capacity", "k4_late"),
        ("learns at all", "echo k=8", "capacity", "k8_late"),
        ("learns at all", "shuffled echo k=2", "shuffled", "late"),
        ("a sense of time", "delayed echo k=2, 2 steps", "delayed", "late"),
        ("a sense of time", "beat, every 4 steps", "beat", "late"),
        ("a sense of time", "sequence, 8 lines x 4 (prediction)", "sequence", "late"),
        ("a sense of time", "sequence, surprise falling (late - early)", "sequence",
         "late_minus_early"),
        ("acting in a world", "catch 4x4", "catch", "late"),
        ("acting in a world", "cartpole, lean reward (steps per fall)", "cartpole", "late"),
        ("acting in a world", "cartpole, sparse reward (steps per fall)", "cartpole-sparse",
         "late"),
        ("reach", "cue readable how far (tiles)", "reach", "reach"),
    ]
    for d in range(1, 8):
        rows.append(("reach", "cue readable at %d tiles" % d, "reach", "readable_%d" % d))
    for d in (1, 2, 3, 5):
        rows.append(("reach", "echo k=2, cue and button %d tiles apart" % d, "distance%d" % d,
                     "late"))
    rows.append(("features nobody designed in", "control: echo k=2 (frozen sheet, readout "
                 "learns)", "readout-echo", "late"))
    for task, label in FEATURES:
        rows.append(("features nobody designed in", label + " (frozen sheet, readout learns)",
                     "readout-" + task, "late"))
    for task, label in FEATURES:
        rows.append(("features nobody designed in", label + " (whole blob learns)",
                     "full-" + task, "late"))
    rows += [
        ("keeps learning, keeps what it had", "echo A at the end of learning it", "retention",
         "a_end"),
        ("keeps learning, keeps what it had", "echo B (2 new cues and buttons) at its end",
         "retention", "b_end"),
        ("keeps learning, keeps what it had", "echo A retested after B, first window",
         "retention", "a_retest_start"),
        ("keeps learning, keeps what it had", "echo A retested after B, end", "retention",
         "a_retest_end"),
        ("keeps learning, keeps what it had", "capacity: largest k held (2..16)", "capacity",
         "capacity"),
        ("keeps learning, keeps what it had", "echo k=16", "capacity", "k16_late"),
        ("does not wreck itself idle", "echo before unplugging", "idle", "before"),
        ("does not wreck itself idle", "echo after unplugged, first window", "idle",
         "after_start"),
        ("does not wreck itself idle", "echo after unplugged, end", "idle", "after_end"),
    ]
    end = [("room", "connections per neuron", "connections_per_neuron"),
           ("room", "share of connections at strength 1", "share_strength_1"),
           ("room", "share of connections at strength 4", "share_strength_4"),
           ("room", "share of connections at strength 5 (top)", "share_strength_5"),
           ("room", "share of neurons at the connection cap", "share_at_cap"),
           ("room", "share of neurons with threshold at cap", "share_threshold_cap"),
           ("stays alive", "firing rate over target", "rate_over_target"),
           ("stays alive", "share of silent neurons", "share_silent"),
           ("stays alive", "share of stuck neurons", "share_stuck"),
           ("stays alive", "rhythm period (ticks, 0 none)", "rhythm_period"),
           ("stays alive", "rhythm strength", "rhythm_strength")]
    for prop, test, key in end:
        rows.append((prop, test + ", end of echo k=2 life", "capacity", key))
    rows.append(("fits the machine", "ticks/s in the echo k=2 life", "capacity", "ticks_per_s"))
    for size in ("8x8", "16x16", "32x32"):
        rows.append(("fits the machine", "ticks/s at %s tiles, random input" % size, "speed",
                     "ticks_per_s_" + size))
    rows.append(("fits the machine", "bytes per neuron (32x32 tiles)", "speed",
                 "bytes_per_neuron_32x32"))
    # the rest of the map: the window score late in the life (no-teacher worlds: the sheet's
    # own predictions; meaningless for engines that keep none, such as the minimal one)
    for name, kind, what, _ in WORLDS:
        rows.append(("map: " + kind, "%s: %s" % (name, what), "map-" + name, "late"))
    for name, kind, what, _ in PROTOCOLS:
        for key in PROTOCOL_KEYS[kind]:
            rows.append(("map: " + kind, "%s: %s, %s" % (name, what, key), "map-" + name, key))
    for job, label, _ in FUNCTIONS:
        rows.append(("fixed functions (the battery)", label, job, "late"))
    # after every older row, so old scorecards still line up
    for job, _, label in MORE_WORLDS:
        rows.append(("more worlds (idea 63)", label, job, "late"))
    return rows


# keys where a negative number means "this engine does not have it"
UNKNOWN_IF_NEGATIVE = {"connections_per_neuron", "share_strength_1", "share_strength_4",
                       "share_strength_5", "share_at_cap", "share_threshold_cap",
                       "rate_over_target", "share_silent", "share_stuck", "rhythm_period",
                       "rhythm_strength"}


def read_sum(path):
    """driver -> key -> value"""
    out = {}
    if not os.path.exists(path):
        return out
    with open(path) as f:
        next(f, None)
        for line in f:
            parts = line.strip().split(",")
            if len(parts) == 3:
                out.setdefault(parts[0], {})[parts[1]] = float(parts[2])
    return out


def run_one(cmd, log):
    with open(log, "w") as f:
        f.write(" ".join(cmd) + "\n")
        f.flush()
        return subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT).returncode


def readout_supported(extra, scratch):
    cmd = ["nice", "-n", "19", BLOBRUN, "--frozen-readout", "--steps", "1", "--window", "1",
           "--no-baselines", "--alive-ticks", "0", "--rhythm-ticks", "0", "--out",
           os.path.join(scratch, "readout-check.csv")] + extra
    return subprocess.run(cmd, stdout=subprocess.DEVNULL,
                          stderr=subprocess.DEVNULL).returncode == 0


def fmt(v, key):
    if v is None:
        return "n/a"
    if key.startswith("ticks_per_s") or key.startswith("bytes"):
        return "%.0f" % v
    if abs(v) >= 100:
        return "%.0f" % v
    if abs(v) >= 10:
        return "%.1f" % v
    return "%.3f" % v


def collect(rows, jobs, seeds, outdir):
    """one dict per row: mean, spread, learn_off, chance, seeds"""
    table = []
    for prop, test, job, key in rows:
        if job not in jobs and job != "speed":
            table.append(dict(prop=prop, test=test, key=key, setting="not run"))
            continue
        vals = {"blob": [], "learn_off": [], "coin": []}
        job_seeds = seeds[:1] if job == "speed" else seeds
        for seed in job_seeds:
            sums = read_sum(os.path.join(outdir, "%s-s%d-sum.csv" % (job, seed)))
            # an engine with its own driver names (--table) stands in for the blob
            names = {"blob": "blob", "learn_off": "learn_off", "coin": "coin"}
            if "blob" not in sums and "table" in sums:
                names.update(blob="table", learn_off="table_off")
            for driver in vals:
                v = sums.get(names[driver], {}).get(key)
                if v is not None and not (key in UNKNOWN_IF_NEGATIVE and v < 0):
                    vals[driver].append(v)
        b = vals["blob"]
        row = dict(prop=prop, test=test, key=key, n=len(b),
                   mean=statistics.mean(b) if b else None,
                   spread=statistics.stdev(b) if len(b) > 1 else None,
                   learn_off=statistics.mean(vals["learn_off"]) if vals["learn_off"] else None,
                   chance=statistics.mean(vals["coin"]) if vals["coin"] else None,
                   setting=" ".join(jobs[job][0]) if job in jobs else "--speed")
        table.append(row)
    return table


def write_card(path, table, about):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", newline="") as f:
        out = csv.writer(f)
        out.writerow(["property", "test", "mean", "spread", "learn_off", "chance", "seeds",
                      "setting"])
        for k, v in about:
            out.writerow(["about", k, "", "", "", "", "", v])
        for r in table:
            cells = [r["prop"], r["test"]]
            for k in ("mean", "spread", "learn_off", "chance"):
                v = r.get(k)
                cells.append("" if v is None else "%.6g" % v)
            cells.append(str(r.get("n", 0)))
            cells.append(r.get("setting", ""))
            out.writerow(cells)


def print_card(table):
    print("%-62s %9s %9s %9s %9s %3s" % ("test", "mean", "spread", "learn off", "chance", "n"))
    prop = None
    for r in table:
        if r["prop"] != prop:
            prop = r["prop"]
            print("-- " + prop)
        print("%-62s %9s %9s %9s %9s %3s" % (
            r["test"][:62], fmt(r.get("mean"), r["key"]), fmt(r.get("spread"), r["key"]),
            fmt(r.get("learn_off"), r["key"]), fmt(r.get("chance"), r["key"]), r.get("n", 0)))


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 1
    name = args[0]
    quick = "--quick" in args[1:]
    resume = "--resume" in args[1:]
    only = None
    extra = []
    rest = args[1:]
    while rest:
        a = rest.pop(0)
        if a == "--only" and rest:
            more = ",".join(w[0] for w in MORE_WORLDS)
            only = rest.pop(0).replace("worlds", more).split(",")
        elif a not in ("--quick", "--resume"):
            extra.append(a)
    life = QUICK if quick else FULL
    card_name = name + ("-quick" if quick else "")
    outdir = os.path.join(ROOT, "out", "suite", card_name)
    os.makedirs(outdir, exist_ok=True)

    readout = readout_supported(extra, outdir)
    jobs = jobs_for(life, readout)
    if only:
        unknown = [j for j in only if j not in jobs]
        if unknown:
            print("no job %s; jobs: %s" % (" ".join(unknown), " ".join(jobs)))
            return 1
        jobs = {j: jobs[j] for j in only}
    life_flags = BASE + ["--alive-ticks", str(life["alive"])]
    todo = []
    for job, (job_args, cost) in jobs.items():
        for seed in life["seeds"]:
            todo.append((cost, job, seed, job_args))
    todo.sort(key=lambda t: -t[0])

    def launch(item):
        cost, job, seed, job_args = item
        stem = os.path.join(outdir, "%s-s%d" % (job, seed))
        if resume and os.path.exists(stem + "-sum.csv"):
            return job, seed, 0
        cmd = (["nice", "-n", "19", BLOBRUN] + life_flags + job_args +
               ["--seed", str(seed), "--task-seed", str(seed), "--out", stem + ".csv"] + extra)
        return job, seed, run_one(cmd, stem + ".txt")

    print("suite %s: %d runs, %d at a time, seeds %s%s" % (
        card_name, len(todo), JOBS_AT_ONCE, life["seeds"],
        "" if readout else "; no frozen-readout mode in this engine (genome learn_mode)"))
    started = time.time()
    failed = []
    done = 0
    with ThreadPoolExecutor(max_workers=JOBS_AT_ONCE) as pool:
        for job, seed, code in pool.map(launch, todo):
            done += 1
            if code != 0:
                failed.append("%s-s%d" % (job, seed))
            if done % 8 == 0 or done == len(todo):
                print("  %d of %d runs done, %.0f min" % (done, len(todo),
                                                         (time.time() - started) / 60), flush=True)
    # speed alone, after everything else, so the suite's own other run is not beside it
    speed_stem = os.path.join(outdir, "speed-s%d" % life["seeds"][0])
    if not only and not (resume and os.path.exists(speed_stem + "-sum.csv")):
        code = run_one(["nice", "-n", "19", BLOBRUN, "--speed", "--speed-ms",
                        str(life["speed_ms"]), "--seed", str(life["seeds"][0]), "--out",
                        speed_stem + ".csv"] + extra, speed_stem + ".txt")
        if code != 0:
            failed.append("speed")
    minutes = (time.time() - started) / 60

    rows = rows_for()
    if only:
        rows = [r for r in rows if r[2] in jobs]
    table = collect(rows, jobs, life["seeds"], outdir)
    commit = subprocess.run(["git", "-C", ROOT, "rev-parse", "--short", "HEAD"],
                            capture_output=True, text=True).stdout.strip()
    about = [("name", card_name), ("flags", " ".join(extra)),
             ("life", " ".join(BASE) + " --steps %d --window %d" % (life["steps"],
                                                                   life["window"])),
             ("seeds", " ".join(str(s) for s in life["seeds"])),
             ("date", time.strftime("%Y-%m-%d")), ("commit", commit),
             ("wall", "%.0f min, %d runs at a time" % (minutes, JOBS_AT_ONCE)),
             ("speed", "single core under nice -n 19, CPU clock capped at about 4.6 GHz, "
                       "other agents' runs on the machine: not comparable with quiet runs"),
             ("frozen readout", "yes" if readout else "not in this engine")]
    if only:
        about.append(("only", " ".join(only)))
    card = os.path.join(ROOT, "out", "scorecards", card_name + ".csv")
    write_card(card, table, about)
    print_card(table)
    print("scorecard: %s (%.0f min)" % (os.path.relpath(card, ROOT), minutes))
    if failed:
        print("FAILED runs (see their .txt in %s): %s" % (os.path.relpath(outdir, ROOT),
                                                         " ".join(failed)))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
