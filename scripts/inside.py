#!/usr/bin/env python3
"""The inside-learning test (docs/results-2026-11-25-compete.md). See scripts/inside.sh."""

import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, os.environ.get("INSIDE_BUILD", "build"))
MNIST = os.path.join(BUILD, "blobmnist")
PROBE = os.path.join(BUILD, "blobprobe")

# the MNIST sheet and reader of scripts/mnist.sh (the born sheet reads 0.924 there)
SHEET = ("--mnist-code t28 --g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1 "
         "--g.motor_w 64 --g.motor_h 64 --motor-spread 102 --g.local_connect_shift 4").split()
READER = "--scale 2 --passes 10 --lr-1000 1 --views 1".split()
# the calculation battery's probe (scripts/calc.py)
CALC_BASE = "--ticks 20 --sensor-side 3 --motor-side 3 --motor-spread 128".split()
CALC = {
    "xor4": "--calc xor --calc-bits 4",
    "parity4": "--calc parity --calc-bits 4",
    "add4": "--calc add --calc-bits 4",
}


def mnist_cmd(kind, seed, opt, flags, out):
    cmd = ["nice", "-n", "19", MNIST, "ceiling"] + SHEET + READER + [
        "--fit", str(opt["fit"]), "--seed", str(seed)]
    if kind != "born":
        cmd += ["--pretrain", str(opt["life"])]
    if kind == "shuffled":
        cmd += ["--pretrain-shuffle", "1"]
    return cmd + opt.get("mnist_extra", []) + flags, out


def calc_cmd(member, kind, seed, opt, flags, out):
    cmd = (["nice", "-n", "19", PROBE, "calc"] + CALC_BASE + ["--task", "calc"] +
           CALC[member].split() + ["--seed", str(seed), "--task-seed", str(seed),
                                   "--trials", str(opt["trials"]), "--label", member,
                                   "--out", out[:-4] + ".csv"])
    if kind == "lived":
        cmd += ["--life", str(opt["calc_life"])]
    return cmd + flags, out


def run(job):
    cmd, out = job
    if os.path.exists(out) and "total wall" in open(out).read() or os.path.exists(
            out[:-4] + ".csv"):
        return
    with open(out + ".part", "w") as f:
        f.write(" ".join(cmd[3:]) + "\n")
        f.flush()
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT)
    os.replace(out + ".part", out)


def mnist_score(path):
    if not os.path.exists(path):
        return None, "", ""
    text = open(path).read()
    m = re.search(r"sensor cells left out\s+\d+\s+([0-9.]+)", text)
    lives = [l for l in text.splitlines() if l.startswith("life ")]
    born = [l for l in text.splitlines() if l.startswith("as born")]
    frozen = [l for l in text.splitlines() if l.startswith("frozen")]
    wiring = (lives[-1] if lives else born[0] if born else "")
    return (float(m.group(1)) if m else None), wiring, (frozen[0] if frozen else "")


def calc_score(path):
    out = {}
    if os.path.exists(path):
        for line in open(path).read().splitlines()[1:]:
            p = line.split(",")
            if len(p) == 3:
                out[(p[0], p[1])] = float(p[2])
    return out


def fmt(v):
    return "  -  " if v is None else "%.4f" % v


