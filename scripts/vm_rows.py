#!/usr/bin/env python3
"""The vm engine's own rows (docs/results-2026-11-25-vm-route.md): named worlds on a range
of seeds, two lives at a time under nice, each beside its learning-off twin and coin flips.

    scripts/vm_rows.py TAG ROW,ROW,... SEEDS [--steps N] [blobrun flags, --v.name value ...]
    scripts/vm_rows.py report hold,cues,steps 5-12

SEEDS is a range like 5-12. A group name (hold, cues, steps, time) stands for its rows.
Lives are the standard one (scripts/task.sh): 20 ticks a step, 3x3 groups, 40,000 steps,
windows of 1,000. Runs land in out/vm/rows/TAG/; a finished run is not run again.
"""
import os
import statistics
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLOBRUN = os.path.join(ROOT, "build", "blobrun")
BASE = ["--engine", "vm", "--ticks", "20", "--sensor-side", "3", "--motor-side", "3"]
ROWS = {
    "echo2": ["--task", "echo", "--k", "2"],
    "echo8": ["--task", "echo", "--k", "8"],
    "echo16": ["--task", "echo", "--k", "16"],
    # 32 cue lines and buttons only fit at 2x2 groups
    "echo32": ["--task", "echo", "--k", "32", "--sensor-side", "2", "--motor-side", "2"],
    "shuffled": ["--task", "shuffled-echo"],
    "delay0": ["--task", "delayed-echo", "--delay", "0"],
    "delay1": ["--task", "delayed-echo", "--delay", "1"],
    "delay2": ["--task", "delayed-echo", "--delay", "2"],
    "delay5": ["--task", "delayed-echo", "--delay", "5"],
    "delay20": ["--task", "delayed-echo", "--delay", "20"],
    "delay2k8": ["--task", "delayed-echo", "--delay", "2", "--k", "8"],
    "order": ["--task", "order"],
    "gap": ["--task", "gap"],
    "both": ["--task", "both-not-either"],
    "two-step": ["--task", "two-step"],
    "beat": ["--task", "beat", "--period", "4"],
    "dist1": ["--task", "distract", "--distract", "1"],
    "dist2": ["--task", "distract", "--distract", "2"],
    "dist4": ["--task", "distract", "--distract", "4"],
    "dist8": ["--task", "distract", "--distract", "8"],
    "add": ["--task", "serial-add"],
    "majority": ["--task", "majority"],
    "place": ["--task", "place"],
}
GROUPS = {
    "hold": ["delay0", "delay1", "delay2", "delay5", "delay20", "delay2k8"],
    "cues": ["echo2", "echo8", "echo16", "echo32", "shuffled"],
    "steps": ["order", "gap", "both", "two-step"],
    "time": ["beat"],
    # what a lookup of the recent past cannot do: distractors in the gap, a computed held
    # value, cases never rewarded
    "beyond": ["dist1", "dist2", "dist4", "dist8", "add", "majority", "place"],
}


def seeds_of(text):
    a, _, b = text.partition("-")
    return list(range(int(a), int(b or a) + 1))


def read_sum(path):
    out = {}
    if not os.path.exists(path):
        return out
    for line in open(path).read().splitlines()[1:]:
        p = line.split(",")
        if len(p) == 3:
            out.setdefault(p[0], {})[p[1]] = float(p[2])
    return out


def main():
    tag, names, seeds = sys.argv[1], sys.argv[2].split(","), seeds_of(sys.argv[3])
    rows = []
    for n in names:
        rows += GROUPS.get(n, [n])
    rest = sys.argv[4:]
    steps = "40000"
    if "--steps" in rest:
        i = rest.index("--steps")
        steps = rest[i + 1]
        rest = rest[:i] + rest[i + 2:]
    window = "1000" if int(steps) >= 20000 else "200"
    outdir = os.path.join(ROOT, "out", "vm", "rows", tag)
    os.makedirs(outdir, exist_ok=True)
    jobs = []
    for r in rows:
        for s in seeds:
            csv = os.path.join(outdir, "%s-s%d.csv" % (r, s))
            if os.path.exists(csv[:-4] + "-sum.csv"):
                continue
            cmd = ["nice", "-n", "19", BLOBRUN] + BASE + ROWS[r] + [
                "--steps", steps, "--window", window, "--seed", str(s), "--out", csv,
                "--alive-ticks", "0", "--rhythm-ticks", "0"] + rest
            jobs.append((csv, cmd))

    def run(job):
        csv, cmd = job
        with open(csv[:-4] + ".log", "w") as f:
            f.write(" ".join(cmd) + "\n")
            f.flush()
            subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT)

    with ThreadPoolExecutor(2) as ex:
        list(ex.map(run, jobs))
    print("tag %s, seeds %s, %s steps, extra: %s" % (tag, sys.argv[3], steps, " ".join(rest)))
    print("%-10s %7s %6s %7s %7s   late score per seed (vm)" % ("row", "vm", "+-", "off",
                                                                 "coin"))
    for r in rows:
        v, o, c = [], [], []
        for s in seeds:
            d = read_sum(os.path.join(outdir, "%s-s%d-sum.csv" % (r, s)))
            v.append(d.get("blob", {}).get("late", float("nan")))
            o.append(d.get("learn_off", {}).get("late", float("nan")))
            c.append(d.get("coin", {}).get("late", float("nan")))
        sd = statistics.stdev(v) if len(v) > 1 else 0.0
        print("%-10s %7.3f %6.3f %7.3f %7.3f   %s" % (
            r, statistics.mean(v), sd, statistics.mean(o), statistics.mean(c),
            " ".join("%.2f" % x for x in v)))


main()
