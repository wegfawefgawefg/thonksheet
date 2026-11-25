#!/usr/bin/env python3
"""Why the sheet does not answer the same digit the same way twice (idea 168,
docs/results-2026-11-26-repeatable.md). See repeat.sh.

  repeat.py NAME [--quick] [--seeds 5-12] [--arms a,b] [--tasks mnist,pair]
            [--kinds born,transplant] [--jobs 3] [--settle 3000] [--table-only]
  repeat.py wide NAME [--seeds 5] [--tasks mnist,pair]   the wide cells' transplant wiring
  repeat.py wiring NAME [--seeds 8-12]   idea 165's transplant wiring for seeds without one
  repeat.py time [--seeds 5-12]   the time rows of `scripts/oneset.sh` (cached there) for the
            columns of TIME: each change on the best setting, beside the best setting
  repeat.py check NAME [--arms base] [--kinds born,transplant] [--seeds 5] [--tasks mnist,pair]
            idea 165's measure: two whole runs (3,000 and 3,001 digits of settling)

Each arm is one `blobmnist ceiling --twice 1` run per task, kind and seed: the born sheet, or
idea 165's transplanted wiring (`--wiring-load`), with the arm's change, 3,000 digits of
settling, the inside-learning test's reader on the 60,000 training digits, then the test
digits read twice from two starting states. Three numbers per run: (a) a cell's count on a
test digit against its count on the same digit in the second pass (correlation over digits,
mean over the non-sensor cells), (b) the fresh reader on the test digits, (c) spikes per
1,000 cell-ticks and the share of cell-digits with a spike.
"""

import gzip
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MNIST = os.path.join(ROOT, os.environ.get("REPEAT_BUILD", "build"), "blobmnist")
# idea 165's transplant wirings, gzipped (seed 5 kept there; more seeds in out/repeat/wiring/,
# made by `repeat.py wiring`, or in the run folder)
WIRING = os.environ.get("REPEAT_WIRING", "/media/vega/mdot2b/Coding2/AI/thonksheet/out/"
                        "from-agents/ladder2/full")
SHEET = ("--mnist-code t28 --g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1 "
         "--g.motor_w 64 --g.motor_h 64 --motor-spread 102 --g.local_connect_shift 4").split()
READER = "--scale 2 --passes 10 --lr-1000 1 --views 1".split()
PAIR = "--pair 2 --pair-mix 1".split()
WIDE = ["--g.wire_pixel_shift", "2"]

