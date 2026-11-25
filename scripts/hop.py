#!/usr/bin/env python3
"""The error sent one hop (docs/results-2026-11-25-hop.md). See scripts/hop.sh."""

import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, os.environ.get("HOP_BUILD", "build"))
PROBE = os.path.join(BUILD, "blobprobe")

# the tags job's toy life and readout (scripts/tags.sh)
LIFE = "--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000 --motor-spread 128"
READOUT = "--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1"
TASKS = {
    "xor": "--task both-not-either",
    "parity3": "--task points --fn parity --fn-lines 3",
    "parity4": "--task points --fn parity --fn-lines 4",
}
# the rule's settings: name -> life flags (beside the readout)
HOP = "--g.hop_on 1"
SETTINGS = {
    "born": None,
    "off": "",
    "m2": HOP + " --g.hop_teach 2",
    "m1": HOP + " --g.hop_teach 1",
    "m1good": HOP + " --g.hop_teach 1 --g.hop_good 1",
    "m2coin": HOP + " --g.hop_teach 2 --g.hop_coin 1",
    "m1coin": HOP + " --g.hop_teach 1 --g.hop_coin 1",
}
# calc: the battery's probe, a verdict per bit, the working rule as the readout
CALC_BASE = "--ticks 20 --sensor-side 3 --motor-side 3 --motor-spread 128 --calc-grade bit"
CALC = {
    "xor4": "--calc xor --calc-bits 4",
    "parity4": "--calc parity --calc-bits 4",
    "add4": "--calc add --calc-bits 4",
}


def run(job):
    cmd, out = job
    if os.path.exists(out):
        return
    with open(out + ".part", "w") as f:
        f.write(" ".join(cmd) + "\n")
        f.flush()
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT)
    os.replace(out + ".part", out)


def toy_jobs(outdir, tasks, settings, seeds, extra):
    jobs = []
    for task in tasks:
        for name in settings:
            for s in seeds:
                hop = SETTINGS[name]
                life = "%s %s %s %s %s" % (TASKS[task], LIFE, READOUT, hop or "", extra)
                cmd = ["nice", "-n", "19", PROBE, "transfer", "--life", life.strip(),
                       "--then", TASKS[task].split()[1], "--then-steps", "20000",
                       "--seed", str(s)] + (["--born", "1"] if hop is None else [])
                # points tasks: the fresh readout keeps the life's fn flags
                jobs.append((cmd, os.path.join(outdir, "toy-%s-%s-%d.txt" % (task, name, s))))
    return jobs


def toy_score(path):
    if not os.path.exists(path):
        return None
    m = re.search(r"first quarter ([0-9.]+) late ([0-9.]+)", open(path).read())
    return (float(m.group(1)), float(m.group(2))) if m else None


def toy_table(outdir, tasks, settings, seeds):
    lines = ["fresh output on the sheet after a 40,000-step life (late = mean of the last two "
             "of 20 windows; first quarter in brackets); seeds %s" % ",".join(map(str, seeds))]
    for task in tasks:
        lines.append("")
        lines.append("%-8s %-8s %s   mean late (first quarter)" % (
            task, "setting", " ".join("%-6d" % s for s in seeds)))
        for name in settings:
            v = [toy_score(os.path.join(outdir, "toy-%s-%s-%d.txt" % (task, name, s)))
                 for s in seeds]
            got = [x for x in v if x]
            cells = " ".join("%.3f " % x[1] if x else "  -   " for x in v)
            mean = ("%.3f (%.3f)" % (sum(x[1] for x in got) / len(got),
                                     sum(x[0] for x in got) / len(got))) if got else "-"
            lines.append("%-8s %-8s %s   %s" % ("", name, cells, mean))
    return "\n".join(lines)


def calc_jobs(outdir, members, settings, seeds, life, extra):
    jobs = []
    for mem in members:
        for name in settings:
            for s in seeds:
                hop = SETTINGS[name]
                out = os.path.join(outdir, "calc-%s-%s-%d.txt" % (mem, name, s))
                cmd = (["nice", "-n", "19", PROBE, "calc"] + CALC_BASE.split() + ["--task", "calc"]
                       + CALC[mem].split() + ["--seed", str(s), "--task-seed", str(s),
                                              "--trials", "3000", "--label", mem,
                                              "--out", out[:-4] + ".csv"])
                if hop is not None:
                    cmd += ["--life", str(life), "--life-teach",
                            "2" if "m2" in name else "1"] + READOUT.split() + hop.split()
                jobs.append((cmd + extra.split(), out))
    return jobs


