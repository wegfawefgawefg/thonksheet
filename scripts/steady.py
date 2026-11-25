#!/usr/bin/env python3
"""Do the inner rules learn more from a steadier sheet? (idea 174,
docs/results-2026-11-26-steady.md). See steady.sh.

  steady.py NAME [--quick] [--seeds 5-8] [--arms a,b] [--tasks mnist,pair] [--reads std,quiet]
            [--kinds lived,shuffled,...] [--jobs 3] [--table-only]

Each run is `blobmnist ceiling`: a life of the arm (or none, born), then the inside-learning
test's fresh reader of the frozen sheet, everything at matched rate (--g.threshold_cap 1000 in
the life and the read). Two reads: std, the standard one (regulation moving, 20 ticks a
digit); quiet, regulation frozen after 3,000 digits of settling and 80 ticks a digit
(docs/results-2026-11-26-repeatable.md). An arm changes the life only; the read is the same
for every arm, so the born sheet is shared.
"""

import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import compete2  # noqa: E402  the competitive-learning setting B5 and the read rate
import inside  # noqa: E402  the MNIST sheet, its reader, the taught life's readout, scores
import repeat  # noqa: E402  the quiet read's arm, the pair test, the teacher's wiring

ROOT = inside.ROOT
MNIST = os.path.join(ROOT, os.environ.get("STEADY_BUILD", "build"), "blobmnist")
CAP = ["--g.threshold_cap", "1000"]
# the rules: competitive learning as compete2's B5; the one-hop rule with errors from the
# answer (docs/results-2026-11-25-hop-real.md), a taught life whose readout learns
COMPETE = compete2.B5.split()
HOP = inside.RULE + ["--g.hop_on", "1", "--g.hop_shift", "9", "--life-teach", "3"]
# the quiet read: regulation frozen after settling, 80 ticks a digit
QUIET = repeat.ARM["freeze+show80"][2] + ["--settle", "3000"]
# the teacher's wiring (idea 165's model at its bottom rung), seeds 5-12
repeat.WIRING = os.environ.get("REPEAT_WIRING", "/media/vega/mdot2b/Coding2/AI/thonksheet/out/"
                               "from-agents/repeat/wiring")
READS = {"std": [], "quiet": QUIET}
TASKS = {"mnist": [], "pair": repeat.PAIR}
# the controls of each rule (flags added to the life); born has no life
KINDS = {"compete": [("lived", []), ("shuffled", ["--life-shuffle", "1"]),
                     ("churn", ["--g.compete_churn", "1"])],
         "hop": [("lived", []), ("coin", ["--g.hop_coin", "1"]),
                 ("shuffled", ["--life-shuffle", "1"])]}
THFREEZE = ["--life-settle", "3000", "--life-set", "threshold_reg_on=0,sector_bias_max=128",
            "--life-target", "60"]
SLOWTH = ["--life-set", "threshold_up_shift=7,threshold_down_shift=12"]
SLOWBIAS = ["--life-set", "sector_smooth_shift=8"]
COUNT = ["--g.compete_window", "20", "--g.compete_shift", "2"]


def arm(name, rule, what, flags=(), digits=60000):
    return name, (rule, what, list(flags), digits)


