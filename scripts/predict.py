#!/usr/bin/env python3
"""Idea 162, every line is a target (docs/results-2026-11-25-predict.md). See scripts/predict.sh."""

import os
import statistics
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, os.environ.get("PREDICT_BUILD", "build"))
RUN = os.path.join(BUILD, "blobrun")
PROBE = os.path.join(BUILD, "blobprobe")

# one sheet for every arm: the default 64 x 64 sheet with the sensor lattice across the top
# half (room for the error lines) and every cell of the bottom-left quadrant a motor cell
# (room for the predictor groups near the sensors); the working output rule
PLUG = "--ticks 20 --sensor-side 2 --motor-side 3 --motor-spread 64 --window 1000".split()
GENOME = ("--g.sensor_w 64 --g.motor_stride 1 --g.learn_mode 5 --g.readout_evidence 1 "
          "--g.readout_shift 4 --g.readout_balance 1").split()
PRED = "--predict 1,2,4,8 --predict-shift 2".split()
ERR = "--predict-errors 1".split()
PRESS = "--predict-press 1".split()
COMPETE = "--g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5".split()

WORLDS = {
    "catch": [],
    "track": [],
    "forage": [],
    "cartpole": "--cart-reward lean --number-code place".split(),
    "pendulum": "--number-code place".split(),
    "mountain-car": "--number-code place".split(),
    "chain": [],
    "delayed-echo": "--delay 4".split(),
    "beat": [],
    "predict-next": "--k 4 --markov-order 1".split(),
    "echo": [],
}
# the read's own world settings where they differ: predict-next asks for the next symbol,
# which at markov order 1 is on the lines (the symbol shown fixes it 7 times in 8); at
# order 2 it needs the symbol before too
READ_FLAGS = {"predict-next": "--k 4 --markov-order 2".split()}
# worlds the inside read asks something of (probe/predict_labels.hpp)
READ_WORLDS = ["catch", "track", "cartpole", "chain", "delayed-echo", "beat", "predict-next",
               "echo"]

# blobrun arms: flags, and whether the learning-off twin (and coin) run too
TASK_ARMS = {
    "off": ([], True),
    "pred": (PRED, True),
    "pred_err": (PRED + ERR, False),
    "pred_press": (PRED + PRESS, False),
    "press": (PRESS, False),
    "compete": (COMPETE, False),
    "pred_compete": (PRED + COMPETE, False),
}
# blobprobe predict arms: flags, life (False: born), shuffled
READ_ARMS = {
    "born": ([], False, False),
    "task": ([], True, False),
    "pred": (PRED, True, False),
    "pred_err": (PRED + ERR, True, False),
    "shuffled": (PRED, True, True),
    "compete": (COMPETE, True, False),
    "pred_compete": (PRED + COMPETE, True, False),
    "pred_err_compete": (PRED + ERR + COMPETE, True, False),
}


def usage():
    sys.stderr.write(open(os.path.join(ROOT, "scripts", "predict.sh")).read().split("set -")[0])
    sys.exit(1)


def seeds_of(text):
    out = []
    for part in text.split(","):
        if "-" in part:
            a, b = part.split("-")
            out += list(range(int(a), int(b) + 1))
        elif part:
            out.append(int(part))
    return out


def parse(argv):
    if not argv or argv[0].startswith("-"):
        usage()
    opt = {"name": argv[0], "seeds": [5, 6, 7, 8, 9, 10, 11, 12], "jobs": 3, "steps": 20000,
           "trials": 3000, "only": ["task", "read", "old", "future"], "worlds": list(WORLDS),
           "table_only": False, "extra": []}
    i = 1
    while i < len(argv):
        a = argv[i]
        val = argv[i + 1] if i + 1 < len(argv) else None
        if a == "--quick":
            opt.update(seeds=[1], steps=2000, trials=600)
        elif a == "--table-only":
            opt["table_only"] = True
        elif a == "--seeds":
            opt["seeds"] = seeds_of(val)
            i += 1
        elif a in ("--jobs", "--steps", "--trials"):
            opt[a[2:]] = int(val)
            i += 1
        elif a == "--only":
            opt["only"] = val.split(",")
            i += 1
        elif a == "--worlds":
            opt["worlds"] = val.split(",")
            i += 1
        else:
            opt["extra"].append(a)
        i += 1
    return opt


