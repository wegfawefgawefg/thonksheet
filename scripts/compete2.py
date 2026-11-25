#!/usr/bin/env python3
"""Competitive learning followed up (docs/results-2026-11-25-compete2.md). See compete2.sh."""

import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import inside  # noqa: E402  the inside-learning test's commands and scores

ROOT = inside.ROOT
BUILD = os.path.join(ROOT, os.environ.get("COMPETE2_BUILD", "build"))
inside.MNIST = os.path.join(BUILD, "blobmnist")
inside.PROBE = os.path.join(BUILD, "blobprobe")

B5 = "--g.learn_mode 5 --g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5"
A5 = "--g.learn_mode 5 --g.compete_on 1 --g.compete_shift 5"
CAP = " --g.threshold_cap 1000"
# the rule's settings: with the tile's winners (B5, the setting to use) and without (A5),
# each as the sheet fires and with the rate held at target in the life and the read
SETTINGS = {"b5": B5, "b5cap": B5 + CAP, "a5": A5, "a5cap": A5 + CAP}
KINDS = ["born", "lived", "shuffled", "churn"]
MEMBERS = ["xor4", "add4"]
# the one-hop rule beside competitive learning (docs/results-2026-11-25-hop.md, calc-comp):
# a taught life, the readout of scripts/hop.sh learning, motor cells put back as born
HOP_READOUT = "--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1"
HOP = "--g.hop_on 1 --g.hop_teach 2"
TW, COMP = "--g.tile_winners 4", "--g.compete_on 1 --g.compete_shift 5"
HOPCOMP = [  # name, life flags (None: born), born-sheet flags
    ("born", None, ""), ("born-tw", None, TW),
    ("readout", "", ""), ("readout-tw", TW, TW),
    ("compete-tw", TW + " " + COMP, TW), ("compete", COMP, ""),
    ("hop", HOP, ""), ("hop-tw", HOP + " " + TW, TW),
    ("hop-compete", HOP + " " + COMP, ""), ("hop-compete-tw", HOP + " " + TW + " " + COMP, TW),
    ("hop-compete7-tw", HOP + " " + TW + " " + COMP + " --g.compete_shift 7", TW),
]
# the time rows of the suite (scripts/suite.py), on the readout of the scoreboard's
# blob-readout column, with the tile's winners either way
TIME_BASE = ("--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 "
             "--motor-spread 128 --g.tile_winners 4")
TIME_JOBS = "delayed,full-order,full-gap"


def seeds_of(text):
    lo_hi = text.split("-")
    return (list(range(int(lo_hi[0]), int(lo_hi[1]) + 1)) if len(lo_hi) == 2
            else [int(x) for x in text.split(",")])


def run(job):
    cmd, out = job
    if "--out" in cmd:
        done = os.path.exists(cmd[cmd.index("--out") + 1])  # calc: its csv
    elif "ceiling" in cmd:
        done = os.path.exists(out) and "total wall" in open(out).read()
    else:
        done = os.path.exists(out)
    if done:
        return
    with open(out + ".part", "w") as f:
        f.write(" ".join(cmd[3:]) + "\n")
        f.flush()
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT)
    os.replace(out + ".part", out)


def mnist_rate(path):
    """the hidden cells' rate while the reader's digits were shown"""
    if not os.path.exists(path):
        return None
    m = re.search(r"frozen, fit digits.*?([0-9.]+) spikes per 1000", open(path).read())
    if not m:
        m = re.search(r"as born.*?([0-9.]+) spikes per 1000", open(path).read())
    return float(m.group(1)) if m else None


def fmt(v, d=4):
    return "  -   " if v is None else ("%." + str(d) + "f") % v


def diff_line(label, a, b, seeds, d=4):
    """per seed a - b, the mean, and how many seeds are above"""
    pairs = [(x - y) if x is not None and y is not None else None for x, y in zip(a, b)]
    got = [p for p in pairs if p is not None]
    cells = " ".join(("%+." + str(d) + "f") % p if p is not None else "   -   " for p in pairs)
    if not got:
        return "%-22s %s" % (label, cells)
    return "%-22s %s   mean %+.4f, %d of %d above" % (label, cells, sum(got) / len(got),
                                                     sum(p > 0 for p in got), len(got))


def mean(v):
    got = [x for x in v if x is not None]
    return sum(got) / len(got) if got else None


# ---- MNIST: born, lived, shuffled, churn; as the sheet fires and at target rate ----