# name: (what it is, flags). one change each from the standard sheet; the last are pairs
ARMS = [
    ("base", "the standard sheet", []),
    ("rec1", "no excitation from other cells (pixels, inhibition kept)", ["--rec-cut", "1"]),
    ("rec2", "cells hear only pixels", ["--rec-cut", "2"]),
    ("noinh", "no inhibition (gain 0)", ["--g.inhibit_gain", "0"]),
    ("inhfast", "inhibition with no delay", ["--inh-fast", "1"]),
    ("freeze", "thresholds and sector bias frozen after settling", ["--read-freeze", "3"]),
    ("freezeth", "thresholds frozen after settling", ["--read-freeze", "1"]),
    ("freezebias", "sector bias frozen after settling", ["--read-freeze", "2"]),
    ("reset", "potentials and cooldowns cleared every digit", ["--reset-digit", "1"]),
    ("resetring", "and the spike ring cleared too", ["--reset-digit", "2"]),
    ("gap100", "100 quiet ticks after every digit", ["--gap", "100"]),
    ("paceoff", "pacemakers off", ["--pace-off", "1"]),
    ("noiseoff", "noise off", ["--g.noise_on", "0"]),
    ("quiet", "pacemakers and noise off", ["--pace-off", "1", "--g.noise_on", "0"]),
    ("delay1", "every delay 1", ["--delays-one", "1"]),
    ("show40", "digit shown 40 ticks", ["--ticks", "40"]),
    ("show80", "digit shown 80 ticks", ["--ticks", "80"]),
    ("last10", "only the last 10 of 20 ticks counted", ["--count-from", "10"]),
    ("every2", "a pixel line fires every second tick", ["--input-every", "2"]),
    ("target16", "target rate half (16 per 1,024)",
     ["--g.sector_target_per_1024", "16", "--g.threshold_down_shift", "10"]),
    ("target8", "target rate a quarter (8 per 1,024)",
     ["--g.sector_target_per_1024", "8", "--g.threshold_down_shift", "11"]),
    ("wide", "wider cells (wire_pixel_shift 2)", WIDE),
    ("thregoff", "no per-cell threshold regulation, from birth", ["--g.threshold_reg_on", "0"]),
    ("slowreg", "threshold regulation 8 times slower",
     ["--g.threshold_up_shift", "7", "--g.threshold_down_shift", "12"]),
    ("winners2", "2 winners per tile", ["--g.tile_winners", "2"]),
    ("slowbias", "sector activity smoothed over 256 ticks, not 16", ["--g.sector_smooth_shift", "8"]),
    ("thregoff32", "no per-cell threshold regulation, sector bias up to 32",
     ["--g.threshold_reg_on", "0", "--g.sector_bias_max", "32"]),
    ("thregoff32+slowbias", "both", ["--g.threshold_reg_on", "0", "--g.sector_bias_max", "32",
                                     "--g.sector_smooth_shift", "8"]),
    ("still", "frozen, potentials, cooldowns and ring cleared, no noise, no pacemakers",
     ["--read-freeze", "3", "--reset-digit", "2", "--pace-off", "1", "--g.noise_on", "0"]),
    ("still+noise", "the same with noise on",
     ["--read-freeze", "3", "--reset-digit", "2", "--pace-off", "1"]),
    ("still+noise+rec2", "the same, cells hearing only pixels",
     ["--read-freeze", "3", "--reset-digit", "2", "--pace-off", "1", "--rec-cut", "2"]),
    ("still+show80", "still, shown 80 ticks", ["--read-freeze", "3", "--reset-digit", "2",
                                                "--pace-off", "1", "--g.noise_on", "0",
                                                "--ticks", "80"]),
    ("freeze+show80", "frozen and shown 80 ticks", ["--read-freeze", "3", "--ticks", "80"]),
    ("freeze+reset", "frozen and potentials cleared", ["--read-freeze", "3", "--reset-digit", "1"]),
    ("thregoff+show40", "no threshold regulation and shown 40 ticks",
     ["--g.threshold_reg_on", "0", "--ticks", "40"]),
]
ARM = {a[0]: a for a in ARMS}
DEFAULT_ARMS = [a[0] for a in ARMS]


def nice(cmd):
    return ["nice", "-n", "19"] + cmd


def run_job(job):
    """(command, output file): output written to .part and renamed when the run finished"""
    cmd, out = job
    if os.path.exists(out):
        return
    with open(out + ".part", "w") as f:
        f.write(" ".join(cmd[3:]) + "\n")
        f.flush()
        r = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT)
    if r.returncode == 0:
        os.replace(out + ".part", out)


def pool(jobs, n):
    with ThreadPoolExecutor(n) as ex:
        list(ex.map(run_job, jobs))


def wiring_path(outdir, task, seed, wide):
    """the transplant's wiring, unpacked into the run folder (and removed after)"""
    name = "wiring-%s%s-%d.txt" % ("wide-" if wide else "", task, seed)
    local = os.path.join(outdir, name)
    for src in (os.path.join(outdir, name + ".gz"), os.path.join(ROOT, "out", "repeat", "wiring",
                                                                 name + ".gz"),
                os.path.join(WIRING, name + ".gz")):
        if not os.path.exists(local) and os.path.exists(src):
            with gzip.open(src, "rb") as a, open(local, "wb") as b:
                shutil.copyfileobj(a, b)
    return local if os.path.exists(local) else None


def run_file(outdir, arm, kind, task, seed):
    return os.path.join(outdir, "%s-%s-%s-%d.txt" % (arm, kind, task, seed))