def task_job(world, arm, seed, opt, folder):
    flags, baselines = TASK_ARMS[arm]
    base = os.path.join(folder, f"{world}-{arm}-s{seed}")
    cmd = (["nice", "-n", "19", RUN, "--task", world] + PLUG + WORLDS[world] + GENOME + flags +
           ["--steps", str(opt["steps"]), "--seed", str(seed), "--out", base + ".csv"])
    if not baselines:
        cmd.append("--no-baselines")
    return cmd + opt["extra"], base + ".txt", base + "-sum.csv"


def old_job(seed, opt, folder):
    # the old guess (blob.predicted, the design rule, learn mode 0) on the same plug
    base = os.path.join(folder, f"predict-next-oldguess-s{seed}")
    cmd = (["nice", "-n", "19", RUN, "--task", "predict-next"] + PLUG + WORLDS["predict-next"] +
           ["--g.sensor_w", "64", "--g.motor_stride", "1", "--steps", str(opt["steps"]),
            "--seed", str(seed), "--out", base + ".csv"])
    return cmd + opt["extra"], base + ".txt", base + "-sum.csv"


def read_job(world, arm, seed, opt, folder):
    flags, lived, shuffled = READ_ARMS[arm]
    base = os.path.join(folder, f"read-{world}-{arm}-s{seed}")
    cmd = (["nice", "-n", "19", PROBE, "predict", "--task", world] + PLUG +
           READ_FLAGS.get(world, WORLDS[world]) +
           GENOME + flags + ["--seed", str(seed), "--trials", str(opt["trials"]),
                             "--life", str(opt["steps"] if lived else 0), "--label", arm,
                             "--out", base + ".csv"])
    if shuffled:
        cmd += ["--shuffle", "1"]
    return cmd + opt["extra"], base + ".txt", base + ".csv"


# the ceiling (blobprobe predict --future): how much of each line's future a fresh linear
# reader finds in the frozen sheet, born and after a life with the predictor cells
FUTURE_ARMS = {"born": ([], False), "pred": (PRED, True)}


def future_job(world, arm, seed, opt, folder):
    flags, lived = FUTURE_ARMS[arm]
    base = os.path.join(folder, f"future-{world}-{arm}-s{seed}")
    cmd = (["nice", "-n", "19", PROBE, "predict", "--task", world] + PLUG + WORLDS[world] +
           GENOME + flags + ["--seed", str(seed), "--trials", str(opt["trials"]),
                             "--life", str(opt["steps"] if lived else 0), "--label", arm,
                             "--future", "1,8", "--out", base + ".csv"])
    return cmd + opt["extra"], base + ".txt", base + ".csv"


def run(job):
    cmd, log, done = job
    if os.path.exists(done) and os.path.exists(log):
        return
    if os.path.exists(done):
        os.remove(done)  # a half-written csv from a run that was stopped
    with open(log + ".part", "w") as f:
        f.write(" ".join(cmd[3:]) + "\n")
        f.flush()
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT)
    os.replace(log + ".part", log)


# --- reading results ---

def read_sum(path):
    out = {}
    if os.path.exists(path):
        for line in open(path).read().splitlines():
            p = line.split(",")
            if len(p) == 3:
                try:
                    out[(p[0], p[1])] = float(p[2])
                except ValueError:
                    pass
    return out


def read_csv(path):
    out = {}
    if os.path.exists(path):
        for line in open(path).read().splitlines():
            p = line.split(",")
            if len(p) == 4:
                out[(p[1], p[2])] = float(p[3])
    return out


def mean(xs):
    xs = [x for x in xs if x is not None]
    return statistics.mean(xs) if xs else None


def sd(xs):
    xs = [x for x in xs if x is not None]
    return statistics.stdev(xs) if len(xs) > 1 else 0.0


def fmt(x, w=6, d=3):
    return f"{x:{w}.{d}f}" if x is not None else " " * (w - 1) + "-"