def mnist_jobs(opt, outdir):
    jobs = []
    for name in opt["settings"]:
        d = os.path.join(outdir, "mnist", name)
        os.makedirs(d, exist_ok=True)
        for s in opt["seeds"]:
            for k in opt["kinds"]:
                o = os.path.join(d, "mnist-%s-%d.txt" % (k, s))
                jobs.append(inside.mnist_cmd(k, s, opt, SETTINGS[name].split(), o))
    if opt["quick"]:
        return jobs
    for s in opt["seeds"]:
        o = os.path.join(outdir, "mnist", "baselines-%d.txt" % s)
        jobs.append((["nice", "-n", "19", inside.MNIST, "baselines", "--mnist-code", "t28",
                      "--seed", str(s), "--grey", "0"], o))
    return jobs


def projection(path):
    if not os.path.exists(path):
        return None
    m = re.search(r"random projection \S+ x \S+, linear\s+([0-9.]+)", open(path).read())
    return float(m.group(1)) if m else None


def mnist_table(opt, outdir):
    seeds = opt["seeds"]
    head = "%-22s %s" % ("seed", " ".join("%-7d" % s for s in seeds))
    lines = ["MNIST test digits, a fresh linear reader of the non-sensor cells fit on %d training "
             "digits, after a %d-digit life with no reward (born: no life)" % (opt["fit"],
                                                                             opt["life"])]
    score, rate = {}, {}
    for name in opt["settings"]:
        for k in opt["kinds"]:
            paths = [os.path.join(outdir, "mnist", name, "mnist-%s-%d.txt" % (k, s)) for s in seeds]
            score[name, k] = [inside.mnist_score(p)[0] for p in paths]
            rate[name, k] = [mnist_rate(p) for p in paths]
    for name in opt["settings"]:
        lines += ["", "%s: %s" % (name, SETTINGS[name]), head]
        for k in opt["kinds"]:
            lines.append("%-22s %s   mean %s, read at %s" % (
                k, " ".join(fmt(v) + " " for v in score[name, k]), fmt(mean(score[name, k])),
                fmt(mean(rate[name, k]), 1)))
        for k in opt["kinds"]:
            if k != "lived":
                lines.append(diff_line("lived - " + k, score[name, "lived"], score[name, k], seeds))
    proj = [projection(os.path.join(outdir, "mnist", "baselines-%d.txt" % s)) for s in seeds]
    if any(p is not None for p in proj):
        lines += ["", "%-22s %s   mean %s (no firing rate: each unit is on for half the digits)" % (
            "random projection", " ".join(fmt(v) + " " for v in proj), fmt(mean(proj)))]
    lines += ["", "The claim: lived minus the controls, per seed (" + head.split(None, 1)[1] + ")"]
    for name in opt["settings"]:
        for k in ("shuffled", "churn", "born"):
            if k in opt["kinds"] and "lived" in opt["kinds"]:
                lines.append(diff_line("%s lived-%s" % (name, k), score[name, "lived"],
                                       score[name, k], seeds))
    return "\n".join(lines)


# ---- calc: held-out columns, born, lived, shuffled, churn ----

def calc_jobs(opt, outdir):
    jobs = []
    for name in opt["settings"]:
        d = os.path.join(outdir, "calc", name)
        os.makedirs(d, exist_ok=True)
        for mem in opt["members"]:
            for s in opt["seeds"]:
                for k in opt["kinds"]:
                    o = os.path.join(d, "calc-%s-%s-%d.txt" % (mem, k, s))
                    jobs.append(inside.calc_cmd(mem, k, s, opt, SETTINGS[name].split(), o))
    return jobs


def calc_cells(path):
    d = inside.calc_score(path)
    return (d.get(("sheet", "bits_seen")), d.get(("sheet", "bits_held")),
            d.get(("rate", "hidden_per_1000")))


def calc_table(opt, outdir):
    seeds = opt["seeds"]
    head = "%-22s %s" % ("seed", " ".join("%-6d" % s for s in seeds))
    lines = ["calc: ridge reader of the frozen sheet, bits right on held-out cases (never "
             "answered in the life); life %d ticks, no reward" % opt["calc_life"]]
    for mem in opt["members"]:
        for name in opt["settings"]:
            held, seen, rate = {}, {}, {}
            for k in opt["kinds"]:
                cells = [calc_cells(os.path.join(outdir, "calc", name, "calc-%s-%s-%d.csv" % (
                    mem, k, s))) for s in seeds]
                seen[k], held[k], rate[k] = ([c[i] for c in cells] for i in range(3))
            lines += ["", "%s, %s: %s" % (mem, name, SETTINGS[name]), head]
            for k in opt["kinds"]:
                lines.append("%-22s %s   held %s, seen %s, read at %s" % (
                    k, " ".join(fmt(v, 3) + "  " for v in held[k]), fmt(mean(held[k]), 3),
                    fmt(mean(seen[k]), 3), fmt(mean(rate[k]), 1)))
            for k in opt["kinds"]:
                if k != "lived" and "lived" in held:
                    lines.append(diff_line("held: lived - " + k, held["lived"], held[k], seeds, 3))
    return "\n".join(lines)