def jobs_for(outdir, opt):
    jobs = []
    for seed in opt["seeds"]:
        for task in opt["tasks"]:
            for arm in opt["arms"]:
                flags = ARM[arm][2]
                for kind in opt["kinds"]:
                    out = run_file(outdir, arm, kind, task, seed)
                    if os.path.exists(out):
                        continue
                    cmd = [MNIST, "ceiling"] + SHEET + READER + [
                        "--seed", str(seed), "--settle", str(opt["settle"]), "--twice", "1"]
                    cmd += PAIR if task == "pair" else []
                    cmd += opt["size"] + flags
                    if kind == "transplant":
                        w = wiring_path(outdir, task, seed, WIDE[0] in flags)
                        if not w:
                            continue
                        cmd += ["--wiring-load", w]
                    jobs.append((nice(cmd), out))
    # the long showings first, so the pool ends on short runs
    jobs.sort(key=lambda j: -("show" in j[1]))
    return jobs


def parse(path):
    if not os.path.exists(path):
        return None
    text = open(path).read()
    r = re.search(r"^sheet, sensor cells left out\s+\d+\s+([\d.]+)", text, re.M)
    c = re.search(r"self-correlation ([\d.]+) over (\d+) of (\d+) cells; rate ([\d.]+) per 1000 "
                  r"cell-ticks counted; active ([\d.]+); mean ([\d.]+)", text)
    if not r or not c:
        return None
    return {"reader": float(r.group(1)), "corr": float(c.group(1)),
            "kept": int(c.group(2)) / int(c.group(3)), "rate": float(c.group(4)),
            "active": float(c.group(5))}


def cell(vals, key, fmt):
    """mean, and the lowest and highest seed when there are several"""
    v = [x[key] for x in vals if x]
    if not v:
        return "-"
    text = fmt % (sum(v) / len(v))
    if len(v) > 1:
        text += (" (" + fmt + "-" + fmt + ")") % (min(v), max(v))
    return text + ("" if len(v) == len(vals) else " [%d seeds]" % len(v))


def table(outdir, opt):
    lines = []
    seeds = opt["seeds"]
    for task in opt["tasks"]:
        lines.append("%s, seeds %s" % (task, ",".join(map(str, seeds))))
        lines.append("%-20s %-10s %-24s %-24s %-8s %-7s %-6s" % (
            "arm", "kind", "self-corr", "reader", "rate", "active", "kept"))
        for arm in opt["arms"]:
            for kind in opt["kinds"]:
                vals = [parse(run_file(outdir, arm, kind, task, s)) for s in seeds]
                if not any(vals):
                    continue
                lines.append("%-20s %-10s %-24s %-24s %-8s %-7s %-6s" % (
                    arm, kind, cell(vals, "corr", "%.3f"), cell(vals, "reader", "%.4f"),
                    cell(vals, "rate", "%.1f").split(" ")[0],
                    cell(vals, "active", "%.2f").split(" ")[0],
                    cell(vals, "kept", "%.2f").split(" ")[0]))
        lines.append("")
    lines.append("self-corr: a cell's counts on the test digits against its counts on the same "
                 "digits read again (mean over cells); reader: the fresh reader on the test "
                 "digits; rate: spikes per 1,000 cell-ticks counted; active: share of "
                 "cell-digits with a spike; kept: share of cells that spiked in both passes")
    lines.append("arms: " + "; ".join("%s %s" % (a, ARM[a][1]) for a in opt["arms"]))
    return "\n".join(lines)


