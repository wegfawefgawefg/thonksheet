#!/usr/bin/env python3
"""What stops the one-hop error on real data (docs/results-2026-11-25-hop-real.md).
See scripts/hop2.sh. Each table is a fixed recipe; one line runs it and prints it."""

import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import inside  # noqa: E402  the MNIST sheet, its reader and the taught life's readout
import hop  # noqa: E402  the toys' life and readout, the calc members

BUILD = os.path.join(ROOT, os.environ.get("HOP2_BUILD", "build"))
MNIST = os.path.join(BUILD, "blobmnist")
PROBE = os.path.join(BUILD, "blobprobe")

# the tests. same: are two interleaved half-size digits the same class; sum: is the sum of
# two side-by-side digits odd
SAME = "--pair 2 --pair-mix 1"
SUM = "--pair 1 --pair-mix 0"
HOP = "--g.hop_on 1"
COMPETE = "--g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5"
WIDE = "--g.wire_pixel_shift 2"
DEEP = "--g.wire_deep_percent 50"
# the rule's setting on the pair tests, picked on the validation slice, seeds 1 and 2 ("pick")
PAIRHOP = HOP + " --g.hop_shift 7 --g.hop_fired 0"


def mnist_arms(hop_flags, extra_arms=()):
    """born, the hop rule told the answer, its coin, its shuffled-digit life, and more"""
    arms = [("born", None, ""), ("hop", 2, hop_flags), ("coin", 2, hop_flags + " --g.hop_coin 1"),
            ("shuffled", 2, hop_flags + " --life-shuffle 1")]
    return arms + list(extra_arms)