def tables(opt, folder):
    seeds, worlds = opt["seeds"], opt["worlds"]
    out = []
    n = len(seeds)
    out.append(f"# predict {opt['name']}: seeds {','.join(map(str, seeds))} ({n}), "
               f"{opt['steps']} steps per life\n")
    sums = {}
    for w in list(worlds) + ["predict-next"]:
        for arm in list(TASK_ARMS) + ["oldguess"]:
            for s in seeds:
                sums[(w, arm, s)] = read_sum(os.path.join(folder, f"{w}-{arm}-s{s}-sum.csv"))

    def got(w, arm, who, key):
        return [sums[(w, arm, s)].get((who, key)) for s in seeds]

    # 1. prediction skill
    out.append("## 1. Prediction skill (share of line-steps right, last quarter of the life)\n")
    out.append("pred: the predictor cells; same: the line h steps before; common: each line's "
               "more common value; twin: the same cells on the learning-off twin. bal: the "
               "mean of the right shares on on- and off-line-steps. chg: right share on "
               "line-steps that changed. `*` marks pred above all three baselines (mean over "
               "seeds); `/n` how many seeds it is above all three.\n")
    out.append("| world | h | pred | same | common | twin | | bal pred | bal same | bal common "
               "| bal twin | chg pred |")
    out.append("|---|---|---|---|---|---|---|---|---|---|---|---|")
    csv = ["world,h,pred,same,common,twin,bal_pred,bal_same,bal_common,bal_twin,chg_pred,"
           "seeds_above"]
    for w in worlds:
        for h in (1, 2, 4, 8):
            k = f"pred_h{h}_"
            p = got(w, "pred", "blob", k + "right")
            same = got(w, "pred", "blob", k + "same")
            com = got(w, "pred", "blob", k + "common")
            twin = got(w, "pred", "learn_off", k + "right")
            if mean(p) is None:
                continue
            above = sum(1 for a, b, c, d in zip(p, same, com, twin)
                        if None not in (a, b, c, d) and a > max(b, c, d))
            row = [mean(p), mean(same), mean(com), mean(twin),
                   mean(got(w, "pred", "blob", k + "balanced")),
                   mean(got(w, "pred", "blob", k + "same_balanced")),
                   mean(got(w, "pred", "blob", k + "common_balanced")),
                   mean(got(w, "pred", "learn_off", k + "balanced")),
                   mean(got(w, "pred", "blob", k + "changed_right"))]
            star = "*" if row[0] > max(row[1], row[2], row[3]) else ""
            out.append(f"| {w} | {h} | {fmt(row[0])}{star} | {fmt(row[1])} | {fmt(row[2])} | "
                       f"{fmt(row[3])} | /{above} | {fmt(row[4])} | {fmt(row[5])} | "
                       f"{fmt(row[6])} | {fmt(row[7])} | {fmt(row[8])} |")
            csv.append(",".join([w, str(h)] + [f"{x:.5f}" if x is not None else "" for x in row]
                                + [str(above)]))
    open(os.path.join(ROOT, "out", "predict", opt["name"] + "-skill.csv"), "w").write(
        "\n".join(csv) + "\n")

    # 2. task score
    out.append("\n## 2. Task score, late (last tenth), mean and sd over seeds\n")
    arms = list(TASK_ARMS)
    out.append("| world | " + " | ".join(arms) + " | twin |")
    out.append("|---|" + "---|" * (len(arms) + 1))
    csv = ["world,arm,mean,sd,n"]
    for w in worlds:
        cells = []
        for arm in arms:
            v = got(w, arm, "blob", "late")
            cells.append(f"{fmt(mean(v), 5, 3)} ±{fmt(sd(v), 4, 3)}" if mean(v) is not None
                         else "-")
            if mean(v) is not None:
                csv.append(f"{w},{arm},{mean(v):.5f},{sd(v):.5f},"
                           f"{len([x for x in v if x is not None])}")
        twin = got(w, "off", "learn_off", "late")
        out.append(f"| {w} | " + " | ".join(cells) + f" | {fmt(mean(twin), 5, 3)} |")
    open(os.path.join(ROOT, "out", "predict", opt["name"] + "-task.csv"), "w").write(
        "\n".join(csv) + "\n")
    # paired differences against off
    out.append("\nPaired difference against off (mean over seeds, and seeds where it is "
               "higher / lower):\n")
    out.append("| world | " + " | ".join(a for a in arms if a != "off") + " |")
    out.append("|---|" + "---|" * (len(arms) - 1))
    for w in worlds:
        off = got(w, "off", "blob", "late")
        cells = []
        for arm in arms[1:]:
            v = got(w, arm, "blob", "late")
            d = [b - a for a, b in zip(off, v) if a is not None and b is not None]
            if not d:
                cells.append("-")
                continue
            cells.append(f"{mean(d):+.3f} ({sum(x > 0 for x in d)}/{sum(x < 0 for x in d)})")
        out.append(f"| {w} | " + " | ".join(cells) + " |")

    # 3. cost
    out.append("\n## 3. Cost: ticks per second of the lived sheet (this PC, rough: the "
               "temperature script pins jobs)\n")
    out.append("| world | " + " | ".join(arms) + " |")
    out.append("|---|" + "---|" * len(arms))
    for w in worlds:
        cells = [fmt(mean(got(w, arm, "blob", "ticks_per_s")), 7, 0) for arm in arms]
        out.append(f"| {w} | " + " | ".join(cells) + " |")

    # 4. the old guess on predict-next
    if "predict-next" in worlds:
        out.append("\n## 4. predict-next: the old guess beside the predictor cells\n")
        o = got("predict-next", "oldguess", "blob", "late")
        ot = got("predict-next", "oldguess", "learn_off", "late")
        out.append(f"Old guess (blob.predicted, learn mode 0, share of sensor cells right on a "
                   f"step's first tick): lived {fmt(mean(o))}, twin {fmt(mean(ot))}.  ")
        p = got("predict-next", "pred", "blob", "pred_h1_right")
        out.append(f"Predictor cells (h=1, share of lines right over the step): "
                   f"{fmt(mean(p))}, same {fmt(mean(got('predict-next', 'pred', 'blob', 'pred_h1_same')))}"
                   f", common {fmt(mean(got('predict-next', 'pred', 'blob', 'pred_h1_common')))}"
                   f", twin {fmt(mean(got('predict-next', 'pred', 'learn_off', 'pred_h1_right')))}.")

    # 5. the inside read
    out.append("\n## 5. The inside read: a fresh linear reader on the frozen sheet\n")
    out.append("Share right on the test quarter. sheet: every non-sensor cell; hidden: no sensor "
               "or motor cell. now / two: the same reader on the task's lines of this step / of "
               "this and the last step. all lines: on every line the arm's sheet sees this step "
               "(good, bad, and the press and error lines where the arm has them). maj: the most common answer. rate: spikes per 1,000 "
               "cell-ticks of the non-sensor cells in the read.\n")
    rarms = list(READ_ARMS)
    out.append("| world | question | " + " | ".join(rarms) + " | now | two | maj |")
    out.append("|---|---|" + "---|" * (len(rarms) + 3))
    csv = ["world,question,arm,sheet,sheet_sd,hidden,hidden_sd,rate_sheet,rate_hidden,n"]
    rates = {}
    for w in READ_WORLDS:
        if w not in worlds:
            continue
        data = {(arm, s): read_csv(os.path.join(folder, f"read-{w}-{arm}-s{s}.csv"))
                for arm in rarms for s in seeds}
        questions = sorted({q for d in data.values() for (q, _) in d})
        for q in questions:
            cells = []
            for arm in rarms:
                sv = [data[(arm, s)].get((q, "sheet")) for s in seeds]
                hv = [data[(arm, s)].get((q, "hidden")) for s in seeds]
                rv = [data[(arm, s)].get((q, "rate_sheet")) for s in seeds]
                rh = [data[(arm, s)].get((q, "rate_hidden")) for s in seeds]
                cells.append(f"{fmt(mean(sv), 5)} / {fmt(mean(hv), 5)}" if mean(sv) is not None
                             else "-")
                rates[(w, arm)] = mean(rv)
                if mean(sv) is not None:
                    csv.append(f"{w},{q},{arm},{mean(sv):.5f},{sd(sv):.5f},{mean(hv):.5f},"
                               f"{sd(hv):.5f},{mean(rv):.3f},{mean(rh):.3f},"
                               f"{len([x for x in sv if x is not None])}")
            base = [data[(a, s)] for a in rarms for s in seeds if data[(a, s)]]
            now = mean([d.get((q, "lines_now")) for d in base])
            # every line of the step: differs by arm (good, bad, the extra lines)
            lall = {a: mean([data[(a, s)].get((q, "lines_all")) for s in seeds]) for a in rarms}
            two = mean([d.get((q, "lines_two")) for d in base])
            maj = mean([d.get((q, "majority")) for d in base])
            out.append(f"| {w} | {q} | " + " | ".join(cells) +
                       f" | {fmt(now, 5)} | {fmt(two, 5)} | {fmt(maj, 5)} |")
            out.append(f"| | all lines | " + " | ".join(fmt(lall[a], 5) for a in rarms) +
                       " | | | |")
    out.append("\nEach cell: sheet / hidden. Firing rates (sheet, spikes per 1,000 cell-ticks):\n")
    out.append("| world | " + " | ".join(rarms) + " |")
    out.append("|---|" + "---|" * len(rarms))
    for w in READ_WORLDS:
        if w in worlds:
            out.append(f"| {w} | " + " | ".join(fmt(rates.get((w, a)), 5, 1) for a in rarms) +
                       " |")
    open(os.path.join(ROOT, "out", "predict", opt["name"] + "-read.csv"), "w").write(
        "\n".join(csv) + "\n")
    # 6. the ceiling
    rows = []
    for w in worlds:
        for h in (1, 8):
            vals = {}
            for arm in FUTURE_ARMS:
                d = [read_csv(os.path.join(folder, f"future-{w}-{arm}-s{s}.csv")) for s in seeds]
                for k in ("sheet", "lines", "same", "common"):
                    vals[(arm, k)] = mean([x.get(("future", f"h{h}_{k}")) for x in d])
            if vals[("pred", "sheet")] is None and vals[("born", "sheet")] is None:
                continue
            p = mean(got(w, "pred", "blob", f"pred_h{h}_right"))
            rows.append(f"| {w} | {h} | {fmt(p)} | {fmt(vals[('born', 'sheet')])} | "
                        f"{fmt(vals[('pred', 'sheet')])} | {fmt(vals[('pred', 'lines')])} | "
                        f"{fmt(vals[('pred', 'same')])} | {fmt(vals[('pred', 'common')])} |")
    if rows:
        out.append("\n## 6. The ceiling: a fresh linear reader of each line's future\n")
        out.append("Share of line-steps right on the read's test quarter (moves from the coin, "
                   "so not the life's own steps). cells: the predictor cells in the life "
                   "(section 1); born / lived: a ridge reader per line of the frozen sheet's "
                   "counts, as born and after the pred life; lines: the same reader on this "
                   "step's lines; same, common: on the read's rows.\n")
        out.append("| world | h | cells (life) | born sheet | lived sheet | lines | same | "
                   "common |")
        out.append("|---|---|---|---|---|---|---|---|")
        out += rows
    text = "\n".join(out) + "\n"
    open(os.path.join(ROOT, "out", "predict", opt["name"] + ".md"), "w").write(text)
    print(text)


def main():
    opt = parse(sys.argv[1:])
    folder = os.path.join(ROOT, "out", "predict", opt["name"])
    os.makedirs(folder, exist_ok=True)
    if not opt["table_only"]:
        jobs = []
        for s in opt["seeds"]:
            for w in opt["worlds"]:
                if "task" in opt["only"]:
                    jobs += [task_job(w, arm, s, opt, folder) for arm in TASK_ARMS]
                if "read" in opt["only"] and w in READ_WORLDS:
                    jobs += [read_job(w, arm, s, opt, folder) for arm in READ_ARMS]
                if "future" in opt["only"]:
                    jobs += [future_job(w, arm, s, opt, folder) for arm in FUTURE_ARMS]
            if "old" in opt["only"] and "predict-next" in opt["worlds"]:
                jobs.append(old_job(s, opt, folder))
        with ThreadPoolExecutor(max_workers=opt["jobs"]) as pool:
            list(pool.map(run, jobs))
    tables(opt, folder)


if __name__ == "__main__":
    main()
