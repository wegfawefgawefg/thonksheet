#!/usr/bin/env python3
"""The calculation battery (ideas 133, 134; docs/curriculum.md "Calculation").

    scripts/calc.py NAME [--quick] [--jobs N] [--seeds 5-12] [--only fam,fam] [--configs a,b]
                    [--table-only] [blobrun flags and --g.name value ...]

Every member is a `blobrun --task calc` world whose answer is a pattern of bits. Each member
is scored three ways (configs):

  probe    blobprobe calc: a ridge reader per output bit, fitted to the frozen sheet as
           born (and to the input lines alone), on seen cases, scored on later trials of
           seen cases and on held-out cases
  bit      the working rule (learn_mode 5, frozen sheet), a verdict per output bit; with the
           learning-off twin and coin flips beside it
  whole    the same rule, one good or bad for the whole answer
  good     the same rule with readout_good 1 (good also moves), whole answer
  m6       learn_mode 6 (the sheet learns too), a verdict per bit
  minimal  the minimal engine, whole answer (it reads no marks), with its own twin
  table    table neurons, whole answer, with their own twin

`probe` and `bit` run on every member; the rest on the first size of each family (marked
* in the table) unless --configs names them. Runs go to out/calc/NAME/, at most --jobs
(2) at a time under nice; finished runs are kept, so a second call only runs what is
missing (--table-only just prints). Everything not recognised is appended to every call.
"""

import os
import re
import statistics
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLOBRUN = os.path.join(ROOT, "build", "blobrun")
BLOBPROBE = os.path.join(ROOT, "build", "blobprobe")

BASE = ["--ticks", "20", "--sensor-side", "3", "--motor-side", "3", "--motor-spread", "128"]
WORKING = ["--g.learn_mode", "5", "--g.readout_evidence", "1", "--g.readout_shift", "4",
           "--g.readout_balance", "1"]
FULL = {"steps": 40000, "window": 1000, "trials": 3000, "seeds": list(range(5, 13))}
QUICK = {"steps": 4000, "window": 200, "trials": 1000, "seeds": [5, 6]}


def member(fam, name, args, first=False):
    return dict(fam=fam, name=name, args=["--task", "calc"] + args, first=first)


def members():
    m = []

    def ladder(fam, op, sizes, extra=()):
        for i, n in enumerate(sizes):
            m.append(member(fam, "%s%d" % (op, n), ["--calc", op, "--calc-bits", str(n)] +
                            list(extra), i == 0))
    ladder("copy", "copy", [2, 4, 8])
    ladder("arith", "add", [2, 3, 4, 6, 8])
    ladder("arith", "sub", [2, 4, 8])
    ladder("arith", "mul", [2, 3, 4])
    ladder("arith", "inc", [2, 4, 8])
    ladder("arith", "compare", [2, 4, 8])
    for op in ("and", "or", "xor", "not"):
        ladder("bitops", op, [4])
    ladder("bitops", "xor", [8])
    m[-1]["first"] = False
    ladder("bitops", "shl", [4, 8])
    ladder("bitops", "rotl", [4])
    ladder("bitops", "ones", [4, 8])
    ladder("bitops", "parity", [2, 4, 8])
    ladder("alu", "alu4", [2, 4])
    ladder("alu", "alu8", [2, 4])
    for i, n in enumerate([1, 2, 3, 4, 6]):
        m.append(member("program", "program3x%d" % n, ["--calc", "program", "--calc-bits", "3",
                                                         "--calc-len", str(n)], i == 0))
    ladder("lookup", "max", [2, 4])
    ladder("lookup", "sort3", [2])
    m.append(member("lookup", "kth2x4", ["--calc", "kth", "--calc-bits", "2",
                                          "--calc-list", "4"], True))
    for op, size in (("turn", 4), ("mirror", 4), ("move", 4), ("scale", 4), ("turn3d", 3),
                     ("move3d", 3), ("angle", 2), ("turn3", 2)):
        m.append(member("geometry", "%s%d" % (op, size), ["--calc", op, "--calc-size",
                                                           str(size)], op == "turn"))
    return m


CONFIGS = {
    # name: (program, flags, baselines)
    "probe": ("probe", [], False),
    "bit": ("run", WORKING + ["--calc-grade", "bit"], True),
    "whole": ("run", WORKING + ["--calc-grade", "whole"], False),
    "good": ("run", WORKING + ["--g.readout_good", "1", "--calc-grade", "whole"], False),
    "m6": ("run", ["--g.learn_mode", "6", "--g.readout_evidence", "1", "--g.readout_shift",
                   "4", "--g.readout_balance", "1", "--calc-grade", "bit"], False),
    "minimal": ("run", ["--engine", "minimal", "--calc-grade", "whole"], True),
    "table": ("run", ["--table", "--calc-grade", "whole"], True),
}
FIRST_ONLY = {"whole", "good", "m6", "minimal", "table"}