# name: (rule, what, life flags, life digits). each alone; the combinations at the end
ARMS = dict([
    arm("base", "compete", "competitive learning B5, 60,000 digits of 20 ticks"),
    arm("s40d", "compete", "shown 40 ticks, same digits", ["--life-ticks", "40"]),
    arm("s40t", "compete", "shown 40 ticks, same ticks", ["--life-ticks", "40"], 30000),
    arm("s80d", "compete", "shown 80 ticks, same digits", ["--life-ticks", "80"]),
    arm("s80t", "compete", "shown 80 ticks, same ticks", ["--life-ticks", "80"], 15000),
    arm("slowth", "compete", "threshold regulation 8 times slower", SLOWTH),
    arm("th32", "compete", "thresholds frozen after 3,000 digits, sector bias to 32",
        ["--life-settle", "3000", "--life-set", "threshold_reg_on=0,sector_bias_max=32"]),
    arm("thfreeze", "compete", "thresholds frozen after 3,000 digits, sector bias to 128, "
        "sector targets 60%", THFREEZE),
    arm("slowbias", "compete", "sector activity smoothed over 256 ticks", SLOWBIAS),
    arm("count5", "compete", "counted: the tile's 4 most active over a digit move, toward "
        "sources on 5 of its 20 ticks (compete_shift 2)", COUNT + ["--g.compete_window_on", "5"]),
    arm("count1", "compete", "counted, toward sources on at least once in the digit",
        COUNT + ["--g.compete_window_on", "1"]),
    arm("r6", "compete", "compete_shift 6", ["--g.compete_shift", "6"]),
    arm("r7", "compete", "compete_shift 7", ["--g.compete_shift", "7"]),
    arm("r5l", "compete", "120,000 digits", [], 120000),
    arm("r6l", "compete", "compete_shift 6, 120,000 digits", ["--g.compete_shift", "6"], 120000),
    arm("r7l", "compete", "compete_shift 7, 120,000 digits", ["--g.compete_shift", "7"], 120000),
    arm("hbase", "hop", "one-hop rule, errors from the answer, hop_shift 9, 60,000 digits"),
    arm("hs40d", "hop", "shown 40 ticks, same digits", ["--life-ticks", "40"]),
    arm("hs80d", "hop", "shown 80 ticks, same digits", ["--life-ticks", "80"]),
    arm("hs80t", "hop", "shown 80 ticks, same ticks", ["--life-ticks", "80"], 15000),
    arm("hslowth", "hop", "threshold regulation 8 times slower", SLOWTH),
    arm("hthfreeze", "hop", "thresholds frozen, bias to 128, targets 60%", THFREEZE),
    arm("hslowbias", "hop", "sector activity smoothed over 256 ticks", SLOWBIAS),
    arm("h9l", "hop", "120,000 digits", [], 120000),
    arm("h10", "hop", "hop_shift 10", ["--g.hop_shift", "10"]),
    arm("h11", "hop", "hop_shift 11", ["--g.hop_shift", "11"]),
    arm("h10l", "hop", "hop_shift 10, 120,000 digits", ["--g.hop_shift", "10"], 120000),
    arm("h11l", "hop", "hop_shift 11, 120,000 digits", ["--g.hop_shift", "11"], 120000),
])
QUICK_ARMS = ["base", "count5", "thfreeze", "s80t", "hbase"]


def seeds_of(text):
    return compete2.seeds_of(text)


def run_job(job):
    """(command, output): written to .part, renamed when the run finished well"""
    cmd, out = job
    if os.path.exists(out):
        return
    with open(out + ".part", "w") as f:
        f.write(" ".join(cmd[3:]) + "\n")
        f.flush()
        r = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT)
    if r.returncode == 0:
        os.replace(out + ".part", out)


def run_file(outdir, task, read, name, seed):
    return os.path.join(outdir, "%s-%s" % (task, read), "%s-%d.txt" % (name, seed))