# ---- the one-hop rule beside competitive learning, decomposed (calc, taught life) ----

def hopcomp_jobs(opt, outdir):
    jobs = []
    d = os.path.join(outdir, "hopcomp")
    os.makedirs(d, exist_ok=True)
    for mem in opt["members"]:
        for name, life, born_flags in HOPCOMP:
            for s in opt["seeds"]:
                o = os.path.join(d, "calc-%s-%s-%d.txt" % (mem, name, s))
                cmd = (["nice", "-n", "19", inside.PROBE, "calc"] + inside.CALC_BASE +
                       ["--calc-grade", "bit", "--task", "calc"] + inside.CALC[mem].split() +
                       ["--seed", str(s), "--task-seed", str(s), "--trials", "3000", "--label",
                        mem, "--out", o[:-4] + ".csv"])
                if life is None:
                    cmd += born_flags.split()
                else:
                    cmd += (["--life", str(opt["hop_life"]), "--life-teach", "2"] +
                            HOP_READOUT.split() + life.split())
                jobs.append((cmd, o))
    return jobs


def hopcomp_table(opt, outdir):
    seeds = opt["seeds"]
    lines = ["calc, taught life of %d ticks (a verdict per bit, the readout learning, motor cells "
             "put back as born), then the frozen read: bits right held-out" % opt["hop_life"],
             "%-18s %s   mean held, seen, read rate" % ("seed", " ".join(
                 "%-6d" % s for s in seeds))]
    for mem in opt["members"]:
        lines.append(mem)
        for name, life, _ in HOPCOMP:
            cells = [calc_cells(os.path.join(outdir, "hopcomp", "calc-%s-%s-%d.csv" % (
                mem, name, s))) for s in seeds]
            held = [c[1] for c in cells]
            lines.append("  %-16s %s   %s, %s, %s" % (
                name, " ".join(fmt(v, 3) + "  " for v in held), fmt(mean(held), 3),
                fmt(mean([c[0] for c in cells]), 3), fmt(mean([c[2] for c in cells]), 1)))
    lines += ["", "flags: " + "; ".join("%s = %s" % (n, l if l is not None else "born " + b)
                                       for n, l, b in HOPCOMP)]
    return "\n".join(lines)


# ---- the time rows of the suite, rule on against off ----

def time_run(opt):
    names = ("compete2-time-off", "compete2-time-on")
    for name, extra in zip(names, ("", COMP)):
        cmd = ["python3", os.path.join(ROOT, "scripts", "suite.py"), name, "--only", TIME_JOBS,
               "--resume"] + (["--quick"] if opt["quick"] else []) + (TIME_BASE + " " + extra).split()
        if not opt["table_only"]:
            subprocess.run(cmd, cwd=ROOT, stdout=subprocess.DEVNULL)
    return time_table([n + ("-quick" if opt["quick"] else "") for n in names])


def time_table(names):
    import csv
    rows = {}
    for col, name in enumerate(names):
        path = os.path.join(ROOT, "out", "scorecards", name + ".csv")
        if not os.path.exists(path):
            continue
        for r in csv.DictReader(open(path)):
            if r["property"] != "about" and r["mean"]:
                rows.setdefault(r["test"], [None, None])[col] = r
    lines = ["the suite's time rows (late score, mean ± spread over its seeds; learning-off twin in brackets), "
             "readout of blob-readout + tile winners 4; on adds " + COMP,
             "%-58s %-22s %-22s" % ("test", "off", "on")]
    for test, (off, on) in rows.items():
        cell = lambda r: ("%.3f ±%.3f (%.3f)" % (float(r["mean"]), float(r["spread"] or 0), float(
            r["learn_off"] or "nan")) if r else "-")
        lines.append("%-58s %-22s %-22s" % (test[:58], cell(off), cell(on)))
    return "\n".join(lines)


# ---- speed ----

def speed_one(build, kind, flags):
    """ticks per second of blobbench (8x8 tiles) or of an MNIST life (6,000 digits, seed 5)"""
    if kind == "bench":
        out = subprocess.run(["nice", "-n", "19", os.path.join(build, "blobbench"), "speed",
                              "--tiles", "8", "--learn", "1", "--seconds-ms", "3000",
                              "--g.learn_mode", "5"] + flags.split(), capture_output=True,
                             text=True, cwd=ROOT).stdout.strip().splitlines()
        return out[-1].split()[2] if len(out) > 1 else "-"
    cmd = ["nice", "-n", "19", os.path.join(build, "blobmnist"), "ceiling"] + inside.SHEET + \
        inside.READER + ["--fit", "300", "--test-count", "300", "--seed", "5", "--life", "6000",
                         "--g.learn_mode", "5"] + flags.split()
    out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT).stdout
    life = [l for l in out.splitlines() if l.startswith("life 6000")]
    m = re.search(r"\(([0-9]+) t/s\).*?([0-9.]+) spikes", life[0]) if life else None
    return "%s (rate %s)" % m.groups() if m else "-"