def read_sum(path):
    out = {}
    if os.path.exists(path):
        with open(path) as f:
            next(f, None)
            for line in f:
                p = line.strip().split(",")
                if len(p) == 3:
                    out.setdefault(p[0], {})[p[1]] = float(p[2])
    return out


def command(mem, config, seed, life, stem, extra):
    prog, flags, baselines = CONFIGS[config]
    if prog == "probe":
        return (["nice", "-n", "19", BLOBPROBE, "calc"] + BASE + mem["args"] +
                ["--seed", str(seed), "--task-seed", str(seed), "--trials",
                 str(life["trials"]), "--label", mem["name"], "--out", stem + "-probe.csv"] +
                extra)
    cmd = (["nice", "-n", "19", BLOBRUN] + BASE + mem["args"] + flags +
           ["--steps", str(life["steps"]), "--window", str(life["window"]), "--seed", str(seed),
            "--task-seed", str(seed), "--alive-ticks", "0", "--rhythm-ticks", "0",
            "--out", stem + ".csv"] + extra)
    if not baselines:
        cmd.insert(cmd.index("--steps"), "--no-baselines")
    return cmd


def done_path(config, stem):
    return stem + ("-probe.csv" if config == "probe" else "-sum.csv")


def mean(v):
    return statistics.mean(v) if v else None


def collect(mems, configs, seeds, outdir):
    """member name -> config -> key -> mean over seeds (and 'n')"""
    res = {}
    for mem in mems:
        for config in configs:
            per = {}
            for seed in seeds:
                stem = os.path.join(outdir, "%s-%s-s%d" % (mem["name"], config, seed))
                if config == "probe":
                    d = read_sum(done_path(config, stem))
                    rows = {"sheet." + k: v for k, v in d.get("sheet", {}).items()}
                    rows.update({"inputs." + k: v for k, v in d.get("inputs", {}).items()})
                else:
                    d = read_sum(done_path(config, stem))
                    rows = {}
                    for driver, keys in d.items():
                        for k, v in keys.items():
                            rows[driver + "." + k] = v
                for k, v in rows.items():
                    per.setdefault(k, []).append(v)
            res.setdefault(mem["name"], {})[config] = {k: mean(v) for k, v in per.items()}
            res[mem["name"]][config]["n"] = max((len(v) for v in per.values()), default=0)
    return res


def cell(d, key, width=5):
    v = d.get(key) if d else None
    return ("%.2f" % v).rjust(width) if v is not None else "-".rjust(width)


def print_table(mems, res, configs, out):
    head = ("member      in/out | probe inputs  | probe sheet   |  bit: seen held whole twin "
            "coin | whole good |  m6 seen held | minimal twin | table twin")
    lines = [head, "-" * len(head)]
    fam = None
    for mem in mems:
        r = res[mem["name"]]
        if mem["fam"] != fam:
            fam = mem["fam"]
            lines.append("-- " + fam)
        pr, b = r.get("probe", {}), r.get("bit", {})
        shape = layout_of(mem)
        lines.append(
            "%-11s %6s | %s %s | %s %s | %s %s %s %s %s | %s %s | %s %s | %s %s | %s %s   %s" % (
                mem["name"] + ("*" if mem["first"] else ""), shape,
                cell(pr, "inputs.bits_seen"), cell(pr, "inputs.bits_held"),
                cell(pr, "sheet.bits_seen"), cell(pr, "sheet.bits_held"),
                cell(b, "blob.late"), cell(b, "blob.bits_held_late"),
                cell(b, "blob.whole_seen_late"), cell(b, "learn_off.late"),
                cell(b, "coin.late"),
                cell(r.get("whole", {}), "blob.late"), cell(r.get("good", {}), "blob.late"),
                cell(r.get("m6", {}), "blob.late"), cell(r.get("m6", {}), "blob.bits_held_late"),
                cell(r.get("minimal", {}), "blob.late"),
                cell(r.get("minimal", {}), "learn_off.late"),
                cell(r.get("table", {}), "table.late"), cell(r.get("table", {}), "table_off.late"),
                "n=%d" % b.get("n", 0)))
    lines.append("")
    lines.append("each output bit, seen cases: probe sheet / working rule (bit grading) / twin")
    for mem in mems:
        r = res[mem["name"]]
        pr, b = r.get("probe", {}), r.get("bit", {})
        bits = []
        for i in range(32):
            k = "bit%d_seen" % i
            if ("sheet." + k) not in pr and ("blob." + k + "_late") not in b:
                break
            bits.append("%s/%s/%s" % (cell(pr, "sheet." + k, 4).strip(),
                                      cell(b, "blob." + k + "_late", 4).strip(),
                                      cell(b, "learn_off." + k + "_late", 4).strip()))
        lines.append("%-11s %s" % (mem["name"], "  ".join(bits)))
    text = "\n".join(lines)
    print(text)
    if out:
        with open(out, "w") as f:
            f.write(text + "\n")


