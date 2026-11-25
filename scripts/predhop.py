#!/usr/bin/env python3
"""Idea 164, prediction errors down the one-hop wires. See scripts/predhop.sh."""

import os
import statistics
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, os.environ.get("PREDICT_BUILD", "build"))
PROBE = os.path.join(BUILD, "blobprobe")

# the prediction battery's sheet (scripts/predict.py): sensor lattice across the top half,
# the bottom-left quadrant all motor cells, the working output rule, predictor cells at
# 1, 2, 4, 8 steps
PLUG = "--ticks 20 --sensor-side 2 --motor-side 3 --motor-spread 64 --window 1000".split()
GENOME = ("--g.sensor_w 64 --g.motor_stride 1 --g.learn_mode 5 --g.readout_evidence 1 "
          "--g.readout_shift 4 --g.readout_balance 1").split()
PRED = "--predict 1,2,4,8 --predict-shift 2".split()
WORLD_GENOME = {"catch": "--g.motor_w 48".split()}
# the one-hop rule as finding 45 and 46 say for many verdicts: slow, near-not-fired marks
HOP = "--g.hop_on 1 --g.hop_shift 7 --g.hop_fired 0".split()
FROM_PRED = HOP + "--g.hop_teach 0 --predict-hop 1".split()
COMPETE = "--g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5".split()
# track is dropped (it locks against a wall on this sheet; predict doc). predict-next at
# markov order 2 in the life and the read: the present symbol does not fix the next
WORLDS = {
    "catch": [],
    "forage": [],
    "cartpole": "--cart-reward lean --number-code place".split(),
    "pendulum": "--number-code place".split(),
    "mountain-car": "--number-code place".split(),
    "chain": [],
    "delayed-echo": "--delay 4".split(),
    "beat": [],
    "predict-next": "--k 4 --markov-order 2".split(),
    "echo": [],
}

# arms: flags, lived, time-shuffled life. every arm has the predictor cells
ARMS = {
    "born": ([], False, False),
    "pred": ([], True, False),                         # the rule off: only motor cells learn
    "hop": (FROM_PRED, True, False),                   # prediction errors only, no reward inside
    "coin": (FROM_PRED + ["--g.hop_coin", "1"], True, False),  # the same errors, random lines
    "shuf": (FROM_PRED, True, True),                   # the hop life on time-shuffled frames
    "buttons": (HOP + ["--g.hop_teach", "2"], True, False),    # the buttons' errors only
    "both": (HOP + "--g.hop_teach 2 --predict-hop 1".split(), True, False),
    "local": (FROM_PRED, True, False),                 # + --predict-hop-reach (set in parse)
    "compete": (FROM_PRED + COMPETE, True, False),     # with competitive learning (finding 50)
}
LIVED = [a for a in ARMS if a != "born"]


def usage():
    sys.stderr.write(open(os.path.join(ROOT, "scripts", "predhop.sh")).read().split("set -")[0])
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
           "trials": 3000, "worlds": list(WORLDS), "arms": list(ARMS), "reach": 4,
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
        elif a in ("--jobs", "--steps", "--trials", "--reach"):
            opt[a[2:]] = int(val)
            i += 1
        elif a in ("--worlds", "--arms"):
            opt[a[2:]] = val.split(",")
            i += 1
        else:
            opt["extra"].append(a)
        i += 1
    return opt


def job(world, arm, seed, opt, folder):
    flags, lived, shuffled = ARMS[arm]
    if arm == "local":
        flags = flags + ["--predict-hop-reach", str(opt["reach"])]
    base = os.path.join(folder, f"{world}-{arm}-s{seed}")
    cmd = (["nice", "-n", "19", PROBE, "predict", "--task", world] + PLUG + WORLDS[world] +
           GENOME + WORLD_GENOME.get(world, []) + PRED + flags +
           ["--seed", str(seed), "--trials", str(opt["trials"]),
            "--life", str(opt["steps"] if lived else 0), "--label", arm, "--future", "1,8",
            "--out", base + ".csv"])
    if shuffled:
        cmd += ["--shuffle", "1"]
    return cmd + opt["extra"], base + ".txt", base + ".csv"


def run(j):
    cmd, log, done = j
    if os.path.exists(done) and os.path.exists(log):
        return
    if os.path.exists(done):
        os.remove(done)  # a half-written csv from a run that was stopped
    with open(log + ".part", "w") as f:
        f.write(" ".join(cmd[3:]) + "\n")
        f.flush()
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT)
    os.replace(log + ".part", log)


# --- tables ---

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