# name -> recipe. mnist: (test flags, arms (name, taught life 1|2|3 or None, flags), seeds)
RECIPES = {
    # the bar: every pair test's plain learners and born sheet, seed 1
    "bar": {"kind": "bar"},
    # picking the step chance on the same test: validation slice, seeds 1, 2
    "pick": {"kind": "mnist", "test": SAME + " --val 1", "seeds": [1, 2], "arms": [
        (a + s, 2, HOP + " --g.hop_shift " + s + (" --g.hop_coin 1" if a == "coin" else ""))
        for s in ("3", "5", "7") for a in ("hop", "coin")] + [("off", 2, "")] + [
        (a + "7nf", 2, HOP + " --g.hop_shift 7 --g.hop_fired 0" +
         (" --g.hop_coin 1" if a == "coin" else "")) for a in ("hop", "coin")]},
    # the report on the same test, seeds 5-12
    "same": {"kind": "mnist", "test": SAME, "seeds": list(range(5, 13)), "arms": mnist_arms(
        PAIRHOP, [("compete", 2, COMPETE), ("both", 2, PAIRHOP + " " + COMPETE),
                  ("bothcoin", 2, PAIRHOP + " --g.hop_coin 1 " + COMPETE),
                  ("coin2", 2, PAIRHOP + " --g.hop_coin 2")])},
    # the second test, parity of the sum side by side, seeds 5-8
    "sum": {"kind": "mnist", "test": SUM, "seeds": [5, 6, 7, 8],
            "arms": mnist_arms(PAIRHOP, [("coin2", 2, PAIRHOP + " --g.hop_coin 2")])},
    # wider cells: more of each cell's inputs are pixels (MNIST at the hop doc's setting)
    "wide": {"kind": "mnist", "test": "", "seeds": [5, 6, 7, 8],
             "arms": [(n, l, f + " " + WIDE) for n, l, f in mnist_arms(HOP + " --g.hop_shift 7")]},
    "widesame": {"kind": "mnist", "test": SAME, "seeds": [5, 6, 7, 8],
                 "arms": [(n, l, f + " " + WIDE) for n, l, f in mnist_arms(PAIRHOP)]},
    # depth: half the cells hear no pixels, only cells
    "deepsame": {"kind": "mnist", "test": SAME, "seeds": list(range(5, 13)),
                 "arms": [(n, l, f + " " + DEEP) for n, l, f in mnist_arms(PAIRHOP)]},
    # errors against the answer alone, every step teaching (hop_teach 3)
    "target": {"kind": "mnist", "test": "", "seeds": list(range(5, 13)), "arms": [
        ("born", None, ""), ("hop", 3, HOP + " --g.hop_shift 9"),
        ("coin", 3, HOP + " --g.hop_shift 9 --g.hop_coin 1"),
        ("shuffled", 3, HOP + " --g.hop_shift 9 --life-shuffle 1")]},
    # the same with the rate held at target (threshold regulation uncapped)
    "targetcap": {"kind": "mnist", "test": "--g.threshold_cap 1000", "seeds": [5, 6, 7, 8],
                  "arms": [("born", None, ""), ("hop", 3, HOP + " --g.hop_shift 9"),
                           ("coin", 3, HOP + " --g.hop_shift 9 --g.hop_coin 1")]},
    # told the answer only on bad steps at the same slow step: is it the target, or the step?
    "slow": {"kind": "mnist", "test": "", "seeds": [5, 6, 7, 8], "arms": [
        ("hop", 2, HOP + " --g.hop_shift 9"), ("coin", 2, HOP + " --g.hop_shift 9 --g.hop_coin 1")]},
    "slowsame": {"kind": "mnist", "test": SAME, "seeds": [5, 6, 7, 8], "arms": [
        ("hop", 2, HOP + " --g.hop_shift 9"), ("coin", 2, HOP + " --g.hop_shift 9 --g.hop_coin 1")]},
    "targetsame": {"kind": "mnist", "test": SAME, "seeds": [5, 6, 7, 8], "arms": [
        ("born", None, ""), ("hop", 3, HOP + " --g.hop_shift 9"),
        ("coin", 3, HOP + " --g.hop_shift 9 --g.hop_coin 1"),
        ("shuffled", 3, HOP + " --g.hop_shift 9 --life-shuffle 1")]},
    "targetpick": {"kind": "mnist", "test": "--val 1", "seeds": [1, 2], "arms": [
        (a + s, 3, HOP + " --g.hop_shift " + s + (" --g.hop_coin 1" if a == "coin" else ""))
        for s in ("7", "9") for a in ("hop", "coin")]},
    # toys: the shuffled-input control (each step shows another step's lines)
    "toys": {"kind": "toy", "tasks": ["xor", "parity3"], "seeds": list(range(5, 13)), "settings": [
        ("born", None, ""), ("m2", HOP + " --g.hop_teach 2", ""),
        ("coin", HOP + " --g.hop_teach 2 --g.hop_coin 1", ""),
        ("shuffled", HOP + " --g.hop_teach 2", "--life-scramble 1")]},
    # parity of 4 under every arm
    "parity4": {"kind": "toy", "tasks": ["parity4"], "seeds": [5, 6, 7, 8], "settings": [
        ("born", None, ""), ("m2", HOP + " --g.hop_teach 2", ""),
        ("coin", HOP + " --g.hop_teach 2 --g.hop_coin 1", ""),
        ("notfired", HOP + " --g.hop_teach 2 --g.hop_fired 0", ""),
        ("deep", HOP + " --g.hop_teach 2 " + DEEP, ""),
        ("deepborn", None, DEEP),
        ("wide", HOP + " --g.hop_teach 2 " + WIDE, ""),
        ("wideborn", None, WIDE)]},
    # calc held-out columns: shuffled input, depth, width
    "calc": {"kind": "calc", "members": ["xor4", "add4", "parity4"], "seeds": [5, 6], "settings": [
        ("born", None, ""), ("m2", HOP, ""), ("coin", HOP + " --g.hop_coin 1", ""),
        ("shuffled", HOP, "--life-scramble 1"),
        ("deepborn", None, DEEP), ("deep", HOP + " " + DEEP, ""),
        ("deepcoin", HOP + " --g.hop_coin 1 " + DEEP, ""),
        ("wideborn", None, WIDE), ("wide", HOP + " " + WIDE, ""),
        ("widecoin", HOP + " --g.hop_coin 1 " + WIDE, "")]},
}


def run(job):
    cmd, out = job
    if os.path.exists(out):
        return
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out + ".part", "w") as f:
        f.write(" ".join(cmd[3:]) + "\n")
        f.flush()
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT)
    os.replace(out + ".part", out)


def mean(v):
    v = [x for x in v if x is not None]
    return sum(v) / len(v) if v else None


