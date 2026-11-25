"""The runner and tables of scripts/timerule.py (idea 166): lives, grids of lives, tables."""

import csv
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, os.environ.get("TIMERULE_BUILD", "build"))
RUN = os.path.join(BUILD, "blobrun")
PROBE = os.path.join(BUILD, "blobprobe")

READOUT = "--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1"
TW = "--g.tile_winners 4"
COMP = "--g.compete_on 1 --g.compete_shift 5"
CAP = "--g.threshold_cap 1000"
# the output-gap job's readout (docs/results-2026-11-25-output-gap.md)
GOOD = "--g.readout_evidence 2 --g.readout_good 2 --g.margin_percent 20 --g.margin_win 2"

# the two sheets the contradiction came from. suite: scripts/suite.py's standard life with
# the scoreboard's blob-readout readout (compete2's time rows); pred: scripts/predict.py's
# one sheet for every arm. a setting is these six things; the walk changes one at a time
SUITE = {"side": 3, "lattice": "std", "spread": 128, "delay": 2, "offtw": 1, "steps": 40000}
PRED = {"side": 2, "lattice": "pred", "spread": 64, "delay": 4, "offtw": 0, "steps": 20000}
LATTICE = {"std": "", "pred": "--g.sensor_w 64 --g.motor_stride 1"}

# arms: what learns beside the readout. off is the born sheet (the tile's winners only if
# the setting's offtw says so); on is idea 158's B5; churn moves toward random sources
ARMS = {"off": "", "on": TW + " " + COMP, "churn": TW + " " + COMP + " --g.compete_churn 1",
        "offcap": CAP, "oncap": TW + " " + COMP + " " + CAP,
        "goodoff": GOOD, "goodon": TW + " " + COMP + " " + GOOD}


def setting_flags(st, arm):
    flags = "--ticks 20 --sensor-side %d --motor-side 3 --motor-spread %d %s %s" % (
        st["side"], st["spread"], LATTICE[st["lattice"]], READOUT)
    extra = ARMS[arm]
    if arm in ("off", "offcap", "goodoff") and st["offtw"]:
        extra = TW + " " + extra
    return (flags + " " + extra).split()


def setting_key(st):
    return "side%d-%s-sp%d-tw%d-st%d" % (st["side"], st["lattice"], st["spread"], st["offtw"],
                                         st["steps"])


def seeds_of(text):
    out = []
    for part in text.split(","):
        if "-" in part:
            a, b = part.split("-")
            out += list(range(int(a), int(b) + 1))
        elif part:
            out.append(int(part))
    return out


def mean(v):
    got = [x for x in v if x is not None]
    return sum(got) / len(got) if got else None


def fmt(v, d=3):
    return "  -  " if v is None else ("%." + str(d) + "f") % v


def read_sum(path):
    d = {}
    if os.path.exists(path):
        for r in csv.reader(open(path)):
            if len(r) == 3 and r[0] != "driver":
                d[r[0], r[1]] = float(r[2])
    return d


# ---- running ----

def run_job(job):
    """job: (command, log path, done path). a finished run (its done file there) is kept"""
    cmd, log, done = job
    if os.path.exists(done):
        return 0
    os.makedirs(os.path.dirname(log), exist_ok=True)
    with open(log + ".part", "w") as f:
        f.write(" ".join(cmd) + "\n")
        f.flush()
        code = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT).returncode
    os.replace(log + ".part", log)
    return code


def run_all(jobs, opt):
    if opt["table_only"]:
        return
    todo = [j for j in jobs if not os.path.exists(j[2])]
    print("timerule: %d runs (%d already done), %d at a time" % (
        len(todo), len(jobs) - len(todo), opt["jobs"]), flush=True)
    with ThreadPoolExecutor(max_workers=opt["jobs"]) as pool:
        bad = [j[1] for j, c in zip(todo, pool.map(run_job, todo)) if c != 0]
    if bad:
        print("FAILED: " + " ".join(os.path.relpath(b, ROOT) for b in bad))