def fmt(x, w=6, d=3):
    return f"{x:{w}.{d}f}" if x is not None else " " * (w - 1) + "-"


def diff(a, b):
    """paired a - b over seeds: mean (seeds above / below), and the list"""
    d = [x - y for x, y in zip(a, b) if x is not None and y is not None]
    if not d:
        return "-", []
    return f"{mean(d):+.3f} ({sum(v > 0 for v in d)}/{sum(v < 0 for v in d)})", d


def tables(opt, folder):
    seeds, worlds = opt["seeds"], opt["worlds"]
    arms = [a for a in ARMS if a in opt["arms"]]
    data = {(w, a, s): read_csv(os.path.join(folder, f"{w}-{a}-s{s}.csv"))
            for w in worlds for a in arms for s in seeds}

    def got(w, a, q, k):
        return [data[(w, a, s)].get((q, k)) for s in seeds]

    out = [f"# predhop {opt['name']}: seeds {','.join(map(str, seeds))}, {opt['steps']}-step "
           f"lives, {opt['trials']} read steps; extra flags: {' '.join(opt['extra']) or 'none'}; "
           f"local reach {opt['reach']}\n"]
    csv = ["table,world,key,arm,seed,value"]
    for (w, a, s), d in data.items():
        for (q, k), v in d.items():
            csv.append(f"{q},{w},{k},{a},{s},{v:.6g}")
    open(os.path.join(ROOT, "out", "predhop", opt["name"] + ".csv"), "w").write(
        "\n".join(csv) + "\n")

    # 1. the future
    out.append("## 1. The future: a fresh ridge reader of every predicted line h steps on\n")
    out.append("Share of line-steps right on the read's test quarter (moves from a coin). "
               "lines: the reader of this step's lines (the same rows in every arm); each arm: "
               "the reader of the hidden cells only (no sensor or motor cell) / of the hidden "
               "cells and the lines together.\n")
    out.append("| world | h | lines | " + " | ".join(arms) + " |")
    out.append("|---|---|---|" + "---|" * len(arms))
    for w in worlds:
        for h in (1, 8):
            lines = mean(got(w, "born" if "born" in arms else arms[0], "future", f"h{h}_lines"))
            cells = [f"{fmt(mean(got(w, a, 'future', f'h{h}_hidden')))} / "
                     f"{fmt(mean(got(w, a, 'future', f'h{h}_both')))}" for a in arms]
            out.append(f"| {w} | {h} | {fmt(lines)} | " + " | ".join(cells) + " |")
    out.append("\nPaired over seeds, hidden-cell reader of the future (mean, seeds above / "
               "below): lived minus born, minus the rule-off life (pred), minus its coin "
               "control, minus its time-shuffled life; hidden minus lines.\n")
    pairs = [(x, y) for x, y in (("hop", "born"), ("hop", "pred"), ("hop", "coin"),
                                 ("hop", "shuf"), ("both", "buttons"), ("buttons", "pred"),
                                 ("local", "hop"), ("compete", "hop"))
             if x in arms and y in arms]
    out.append("| world | h | " + " | ".join(f"{x} - {y}" for x, y in pairs) +
               " | born: hidden - lines | hop: hidden - lines | hop: both - lines |")
    out.append("|---|---|" + "---|" * (len(pairs) + 3))
    for w in worlds:
        for h in (1, 8):
            k = f"h{h}_hidden"
            cells = [diff(got(w, x, "future", k), got(w, y, "future", k))[0] for x, y in pairs]
            for a, key in (("born", k), ("hop", k), ("hop", f"h{h}_both")):
                cells.append(diff(got(w, a, "future", key), got(w, a, "future", f"h{h}_lines"))[0]
                             if a in arms else "-")
            out.append(f"| {w} | {h} | " + " | ".join(cells) + " |")
    if "hop" in arms and "born" in arms:
        out.append("\nPer seed, hop minus born (hidden cells), seeds " +
                   ",".join(map(str, seeds)) + ":\n")
        for w in worlds:
            for h in (1, 8):
                k = f"h{h}_hidden"
                _, d = diff(got(w, "hop", "future", k), got(w, "born", "future", k))
                out.append(f"- {w} h={h}: " + " ".join(f"{v:+.3f}" for v in d))

    # 2. the inside read
    out.append("\n## 2. The inside read: hidden cells only, a fresh linear reader\n")
    out.append("Each cell: share right / hidden firing rate (spikes per 1,000 cell-ticks) while "
               "read. now: the reader of this step's task lines; maj: the most common answer.\n")
    out.append("| world | question | " + " | ".join(arms) + " | now | maj |")
    out.append("|---|---|" + "---|" * (len(arms) + 2))
    qrows = []
    for w in worlds:
        qs = sorted({q for a in arms for s in seeds for (q, k) in data[(w, a, s)]
                     if q not in ("future", "life")})
        for q in qs:
            qrows.append((w, q))
            cells = [f"{fmt(mean(got(w, a, q, 'hidden')))} / {fmt(mean(got(w, a, q, 'rate_hidden')), 4, 1)}"
                     for a in arms]
            out.append(f"| {w} | {q} | " + " | ".join(cells) + f" | "
                       f"{fmt(mean(got(w, arms[0], q, 'lines_now')))} | "
                       f"{fmt(mean(got(w, arms[0], q, 'majority')))} |")
    out.append("\nPaired, hidden cells (mean, seeds above / below):\n")
    out.append("| world | question | " + " | ".join(f"{x} - {y}" for x, y in pairs) + " |")
    out.append("|---|---|" + "---|" * len(pairs))
    for w, q in qrows:
        cells = [diff(got(w, x, q, "hidden"), got(w, y, q, "hidden"))[0] for x, y in pairs]
        out.append(f"| {w} | {q} | " + " | ".join(cells) + " |")

    # 3. the predictor cells' own skill, the task, the cost
    # the shuffled arm's life is the hop arm's life (its skill would be the replay's)
    life = [a for a in arms if a not in ("born", "shuf")]
    for title, key, note in (
            ("3. The predictor cells' skill in the life, h=1 (share of line-steps right, last "
             "quarter)", "pred_h1_right", ""),
            ("3b. The same, balanced (mean of right on on- and off-line-steps), h=1",
             "pred_h1_balanced", ""),
            ("3c. The same, h=8", "pred_h8_right", ""),
            ("4. The task: late score (last tenth of the life's windows)", "task_late", ""),
            ("5. Cost: ticks per second of the life (this PC, rough)", "ticks_per_s", "")):
        out.append(f"\n## {title}\n")
        if note:
            out.append(note + "\n")
        d0 = 0 if key == "ticks_per_s" else 3
        out.append("| world | " + " | ".join(life) + " | " +
                   " | ".join(f"{a} - pred" for a in life if a != "pred") + " |")
        out.append("|---|" + "---|" * (2 * len(life) - 1))
        for w in worlds:
            cells = [fmt(mean(got(w, a, "life", key)), 7, d0) for a in life]
            if key == "ticks_per_s":
                base = mean(got(w, "pred", "life", key))
                cells += [f"{mean(got(w, a, 'life', key)) / base:.2f}x"
                          if base and mean(got(w, a, "life", key)) else "-"
                          for a in life if a != "pred"]
            else:
                cells += [diff(got(w, a, "life", key), got(w, "pred", "life", key))[0]
                          for a in life if a != "pred"]
            out.append(f"| {w} | " + " | ".join(cells) + " |")
    out.append("\n## 6. The rule's work per life (mean over seeds)\n")
    out.append("| world | arm | teaching steps | cells taught a step | counter steps | "
               "re-picks | near share |")
    out.append("|---|---|---|---|---|---|---|")
    for w in worlds:
        for a in life:
            v = {k: mean(got(w, a, "life", k)) for k in
                 ("hop_verdicts", "hop_taught", "hop_steps", "hop_rewires", "hop_near",
                  "hop_cellticks")}
            if not v["hop_verdicts"]:
                continue
            out.append(f"| {w} | {a} | {v['hop_verdicts']:.0f} | "
                       f"{v['hop_taught'] / v['hop_verdicts']:.0f} | {v['hop_steps']:.0f} | "
                       f"{v['hop_rewires']:.0f} | {v['hop_near'] / v['hop_cellticks']:.3f} |")
    text = "\n".join(out) + "\n"
    open(os.path.join(ROOT, "out", "predhop", opt["name"] + ".md"), "w").write(text)
    print(text)


def main():
    opt = parse(sys.argv[1:])
    folder = os.path.join(ROOT, "out", "predhop", opt["name"])
    os.makedirs(folder, exist_ok=True)
    if not opt["table_only"]:
        jobs = [job(w, a, s, opt, folder) for s in opt["seeds"] for w in opt["worlds"]
                for a in opt["arms"]]
        with ThreadPoolExecutor(max_workers=opt["jobs"]) as pool:
            list(pool.map(run, jobs))
    tables(opt, folder)


if __name__ == "__main__":
    main()