def fmt(v, form="%.4f"):
    return "   -  " if v is None else form % v


# MNIST-sized runs (blobmnist ceiling): a fresh linear reader of the non-sensor cells
def mnist_jobs(r, outdir, quick):
    jobs = []
    life, fit = (3000, 5000) if quick else (60000, 60000)
    for name, teach, flags in r["arms"]:
        for s in r["seeds"]:
            cmd = ["nice", "-n", "19", MNIST, "ceiling"] + inside.SHEET + inside.READER + [
                "--fit", str(fit), "--seed", str(s)]
            if teach is not None:
                cmd += ["--life", str(life)] + inside.RULE + ["--life-teach", str(teach)]
            cmd += r["test"].split() + flags.split()
            cmd += ["--test-count", "2000"] if quick else []
            jobs.append((cmd, os.path.join(outdir, "%s-%d.txt" % (name, s))))
    return jobs


def mnist_read(path):
    """score, rate while read, the last life lines' wiring and readout numbers"""
    if not os.path.exists(path):
        return None
    text = open(path).read()
    m = re.search(r"sensor cells left out\s+\d+\s+([0-9.]+)", text)
    if not m:
        return None
    rate = re.search(r"frozen, fit digits.*?: ([0-9.]+) spikes", text)
    wiring = [l for l in text.splitlines() if "pixels" in l and "born inputs kept" in l]
    px = re.search(r"pixels ([0-9.]+) .*born inputs kept ([0-9.]+).*likeness ([0-9.]+)",
                   wiring[-1]) if wiring else None
    ro = re.findall(r"readout right ([0-9.]+)", text)
    return {"score": float(m.group(1)), "rate": float(rate.group(1)) if rate else None,
            "pixels": float(px.group(1)) if px else None, "kept": float(px.group(2)) if px else None,
            "readout": float(ro[-1]) if ro else None}


def mnist_table(r, outdir, name):
    seeds = r["seeds"]
    lines = ["%s: %s; a fresh linear reader of the non-sensor cells (test digits unless --val), "
             "after a taught life of 60,000 (motor cells put back as born)" % (name, r["test"] or "MNIST"),
             "%-12s %s  %-7s %-6s %-7s %-6s %-6s" % ("arm", " ".join("%-6d" % s for s in seeds),
                                                     "mean", "rate", "pixels", "kept", "life")]
    got = {}
    for arm, _, flags in r["arms"]:
        v = [mnist_read(os.path.join(outdir, "%s-%d.txt" % (arm, s))) for s in seeds]
        got[arm] = v
        cells = " ".join(fmt(x["score"]) if x else "  -   " for x in v)
        avg = lambda k: mean([x[k] for x in v if x])
        lines.append("%-12s %s  %s  %s  %s  %s  %s" % (
            arm, cells, fmt(avg("score")), fmt(avg("rate"), "%4.1f"), fmt(avg("pixels"), "%4.1f"),
            fmt(avg("kept"), "%4.2f"), fmt(avg("readout"), "%5.3f")))
    lines.append("(rate: hidden spikes per 1,000 cell-ticks while read; pixels: pixel inputs per "
                 "hidden excitatory cell; kept: born inputs kept; life: the readout's share right "
                 "in the last tenth of the life)")
    # differences, seed by seed: each hop arm against born, its coin, shuffled and off; both
    # against competitive learning alone and against both with the coin
    pairs = [(ref, arm) for ref, _, _ in r["arms"] if ref.startswith("hop")
             for arm, _, _ in r["arms"]
             if arm in ("born", "coin", "coin2", "shuffled", "off") or arm == ref.replace("hop", "coin")]
    pairs += [("both", "compete"), ("both", "bothcoin"), ("bothcoin", "compete")]
    for ref, arm in pairs:
        if ref not in got or arm not in got or ref == arm:
            continue
        d = [a["score"] - b["score"] for a, b in zip(got[ref], got[arm]) if a and b]
        if d:
            lines.append("%s - %-9s %+.4f  (%d of %d seeds above; %+.4f to %+.4f)" % (
                ref, arm, sum(d) / len(d), sum(x > 0 for x in d), len(d), min(d), max(d)))
    return "\n".join(lines)