def train_wiring(name, opt, wide):
    """the bottom rung of idea 165's model (scripts/ladder2.py run TASK 7b trained) trained on
    the born sheet, written as a transplant wiring; wide: on the wide sheet, whose born counts
    are the wider cells'. needs LADDER_VENV (torch). seeds whose wiring exists are skipped"""
    outdir = os.path.join(ROOT, "out", "repeat", name)
    os.makedirs(outdir, exist_ok=True)
    py = os.path.join(os.environ["LADDER_VENV"], "bin", "python")
    tag = "wide-" if wide else ""
    jobs = []
    for seed in opt["seeds"]:
        sheet = os.path.join(outdir, "sheet-%s%d.txt" % (tag, seed))
        pool([(nice([MNIST, "ceiling"] + SHEET + (WIDE if wide else []) + [
            "--fit", "1", "--test-count", "1", "--settle", "0", "--seed", str(seed),
            "--views", "1", "--describe", sheet]), sheet + ".out")], 1)
        for task in opt["tasks"]:
            w = os.path.join(outdir, "wiring-%s%s-%d.txt" % (tag, task, seed))
            if os.path.exists(w + ".gz") or os.path.exists(os.path.join(
                    WIRING, os.path.basename(w) + ".gz")):
                continue
            out = os.path.join(outdir, "model-%s%s-%d.json" % (tag, task, seed))
            jobs.append((nice([py, os.path.join(ROOT, "scripts", "ladder2.py"), "run", task,
                               "7b", "trained", str(seed), sheet, out, "--epochs", "12",
                               "--wiring", w]), out + ".txt"))
    pool(jobs, opt["jobs"])
    for f in sorted(os.listdir(outdir)):
        if f.startswith("wiring-") and f.endswith(".txt"):
            p = os.path.join(outdir, f)
            with open(p, "rb") as a, gzip.open(p + ".gz", "wb") as b:
                shutil.copyfileobj(a, b)
            os.remove(p)


def test_rows(path):
    """the test part of a --rows-out file: (n, d, bytes of n rows of d + 5)"""
    with open(path, "rb") as f:
        for part in range(2):
            n, d = map(int, f.readline().split()[1:])
            raw = f.read(n * (d + 5))
    return n, d, raw


def corr_rows(a, b, n, d):
    """mean over columns of each column's correlation between two row blocks"""
    total, kept = 0.0, 0
    for c in range(5, d + 5):
        x, y = a[c::d + 5], b[c::d + 5]
        sx, sy = sum(x), sum(y)
        sxx, syy = sum(v * v for v in x), sum(v * v for v in y)
        sxy = sum(u * v for u, v in zip(x, y))
        vx, vy = sxx - sx * sx / n, syy - sy * sy / n
        if vx > 1e-9 and vy > 1e-9:
            total += (sxy - sx * sy / n) / (vx * vy) ** 0.5
            kept += 1
    return total / max(kept, 1), kept


def check(name, opt):
    """idea 165's measure of self-correlation (scripts/ladder2.py match): two separate runs
    on the same test digits, one digit of settling apart, for each arm, kind and task. the
    rows files are large and are removed when read"""
    outdir = os.path.join(ROOT, "out", "repeat", name)
    os.makedirs(outdir, exist_ok=True)
    for seed in opt["seeds"]:
        for task in opt["tasks"]:
            for arm in opt["arms"]:
                for kind in opt["kinds"]:
                    one_check(outdir, opt, seed, task, arm, kind)


def one_check(outdir, opt, seed, task, arm, kind):
    out = os.path.join(outdir, "check-%s-%s-%s-%d.txt" % (arm, kind, task, seed))
    if os.path.exists(out):
        print(open(out).read().strip())
        return
    flags = ARM[arm][2]
    if kind == "transplant":
        w = wiring_path(outdir, task, seed, WIDE[0] in flags)
        if not w:
            return
        flags = flags + ["--wiring-load", w]
    rows = [os.path.join(outdir, "rows-%s-%s-%s-%d-%d.bin" % (arm, kind, task, seed, k))
            for k in (0, 1)]
    jobs = []
    for k in (0, 1):
        cmd = [MNIST, "ceiling"] + SHEET + READER + opt["size"] + [
            "--seed", str(seed), "--settle", str(opt["settle"] + k), "--rows-out",
            rows[k]] + (PAIR if task == "pair" else []) + flags
        jobs.append((nice(cmd), rows[k] + ".txt"))
    pool(jobs, 2)
    (n, d, a), (_, _, b) = test_rows(rows[0]), test_rows(rows[1])
    corr, kept = corr_rows(a, b, n, d)
    text = "check %s %s %s seed %d: two whole runs, self-correlation %.4f over %d of %d cells" % (
        arm, kind, task, seed, corr, kept, d)
    with open(out, "w") as f:
        f.write(text + "\n")
    print(text)
    for r in rows:
        os.remove(r)
    for f in os.listdir(outdir):
        if re.match(r"wiring-.*\.txt$", f):
            os.remove(os.path.join(outdir, f))