def table(opt, outdir):
    lines = []
    if "mnist" in opt["parts"]:
        scored = "validation" if "--val" in opt.get("mnist_extra", []) else "test digits"
        lines += ["MNIST, %s: a fresh linear reader of the non-sensor cells, fit on %d "
                  "training digits; life %d digits, no reward" % (scored, opt["fit"], opt["life"]),
                  "seed   born    lived   shuffled  lived-born  lived-shuffled"]
        rows = []
        for s in opt["seeds"]:
            v = [mnist_score(os.path.join(outdir, "mnist-%s-%d.txt" % (k, s)))[0]
                 for k in ("born", "lived", "shuffled")]
            rows.append(v)
            d1 = v[1] - v[0] if None not in v[:2] else None
            d2 = v[1] - v[2] if None not in v[1:] else None
            lines.append("%-5d %s  %s  %s    %s      %s" % (
                s, fmt(v[0]), fmt(v[1]), fmt(v[2]),
                "  -  " if d1 is None else "%+.4f" % d1, "  -  " if d2 is None else "%+.4f" % d2))
        full = [r for r in rows if None not in r]
        if full:
            mean = [sum(r[i] for r in full) / len(full) for i in range(3)]
            lines.append("mean  %s  %s  %s    %+.4f      %+.4f   (%d seeds)" % (
                fmt(mean[0]), fmt(mean[1]), fmt(mean[2]), mean[1] - mean[0], mean[1] - mean[2],
                len(full)))
        base = os.path.join(ROOT, "out", "inside", "baselines.txt")
        if os.path.exists(base):
            for line in open(base).read().splitlines():
                if line.startswith(("linear", "two-layer", "random")):
                    lines.append("baseline  " + line[:45].rstrip())
        s = opt["seeds"][0]
        lines.append("wiring and activity, seed %d (last life line; frozen fit digits):" % s)
        for k in ("born", "lived", "shuffled"):
            _, w, f = mnist_score(os.path.join(outdir, "mnist-%s-%d.txt" % (k, s)))
            lines.append("  %-9s %s" % (k, w))
            lines.append("  %-9s %s" % ("", f))
        lines.append("")
    if "calc" in opt["parts"]:
        lines += ["calc: ridge reader on the frozen sheet, bits right seen / held-out; life %d "
                  "ticks, no reward" % opt["calc_life"],
                  "member    seed   inputs        born          lived         lived-born (held)"]
        for mem in opt["members"]:
            for s in opt["seeds_calc"]:
                b = calc_score(os.path.join(outdir, "calc-%s-born-%d.csv" % (mem, s)))
                l = calc_score(os.path.join(outdir, "calc-%s-lived-%d.csv" % (mem, s)))
                cell = lambda d, r: ("%.3f / %.3f" % (d[(r, "bits_seen")], d[(r, "bits_held")])
                                     if (r, "bits_seen") in d else "     -     ")
                diff = ("%+.3f" % (l[("sheet", "bits_held")] - b[("sheet", "bits_held")])
                        if ("sheet", "bits_held") in l and ("sheet", "bits_held") in b else "-")
                lines.append("%-9s %-5d  %s  %s  %s  %s" % (mem, s, cell(b, "inputs"),
                                                            cell(b, "sheet"), cell(l, "sheet"), diff))
    return "\n".join(lines)


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(open(os.path.join(ROOT, "scripts", "inside.sh")).read().split("set -")[0])
        return 1
    name = args.pop(0)
    opt = {"seeds": [1, 2], "seeds_calc": [5, 6], "life": 60000, "fit": 60000, "trials": 3000,
           "calc_life": 200000, "jobs": 3, "parts": ["mnist", "calc"],
           "members": list(CALC), "table_only": False}
    flags = []
    i = 0
    while i < len(args):
        a = args[i]
        if a in ("--seeds", "--calc-seeds"):
            lo_hi = args[i + 1].split("-")
            seeds = (list(range(int(lo_hi[0]), int(lo_hi[1]) + 1)) if len(lo_hi) == 2
                     else [int(x) for x in args[i + 1].split(",")])
            opt["seeds" if a == "--seeds" else "seeds_calc"] = seeds
            i += 2
        elif a in ("--life", "--fit", "--trials", "--calc-life", "--jobs"):
            opt[a[2:].replace("-", "_")] = int(args[i + 1])
            i += 2
        elif a == "--only":
            opt["parts"] = args[i + 1].split(",")
            i += 2
        elif a == "--members":
            opt["members"] = args[i + 1].split(",")
            i += 2
        elif a in ("--val", "--test-count"):
            # MNIST only: validation slice, fewer test digits (quick checks)
            opt.setdefault("mnist_extra", []).extend([a, args[i + 1]])
            i += 2
        elif a == "--table-only":
            opt["table_only"] = True
            i += 1
        else:
            flags.append(a)
            i += 1
    outdir = os.path.join(ROOT, "out", "inside", name)
    os.makedirs(outdir, exist_ok=True)
    flag_file = os.path.join(outdir, "flags.txt")
    if os.path.exists(flag_file) and open(flag_file).read().strip() != " ".join(flags):
        print("out/inside/%s was made with other flags: %s" % (name, open(flag_file).read()))
        return 1
    open(flag_file, "w").write(" ".join(flags) + "\n")
    jobs = []
    if not opt["table_only"]:
        if "mnist" in opt["parts"]:
            base = os.path.join(ROOT, "out", "inside", "baselines.txt")
            if not os.path.exists(base):
                with open(base, "w") as f:
                    subprocess.run(["nice", "-n", "19", MNIST, "baselines", "--mnist-code", "t28"],
                                   stdout=f, cwd=ROOT)
            for s in opt["seeds"]:
                for k in ("born", "lived", "shuffled"):
                    jobs.append(mnist_cmd(k, s, opt, flags,
                                          os.path.join(outdir, "mnist-%s-%d.txt" % (k, s))))
        if "calc" in opt["parts"]:
            for mem in opt["members"]:
                for s in opt["seeds_calc"]:
                    for k in ("born", "lived"):
                        jobs.append(calc_cmd(mem, k, s, opt, flags,
                                             os.path.join(outdir, "calc-%s-%s-%d.txt" % (mem, k, s))))
        with ThreadPoolExecutor(max_workers=opt["jobs"]) as pool:
            list(pool.map(run, jobs))
    text = table(opt, outdir)
    print(text)
    open(os.path.join(outdir, "table.txt"), "w").write("flags: %s\n\n%s\n" % (" ".join(flags), text))
    return 0


if __name__ == "__main__":
    sys.exit(main())