# the bar: plain learners on each pair test, and the born sheet
def bar_jobs(outdir, quick):
    jobs = []
    for test, flags in (("same", SAME), ("sum", SUM), ("same-side", "--pair 2 --pair-mix 0"),
                        ("sum-mix", "--pair 1 --pair-mix 1")):
        for passes in ("1", "5"):
            jobs.append((["nice", "-n", "19", MNIST, "baselines", "--mnist-code", "t28",
                          "--net-passes", passes] + flags.split() +
                         (["--train", "5000"] if quick else []),
                         os.path.join(outdir, "base-%s-net%s.txt" % (test, passes))))
        jobs.append((["nice", "-n", "19", MNIST, "ceiling"] + inside.SHEET + inside.READER +
                     ["--fit", "5000" if quick else "60000", "--seed", "1"] + flags.split(),
                     os.path.join(outdir, "born-%s-1.txt" % test)))
    return jobs


def bar_table(outdir):
    lines = ["the bar and the room: test digits, 60,000 training pairs; born sheet seed 1",
             "%-10s %8s %8s %8s %8s %8s" % ("test", "linear", "born", "random", "net 1", "net 5")]
    for test in ("same", "sum", "same-side", "sum-mix"):
        row = {}
        for passes in ("1", "5"):
            p = os.path.join(outdir, "base-%s-net%s.txt" % (test, passes))
            text = open(p).read() if os.path.exists(p) else ""
            for key, pat in (("linear", r"linear, 3 passes\s+([0-9.]+)"),
                             ("net" + passes, r"two-layer, 100 hidden, \d+ pass\s+([0-9.]+)"),
                             ("random", r"random projection \S+ x \S+ linear\s+([0-9.]+)")):
                m = re.search(pat, text)
                row[key] = float(m.group(1)) if m else row.get(key)
        b = mnist_read(os.path.join(outdir, "born-%s-1.txt" % test))
        lines.append("%-10s %8s %8s %8s %8s %8s" % (
            test, fmt(row.get("linear")), fmt(b["score"] if b else None), fmt(row.get("random")),
            fmt(row.get("net1")), fmt(row.get("net5"))))
    lines.append("same: are two interleaved half-size digits the same class (half are); sum: is "
                 "the sum of two side-by-side digits odd; -side / -mix the other layout")
    return "\n".join(lines)


# toys: a fresh output on the sheet after a 40,000-step life (blobprobe transfer)
def toy_jobs(r, outdir, quick):
    jobs = []
    life = hop.LIFE.replace("--steps 40000", "--steps 4000") if quick else hop.LIFE
    for task in r["tasks"]:
        for name, hop_flags, probe in r["settings"]:
            for s in r["seeds"]:
                born = hop_flags is None
                text = "%s %s %s %s" % (hop.TASKS[task], life, hop.READOUT,
                                        (hop_flags or "") + " " + (probe if born else ""))
                cmd = ["nice", "-n", "19", PROBE, "transfer", "--life", " ".join(text.split()),
                       "--then", hop.TASKS[task].split()[1], "--then-steps",
                       "2000" if quick else "20000", "--seed", str(s)]
                cmd += (["--born", "1"] if born else probe.split())
                if quick:
                    cmd += ["--window", "100"]
                jobs.append((cmd, os.path.join(outdir, "toy-%s-%s-%d.txt" % (task, name, s))))
    return jobs


def toy_table(r, outdir):
    seeds = r["seeds"]
    lines = ["fresh output on the sheet after a 40,000-step life, late score; seeds %s" %
             ",".join(map(str, seeds))]
    for task in r["tasks"]:
        lines.append("%-8s %-9s %s   mean (first quarter)" % (task, "setting",
                                                            " ".join("%-6d" % s for s in seeds)))
        for name, _, _ in r["settings"]:
            v = [hop.toy_score(os.path.join(outdir, "toy-%s-%s-%d.txt" % (task, name, s)))
                 for s in seeds]
            got = [x for x in v if x]
            cells = " ".join("%.3f " % x[1] if x else "  -   " for x in v)
            avg = ("%.3f (%.3f)" % (mean([x[1] for x in got]), mean([x[0] for x in got]))
                   if got else "-")
            lines.append("%-8s %-9s %s   %s" % ("", name, cells, avg))
    return "\n".join(lines)