def command(opt, task, read, seed, rule_flags, life=None):
    """life: (digits, life flags) or None for no life"""
    cmd = ["nice", "-n", "19", MNIST, "ceiling"] + inside.SHEET + inside.READER + [
        "--fit", str(opt["fit"]), "--seed", str(seed)] + TASKS[task] + READS[read]
    cmd += opt["extra"] + rule_flags + CAP
    if life:
        digits, flags = life
        digits = max(digits * opt["life"] // 60000, 1)
        ticks = [] if "--life-ticks" in flags else ["--life-ticks", "20"]
        cmd += ["--life", str(digits)] + ticks + flags
    return cmd


def jobs_for(opt, outdir):
    jobs = []
    for task in opt["tasks"]:
        for read in opt["reads"]:
            os.makedirs(os.path.join(outdir, "%s-%s" % (task, read)), exist_ok=True)
            for seed in opt["seeds"]:
                rules = sorted({ARMS[a][0] for a in opt["arms"]})
                for rule in rules if "born" in opt["kinds"] else []:
                    out = run_file(outdir, task, read, "born-" + rule, seed)
                    flags = COMPETE if rule == "compete" else []
                    jobs.append((command(opt, task, read, seed, flags), out))
                w = repeat.wiring_path(outdir, task, seed, False) if "teacher" in opt[
                    "kinds"] else None
                if w:
                    jobs.append((command(opt, task, read, seed, ["--wiring-load", w]),
                                 run_file(outdir, task, read, "teacher", seed)))
                for name in opt["arms"]:
                    rule, _, flags, digits = ARMS[name]
                    for kind, kflags in KINDS[rule]:
                        if kind not in opt["kinds"]:
                            continue
                        base = COMPETE if rule == "compete" else HOP
                        jobs.append((command(opt, task, read, seed, base, (digits, flags + kflags)),
                                     run_file(outdir, task, read, "%s-%s" % (name, kind), seed)))
    # the long runs first, so the pool ends on short ones
    jobs = [j for j in jobs if not os.path.exists(j[1])]
    jobs.sort(key=lambda j: -cost(j[0]))
    return jobs


def cost(cmd):
    """about how long a run takes, in standard reads: a quiet read is four, a life's ticks"""
    c = 4.0 if "--read-freeze" in cmd else 1.0
    if "--life" in cmd:
        ticks = int(cmd[cmd.index("--life-ticks") + 1])
        c += int(cmd[cmd.index("--life") + 1]) * ticks / 1200000
    return c


def read_run(path):
    """the score, the read rate, and from the life's lines: the rate in its first and last
    tenth, silent and stuck (over 10x the mean) cells in the last tenth, pixel inputs and
    their likeness at the end; silent cells while read"""
    if not os.path.exists(path):
        return None
    score, _, frozen = inside.mnist_score(path)
    if score is None:
        return None
    text = open(path).read()
    life = re.findall(r"^life \d+ .*?: ([\d.]+) spikes per 1000 cell-ticks, silent ([\d.]+), "
                      r"over 10x the mean ([\d.]+)", text, re.M)
    wire = re.findall(r"^life \d+ .*pixels ([\d.]+) .*likeness ([\d.]+)", text, re.M)
    silent = re.search(r"silent ([\d.]+)", frozen)
    return {"score": score, "rate": compete2.mnist_rate(path),
            "life0": float(life[0][0]) if life else None,
            "life1": float(life[-1][0]) if life else None,
            "lsilent": float(life[-1][1]) if life else None,
            "lstuck": float(life[-1][2]) if life else None,
            "pixels": float(wire[-1][0]) if wire else None,
            "like": float(wire[-1][1]) if wire else None,
            "silent": float(silent.group(1)) if silent else None}


def mean(v):
    v = [x for x in v if x is not None]
    return sum(v) / len(v) if v else None


def fmt(v, form="%.4f"):
    return "  -   " if v is None else form % v


def diff(a, b):
    """paired by seed: mean, seeds above, seeds with both"""
    d = [x["score"] - y["score"] for x, y in zip(a, b) if x and y]
    return (sum(d) / len(d), sum(x > 0 for x in d), len(d)) if d else None


def fmt_diff(d):
    return "    -          " if d is None else "%+.4f (%d of %d)" % d


def table(opt, outdir):
    seeds = opt["seeds"]
    lines = ["matched rate (--g.threshold_cap 1000 in life and read); a fresh linear reader of "
             "the non-sensor cells, test digits; means over seeds %s" % ",".join(map(str, seeds))]
    for task in opt["tasks"]:
        for read in opt["reads"]:
            got = lambda name: [read_run(run_file(outdir, task, read, name, s)) for s in seeds]
            lines += ["", "%s, %s read" % (task, read)]
            for name in ("born-compete", "born-hop", "teacher"):
                v = got(name)
                if any(v):
                    lines.append("%-24s %s  read at %s  [%d seeds]" % (
                        name, fmt(mean([x["score"] for x in v if x])),
                        fmt(mean([x["rate"] for x in v if x]), "%.1f"), sum(1 for x in v if x)))
            lines.append("%-10s %-7s %-7s %-7s %-17s %-17s %-17s %-5s %-11s %-11s %-10s" % (
                "arm", "lived", "ctrl1", "ctrl2", "lived-ctrl1", "lived-ctrl2", "vs base",
                "read", "life rate", "silent/stk", "px/like"))
            for name in opt["arms"]:
                rule = ARMS[name][0]
                k = [kind for kind, _ in KINDS[rule]]
                v = {kind: got("%s-%s" % (name, kind)) for kind in k}
                if not any(v["lived"]):
                    continue
                # the number this job moves: lived minus shuffled (compete), minus coin (hop)
                main = "shuffled" if rule == "compete" else "coin"
                other = "churn" if rule == "compete" else "shuffled"
                ref = "base" if rule == "compete" else "hbase"
                rv = {kind: got("%s-%s" % (ref, kind)) for kind in (main, "lived")}
                d_main = diff(v["lived"], v[main])
                d_ref = None
                if name != ref:
                    a = [x["score"] - y["score"] if x and y else None
                         for x, y in zip(v["lived"], v[main])]
                    b = [x["score"] - y["score"] if x and y else None
                         for x, y in zip(rv["lived"], rv[main])]
                    dd = [x - y for x, y in zip(a, b) if x is not None and y is not None]
                    d_ref = (sum(dd) / len(dd), sum(x > 0 for x in dd), len(dd)) if dd else None
                lv = [x for x in v["lived"] if x]
                avg = lambda key: mean([x[key] for x in lv])
                lines.append("%-10s %s  %s  %s  %s %s %s %-5s %-11s %-11s %-10s" % (
                    name, fmt(avg("score")), fmt(mean([x["score"] for x in v[main] if x])),
                    fmt(mean([x["score"] for x in v[other] if x])), fmt_diff(d_main),
                    fmt_diff(diff(v["lived"], v[other])), fmt_diff(d_ref), fmt(avg("rate"), "%.1f"),
                    "%s>%s" % (fmt(avg("life0"), "%.0f"), fmt(avg("life1"), "%.0f")),
                    "%s/%s" % (fmt(avg("lsilent"), "%.3f"), fmt(avg("lstuck"), "%.3f")),
                    "%s/%s" % (fmt(avg("pixels"), "%.1f"), fmt(avg("like"), "%.2f"))))
    lines += ["", "ctrl1, ctrl2: shuffled and churn for competitive learning, coin and shuffled "
              "for the one-hop rule; lived-ctrl: paired by seed, mean (seeds above, of seeds); "
              "vs base: lived-ctrl1 of the arm less that of base (hbase), paired by seed; "
              "read: hidden spikes per 1,000 cell-ticks while read; life rate: the same in the "
              "life's first and last tenth; silent/stk: hidden cells silent and over 10 times the "
              "mean rate in the life's last tenth; px/like: pixel inputs of a hidden excitatory "
              "cell and their likeness at the end of the life",
              "arms: " + "; ".join("%s %s" % (a, ARMS[a][1]) for a in opt["arms"])]
    return "\n".join(lines)


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(open(os.path.join(ROOT, "scripts", "steady.sh")).read().split("set -")[0])
        return 1
    name = args.pop(0)
    opt = {"seeds": [5, 6, 7, 8], "arms": list(ARMS), "tasks": list(TASKS), "reads": list(READS),
           "kinds": ["born", "teacher", "lived", "shuffled", "churn", "coin"], "jobs": 3,
           "table_only": False, "quick": False, "life": 60000, "fit": 60000, "extra": []}
    if "--quick" in args:
        opt.update(seeds=[5], arms=QUICK_ARMS, life=3000, fit=3000,
                   extra=["--test-count", "1000"])
    i = 0
    while i < len(args):
        a = args[i]
        if a in ("--quick", "--table-only"):
            opt[a[2:].replace("-", "_")] = True
            i += 1
            continue
        if a == "--seeds":
            opt["seeds"] = seeds_of(args[i + 1])
        elif a in ("--arms", "--tasks", "--reads", "--kinds"):
            opt[a[2:]] = args[i + 1].split(",")
        elif a == "--jobs":
            opt["jobs"] = int(args[i + 1])
        else:
            print("steady: unknown option", a)
            return 1
        i += 2
    unknown = [a for a in opt["arms"] if a not in ARMS]
    if unknown:
        print("steady: unknown arms:", ",".join(unknown))
        return 1
    outdir = os.path.join(ROOT, "out", "steady", name)
    os.makedirs(outdir, exist_ok=True)
    if not opt["table_only"]:
        jobs = jobs_for(opt, outdir)
        print("%d runs to do, %d at a time" % (len(jobs), opt["jobs"]), file=sys.stderr)
        with ThreadPoolExecutor(opt["jobs"]) as pool:
            list(pool.map(run_job, jobs))
        for f in sorted(os.listdir(outdir)):
            if re.match(r"wiring-.*\.txt$", f):
                os.remove(os.path.join(outdir, f))
    text = table(opt, outdir)
    with open(os.path.join(outdir, "table.txt"), "w") as f:
        f.write(text + "\n")
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