LAYOUTS = {}


def layout_of(mem):
    """lines in / bits out, from a one-step stand-in call (cached)"""
    key = " ".join(mem["args"])
    if key not in LAYOUTS:
        r = subprocess.run([BLOBRUN] + BASE + mem["args"] + ["--steps", "1", "--window", "1",
                           "--no-baselines", "--show-plug", "--out", "/dev/null"],
                           capture_output=True, text=True)
        lines_in = bits_out = 0
        for line in r.stdout.splitlines():
            found = re.search(r"(\d+) lines of", line)
            if found and line.startswith("sensor region"):
                lines_in = int(found.group(1)) - 3  # good, bad and ask left out
            if found and line.startswith("motor region"):
                bits_out = int(found.group(1)) // 2
        LAYOUTS[key] = "%d/%d" % (lines_in, bits_out)
    return LAYOUTS[key]


def parse_seeds(text):
    if "-" in text:
        a, b = text.split("-")
        return list(range(int(a), int(b) + 1))
    return [int(s) for s in text.split(",")]


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 1
    name, rest = args[0], args[1:]
    quick = "--quick" in rest
    table_only = "--table-only" in rest
    jobs, seeds, only, configs, extra = 2, None, None, None, []
    i = 0
    while i < len(rest):
        a = rest[i]
        if a in ("--quick", "--table-only"):
            pass
        elif a == "--jobs":
            jobs = int(rest[i + 1])
            i += 1
        elif a == "--seeds":
            seeds = parse_seeds(rest[i + 1])
            i += 1
        elif a == "--only":
            only = rest[i + 1].split(",")
            i += 1
        elif a == "--configs":
            configs = rest[i + 1].split(",")
            i += 1
        else:
            extra.append(a)
        i += 1
    life = QUICK if quick else FULL
    seeds = seeds or life["seeds"]
    mems = [m for m in members() if not only or m["fam"] in only or m["name"] in only]
    explicit = configs is not None
    configs = configs or ["probe", "bit", "whole", "good", "m6", "minimal", "table"]
    outdir = os.path.join(ROOT, "out", "calc", name + ("-quick" if quick else ""))
    os.makedirs(outdir, exist_ok=True)

    todo = []
    for mem in mems:
        for config in configs:
            if config in FIRST_ONLY and not mem["first"] and not explicit:
                continue
            for seed in seeds:
                stem = os.path.join(outdir, "%s-%s-s%d" % (mem["name"], config, seed))
                if not os.path.exists(done_path(config, stem)):
                    todo.append((mem, config, seed, stem))
    if not table_only and todo:
        print("calc %s: %d runs, %d at a time, seeds %s" % (name, len(todo), jobs, seeds),
              flush=True)
        started = time.time()
        failed = []

        def launch(item):
            mem, config, seed, stem = item
            cmd = command(mem, config, seed, life, stem, extra)
            with open(stem + "-%s.txt" % ("probe" if config == "probe" else "run"), "w") as f:
                f.write(" ".join(cmd) + "\n")
                f.flush()
                return item, subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT).returncode

        done = 0
        with ThreadPoolExecutor(max_workers=jobs) as pool:
            for (mem, config, seed, _), code in pool.map(launch, todo):
                done += 1
                if code != 0:
                    failed.append("%s-%s-s%d" % (mem["name"], config, seed))
                if done % 10 == 0 or done == len(todo):
                    print("  %d of %d done, %.0f min" % (done, len(todo),
                                                       (time.time() - started) / 60), flush=True)
        if failed:
            print("FAILED: " + " ".join(failed))
    res = collect(mems, configs, seeds, outdir)
    print_table(mems, res, configs, os.path.join(outdir, "table.txt"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