def calc_jobs(r, outdir, quick):
    jobs = []
    life = 20000 if quick else 400000
    for mem in r["members"]:
        for name, hop_flags, probe in r["settings"]:
            for s in r["seeds"]:
                out = os.path.join(outdir, "calc-%s-%s-%d.txt" % (mem, name, s))
                cmd = (["nice", "-n", "19", PROBE, "calc"] + hop.CALC_BASE.split() +
                       ["--task", "calc"] + hop.CALC[mem].split() +
                       ["--seed", str(s), "--task-seed", str(s), "--trials",
                        "500" if quick else "3000", "--label", mem, "--out", out[:-4] + ".csv"])
                if hop_flags is not None:
                    cmd += ["--life", str(life), "--life-teach", "2"] + hop.READOUT.split()
                    cmd += hop_flags.split() + probe.split()
                else:
                    cmd += probe.split()
                jobs.append((cmd, out))
    return jobs


def calc_table(r, outdir):
    seeds = r["seeds"]
    lines = ["calc: ridge reader on the frozen sheet, bits right seen / held-out; taught life "
             "400,000 ticks (told the answer, a verdict per bit), motor cells put back as born",
             "%-8s %-9s %s  mean held" % ("member", "setting",
                                           "  ".join("seed %-9d" % s for s in seeds))]
    for mem in r["members"]:
        for name, _, _ in r["settings"]:
            row, held = [], []
            for s in seeds:
                d = hop.calc_score(os.path.join(outdir, "calc-%s-%s-%d.csv" % (mem, name, s)))
                if ("sheet", "bits_seen") in d:
                    row.append("%.3f / %.3f" % (d[("sheet", "bits_seen")], d[("sheet", "bits_held")]))
                    held.append(d[("sheet", "bits_held")])
                else:
                    row.append("     -       ")
            lines.append("%-8s %-9s %s  %s" % (mem, name, "  ".join(row), fmt(mean(held), "%.3f")))
    return "\n".join(lines)


def main():
    args = sys.argv[1:]
    if not args or args[0] not in RECIPES:
        print(open(os.path.join(ROOT, "scripts", "hop2.sh")).read().split("set -")[0])
        print("tables: " + " ".join(RECIPES))
        return 1
    name = args.pop(0)
    r = dict(RECIPES[name])
    quick, table_only, jobs_at_once = False, False, 3
    i = 0
    while i < len(args):
        if args[i] == "--quick":
            quick = True
        elif args[i] == "--table-only":
            table_only = True
        elif args[i] == "--jobs":
            jobs_at_once = int(args[i + 1])
            i += 1
        elif args[i] == "--seeds":
            lo_hi = args[i + 1].split("-")
            r["seeds"] = (list(range(int(lo_hi[0]), int(lo_hi[1]) + 1)) if len(lo_hi) == 2
                          else [int(x) for x in args[i + 1].split(",")])
            i += 1
        elif args[i] == "--arms":
            keep = args[i + 1].split(",")
            key = "arms" if "arms" in r else "settings"
            r[key] = [a for a in r[key] if a[0] in keep]
            i += 1
        else:
            print("hop2: unknown option %s" % args[i])
            return 1
        i += 1
    if quick and "seeds" in r:
        r["seeds"] = r["seeds"][:1]
    outdir = os.path.join(ROOT, "out", "hop2", name + ("-quick" if quick else ""))
    os.makedirs(outdir, exist_ok=True)
    kind = r["kind"]
    if kind == "mnist":
        jobs, table = mnist_jobs(r, outdir, quick), lambda: mnist_table(r, outdir, name)
    elif kind == "bar":
        jobs, table = bar_jobs(outdir, quick), lambda: bar_table(outdir)
    elif kind == "toy":
        jobs, table = toy_jobs(r, outdir, quick), lambda: toy_table(r, outdir)
    else:
        jobs, table = calc_jobs(r, outdir, quick), lambda: calc_table(r, outdir)
    if not table_only:
        with ThreadPoolExecutor(max_workers=jobs_at_once) as pool:
            list(pool.map(run, jobs))
    text = table()
    print(text)
    open(os.path.join(outdir, "table.txt"), "w").write(text + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