def scaled(steps, opt):
    return max(steps // 10, 1000) if opt["quick"] else steps


def life_job(folder, name, task_args, st, arm, seed, opt, baselines=False):
    """one blobrun life; returns the job and the -sum.csv it writes"""
    steps = scaled(st["steps"], opt)
    window = 200 if opt["quick"] else 1000
    base = os.path.join(folder, "%s-s%d" % (name, seed))
    cmd = (["nice", "-n", "19", RUN] + task_args + setting_flags(st, arm) +
           ["--steps", str(steps), "--window", str(window), "--seed", str(seed),
            "--task-seed", str(seed), "--out", base + ".csv"] +
           ([] if baselines else ["--no-baselines"]) + opt["extra"])
    return (cmd, base + ".txt", base + "-sum.csv"), base + "-sum.csv"


def late_and_rate(path):
    d = read_sum(path)
    return d.get(("blob", "late")), d.get(("blob", "rate_per_1024")), d.get(("learn_off", "late"))


def delayed(d):
    return ["--task", "delayed-echo", "--delay", str(d)]


# ---- a grid of lives: rows of (label, task args, setting), columns of arms ----

def grid(folder, rows, arms, opt, baselines=False, by_args=False):
    """runs every row x arm x seed; returns {(label, arm): [(late, rate, off) per seed]}.
    by_args: runs are named by their flags, so another table with the same life reuses it"""
    jobs, where = [], {}
    for label, task_args, st in rows:
        for arm in arms:
            what = label_key(" ".join(task_args).replace("--", "")) if by_args else label_key(label)
            name = "%s-%s-%s" % (what, setting_key(st), arm)
            for s in opt["seeds"]:
                job, path = life_job(folder, name, task_args, st, arm, s, opt, baselines)
                # --arms: launch only these (the table still shows whatever is there)
                if not opt.get("arms") or arm in opt["arms"]:
                    jobs.append(job)
                where.setdefault((label, arm), []).append(path)
    run_all(jobs, opt)
    return {k: [late_and_rate(p) for p in v] for k, v in where.items()}


def label_key(label):
    return "".join(c if c.isalnum() else "_" for c in label)


def grid_table(title, rows, arms, res, opt, pair=("on", "off")):
    """per row: each arm's mean late score and rate; on minus off with seeds above"""
    lines = [title, "seeds %s; late score (mean of the last tenth), rate in spikes per 1024 "
             "neuron-ticks in brackets" % ",".join(str(s) for s in opt["seeds"]), ""]
    head = "| row | " + " | ".join(arms) + " | %s - %s | seeds up |" % pair
    lines += [head, "|" + "---|" * (len(arms) + 3)]
    for label, _, _ in rows:
        cells = []
        for arm in arms:
            v = res[label, arm]
            cells.append("%s (%s)" % (fmt(mean([x[0] for x in v])), fmt(mean([x[1] for x in v]), 0)))
        a, b = res.get((label, pair[0])), res.get((label, pair[1]))
        d, up = "", ""
        if a and b:
            diffs = [x[0] - y[0] for x, y in zip(a, b) if x[0] is not None and y[0] is not None]
            if diffs:
                d = "%+.3f" % (sum(diffs) / len(diffs))
                up = "%d/%d" % (sum(x > 0 for x in diffs), len(diffs))
        lines.append("| %s | %s | %s | %s |" % (label, " | ".join(cells), d, up))
    return "\n".join(lines)


def per_seed_table(title, rows, arms, res, opt):
    lines = [title, "", "| row | arm | " + " | ".join(str(s) for s in opt["seeds"]) +
             " | mean | rate | learning off |", "|" + "---|" * (len(opt["seeds"]) + 5)]
    for label, _, _ in rows:
        for arm in arms:
            v = res[label, arm]
            lines.append("| %s | %s | %s | %s | %s | %s |" % (
                label, arm, " | ".join(fmt(x[0], 2) for x in v), fmt(mean([x[0] for x in v])),
                fmt(mean([x[1] for x in v]), 1), fmt(mean([x[2] for x in v]))))
    return "\n".join(lines)