def speed_run(opt, outdir):
    """one job at a time; COMPETE2_BEFORE=DIR adds a column from another build (the read-off
    before this job: a build of commit 6bc8130)"""
    before = os.environ.get("COMPETE2_BEFORE")
    full = " --g.compete_full 1"
    rows = [("bench", "rule off", ""), ("bench", "rule on, shift 3", COMP.replace(
        " --g.compete_shift 5", "")), ("bench", "rule on, shift 5", COMP),
            ("bench", "rule on, shift 5, re-pick every update", COMP + full),
            ("mnist", "tile winners, rule off", TW), ("mnist", "B5", B5),
            ("mnist", "B5, re-pick every update", B5 + full),
            ("mnist", "tile winners, rule off, born hot", TW +
             " --g.sector_target_per_1024 64 --g.threshold_down_shift 8"),
            ("mnist", "tile winners, rule off, cap 1000", TW + CAP), ("mnist", "B5, cap 1000", B5 + CAP),
            ("mnist", "B5, cap 1000, re-pick every update", B5 + CAP + full)]
    lines = ["ticks per second, this machine, nice 19, one job at a time (rough: shared machine)",
             "bench: blobbench speed --tiles 8 --learn 1 --g.learn_mode 5 (random input); mnist: "
             "the life of a 6,000-digit MNIST life, seed 5, scripts/inside.sh's sheet",
             "%-6s %-42s %-22s %s" % ("", "", "now", "before" if before else "")]
    for kind, label, flags in rows:
        now = speed_one(BUILD, kind, flags)
        was = speed_one(os.path.join(ROOT, before), kind, flags) if before and full not in flags \
            else ""
        lines.append("%-6s %-42s %-22s %s" % (kind, label, now, was))
    return "\n".join(lines)


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(open(os.path.join(ROOT, "scripts", "compete2.sh")).read().split("set -")[0])
        return 1
    mode = args.pop(0)
    opt = {"seeds": list(range(5, 13)), "life": 60000, "fit": 60000, "calc_life": 200000,
           "hop_life": 400000, "trials": 3000, "jobs": 3, "settings": list(SETTINGS),
           "kinds": list(KINDS), "members": list(MEMBERS), "quick": False, "table_only": False}
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--quick":
            opt.update(quick=True, seeds=[5, 6], life=3000, fit=3000, calc_life=40000,
                       hop_life=40000, mnist_extra=["--test-count", "2000"])
            i += 1
        elif a == "--table-only":
            opt["table_only"] = True
            i += 1
        elif a == "--seeds":
            opt["seeds"] = seeds_of(args[i + 1])
            i += 2
        elif a in ("--settings", "--kinds", "--members"):
            opt[a[2:]] = args[i + 1].split(",")
            i += 2
        elif a == "--jobs":
            opt["jobs"] = int(args[i + 1])
            i += 2
        else:
            print("compete2: unknown option %s" % a)
            return 1
    outdir = os.path.join(ROOT, "out", "compete2-quick" if opt["quick"] else "compete2")
    if mode == "hopcomp" and opt["seeds"] == list(range(5, 13)):
        opt["seeds"] = [5, 6, 7, 8]
    makers = {"mnist": (mnist_jobs, mnist_table), "calc": (calc_jobs, calc_table),
              "hopcomp": (hopcomp_jobs, hopcomp_table)}
    modes = ["mnist", "calc", "hopcomp", "time"] if mode == "all" else [mode]
    texts = []
    for m in modes:
        if m == "time":
            texts.append(time_run(opt))
        elif m == "speed":
            texts.append(speed_run(opt, outdir))
        elif m in makers:
            if not opt["table_only"]:
                with ThreadPoolExecutor(max_workers=opt["jobs"]) as pool:
                    list(pool.map(run, makers[m][0](opt, outdir)))
            texts.append(makers[m][1](opt, outdir))
        else:
            print("compete2: modes are mnist, calc, hopcomp, time, speed, all")
            return 1
        os.makedirs(outdir, exist_ok=True)
        open(os.path.join(outdir, m + ".txt"), "w").write(texts[-1] + "\n")
    print("\n\n".join(texts))
    return 0


if __name__ == "__main__":
    sys.exit(main())