def calc_score(path):
    out = {}
    if os.path.exists(path):
        for line in open(path).read().splitlines()[1:]:
            p = line.split(",")
            if len(p) == 3:
                out[(p[0], p[1])] = float(p[2])
    return out


def calc_table(outdir, members, settings, seeds, life):
    lines = ["calc: ridge reader on the frozen sheet, bits right seen / held-out; taught life %d "
             "ticks (a verdict per bit), motor cells put back as born" % life,
             "member    setting  " + "  ".join("seed %-9d" % s for s in seeds) + "  mean held"]
    for mem in members:
        for name in settings:
            row, held = [], []
            for s in seeds:
                d = calc_score(os.path.join(outdir, "calc-%s-%s-%d.csv" % (mem, name, s)))
                if ("sheet", "bits_seen") in d:
                    row.append("%.3f / %.3f" % (d[("sheet", "bits_seen")], d[("sheet", "bits_held")]))
                    held.append(d[("sheet", "bits_held")])
                else:
                    row.append("     -       ")
            lines.append("%-9s %-8s %s  %s" % (mem, name, "  ".join(row),
                                                "%.3f" % (sum(held) / len(held)) if held else "-"))
    return "\n".join(lines)


def seeds_of(text):
    lo_hi = text.split("-")
    return (list(range(int(lo_hi[0]), int(lo_hi[1]) + 1)) if len(lo_hi) == 2
            else [int(x) for x in text.split(",")])


def main():
    args = sys.argv[1:]
    if len(args) < 2:
        print(open(os.path.join(ROOT, "scripts", "hop.sh")).read().split("set -")[0])
        return 1
    what, name = args.pop(0), args.pop(0)
    opt = {"seeds": [5, 6, 7, 8], "jobs": 3, "tasks": list(TASKS), "members": list(CALC),
           "settings": None, "life": 200000, "table_only": False}
    extra = []
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--seeds":
            opt["seeds"] = seeds_of(args[i + 1])
        elif a in ("--jobs", "--life"):
            opt[a[2:]] = int(args[i + 1])
        elif a in ("--tasks", "--members", "--settings"):
            opt[a[2:]] = args[i + 1].split(",")
        elif a == "--table-only":
            opt["table_only"] = True
            i += 1
            continue
        else:
            extra.append(a)
            i += 1
            continue
        i += 2
    outdir = os.path.join(ROOT, "out", "hop", name)
    os.makedirs(outdir, exist_ok=True)
    flag_file = os.path.join(outdir, "flags.txt")
    if os.path.exists(flag_file) and open(flag_file).read().strip() != " ".join(extra):
        print("out/hop/%s was made with other flags: %s" % (name, open(flag_file).read()))
        return 1
    open(flag_file, "w").write(" ".join(extra) + "\n")
    if what == "toy":
        settings = opt["settings"] or ["born", "off", "m2", "m1", "m2coin"]
        jobs = toy_jobs(outdir, opt["tasks"], settings, opt["seeds"], " ".join(extra))
        table = lambda: toy_table(outdir, opt["tasks"], settings, opt["seeds"])
    elif what == "calc":
        settings = opt["settings"] or ["born", "off", "m2", "m1", "m2coin"]
        jobs = calc_jobs(outdir, opt["members"], settings, opt["seeds"], opt["life"], " ".join(extra))
        table = lambda: calc_table(outdir, opt["members"], settings, opt["seeds"], opt["life"])
    else:
        print("scripts/hop.sh toy|calc NAME ...")
        return 1
    if not opt["table_only"]:
        with ThreadPoolExecutor(max_workers=opt["jobs"]) as pool:
            list(pool.map(run, jobs))
    text = table()
    print(text)
    open(os.path.join(outdir, "table.txt"), "w").write("flags: %s\n\n%s\n" % (" ".join(extra), text))
    return 0


if __name__ == "__main__":
    sys.exit(main())