# the cost side: the time rows with a change on top of the one setting (scripts/oneset.sh,
# docs/results-2026-11-26-one-setting.md); run with the oneset.sh lines in the results doc
TIME_ROWS = "order,gap,delayed2,copy2,xor,far5"
TIME = [("best + shown 80 ticks", ["--ticks", "80"]),
        ("best + no threshold regulation, bias to 32",
         ["--g.threshold_reg_on", "0", "--g.sector_bias_max", "32"])]


def time_table(seeds):
    import statistics
    import oneset as O
    rows, cols = O.read_map(O.MORE)
    rows = O.pick_rows(rows, TIME_ROWS)
    cache = O.load_cache()
    best = cols["best"]
    print("| row | best | " + " | ".join(n for n, _ in TIME) + " |")
    print("|---" * (len(TIME) + 2) + "|")
    for rname, rargs in rows:
        ref = O.scores(cache, rargs, best, seeds, False)
        cells = ["%.3f" % statistics.mean(ref) if None not in ref else "missing"]
        for _, extra in TIME:
            late = O.scores(cache, rargs, best + extra, seeds, False)
            twin = O.scores(cache, rargs, best + extra, seeds, True)
            if None in late or None in twin or None in ref:
                cells.append("missing")
                continue
            chance, perfect = O.calib_of(cache, rargs)
            _, ok = O.judge(late, twin, chance, perfect)
            d, se = O.paired(late, ref)
            cells.append("%.3f%s (%+.3f ±%.3f)" % (statistics.mean(late), "*" if ok else "", d, se))
        print("| %s | %s |" % (rname, " | ".join(cells)))
    print("\n`*` passes; in brackets the paired difference to best and its standard error")


def seeds_arg(v):
    lo_hi = v.split("-")
    if len(lo_hi) == 2:
        return list(range(int(lo_hi[0]), int(lo_hi[1]) + 1))
    return [int(x) for x in v.split(",")]


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 1
    if args[0] == "time":
        time_table(seeds_arg(args[2]) if args[1:2] == ["--seeds"] else list(range(5, 13)))
        return 0
    mode = args.pop(0) if args[0] in ("wide", "wiring", "check") else ""
    name = args.pop(0)
    opt = {"seeds": [5], "tasks": ["mnist", "pair"], "kinds": ["born", "transplant"],
           "arms": DEFAULT_ARMS, "jobs": 3, "settle": 3000, "table_only": False,
           "size": ["--fit", "60000"]}
    if "--quick" in args:
        opt.update(arms=["base", "freeze"], settle=200,
                   size=["--fit", "3000", "--test-count", "1000"])
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--seeds":
            opt["seeds"] = seeds_arg(args[i + 1])
        elif a in ("--tasks", "--kinds", "--arms"):
            opt[a[2:]] = args[i + 1].split(",")
        elif a in ("--jobs", "--settle"):
            opt[a[2:]] = int(args[i + 1])
        elif a in ("--table-only", "--quick"):
            opt[a[2:].replace("-", "_")] = True
            i -= 1
        else:
            print("unknown option", a)
            return 1
        i += 2
    unknown = [a for a in opt["arms"] if a not in ARM]
    if unknown:
        print("unknown arms:", ",".join(unknown))
        return 1
    if mode in ("wide", "wiring"):
        train_wiring(name, opt, mode == "wide")
        return 0
    if mode == "check":
        opt["arms"] = opt["arms"] if "--arms" in args else ["base"]
        check(name, opt)
        return 0
    outdir = os.path.join(ROOT, "out", "repeat", name)
    os.makedirs(outdir, exist_ok=True)
    if not opt["table_only"]:
        jobs = jobs_for(outdir, opt)
        print("%d runs to do, %d at a time" % (len(jobs), opt["jobs"]), file=sys.stderr)
        pool(jobs, opt["jobs"])
        for f in sorted(os.listdir(outdir)):
            if re.match(r"wiring-.*\.txt$", f):
                os.remove(os.path.join(outdir, f))
    text = table(outdir, opt)
    with open(os.path.join(outdir, "table.txt"), "w") as f:
        f.write(text + "\n")
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
