#!/usr/bin/env python3
"""The function battery (docs/curriculum.md, "The function battery"), every member at three
levels, printed as one table and a ladder.

    scripts/functions.py NAME [--quick] [--levels abc] [--members REGEX] [--seeds 5-12]
                         [--jobs 2] [--table-only] [blobrun flags and --g.name value ...]

Levels, each member on each seed:
  a  blobprobe reader: a linear reader of the sheet as born (learning off), and the same
     reader on the input lines alone
  b  the working readout rule on a frozen sheet (READOUT below), beside its learning-off
     twin and coin flips
  c  the minimal engine (--engine minimal) and table neurons (--table) with their default
     rules, each beside its own learning-off twin
Runs go to out/functions/NAME/<level>/<member>-s<seed>.*; a run whose output is there is
not run again (a new NAME for new flags). Everything after NAME that is not one of the
options above goes to every call, last, so `--g.tiles_w 16 --g.tiles_h 16` grows the sheet.
At most --jobs runs at a time, each under nice -n 19.

A member "gets" it at a level when its late score is at least halfway from chance (one
over the answers that occur) to 1.0, as the suite's capacity row counts. Held-out members
are scored on values never rewarded.
"""

import math
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
READOUT = ["--g.learn_mode", "5", "--g.readout_evidence", "1", "--g.readout_shift", "4",
           "--g.readout_balance", "1", "--motor-spread", "128"]
FULL = {"steps": 40000, "window": 1000, "trials": 3000, "settle": 20000,
        "seeds": list(range(5, 13))}
QUICK = {"steps": 4000, "window": 200, "trials": 800, "settle": 5000, "seeds": [5, 6]}

SINE = [round(2047 * math.sin(2 * math.pi * i / 128)) for i in range(128)]


# --- the members ---

def number_value(fn, x, lines):
    """f at x's centre in [0, 4096), as task_function_number.cpp"""
    u = (2 * x + 1) * 4096 // (2 * lines)
    turn = u * 128 // 4096
    if fn == "step":
        return 0 if u < 2048 else 4095
    if fn == "tent":
        return 4096 - abs(2 * u - 4096)
    if fn == "sine":
        return 2048 + SINE[turn & 127]
    if fn == "cosine":
        return 2048 + SINE[(turn + 32) & 127]
    if fn == "square":
        return u * u // 4096
    return u


def band(value, rng, bands):
    return min(value * bands // rng, bands - 1)


def answers_number(fn, lines, bands):
    bands = 2 if fn == "step" else bands
    return len({band(number_value(fn, x, lines), 4096, bands) for x in range(lines)})


def answers_pair(fn, lines, bands):
    if fn == "compare":
        return 2
    out = set()
    for a in range(lines):
        for b in range(lines):
            if fn == "sum":
                out.add(band(a + b, 2 * lines - 1, bands))
            elif fn == "diff":
                out.add(band(a - b + lines - 1, 2 * lines - 1, bands))
            elif fn == "larger":
                out.add(band(max(a, b), lines, bands))
            else:
                out.add(band(a * b, (lines - 1) ** 2 + 1, bands))
    return len(out)


def member(family, size, label, args, answers, side=3, held=False):
    """one battery member: `size` orders the family's ladder"""
    mid = re.sub(r"[^a-z0-9]+", "-", (family + " " + label).lower()).strip("-")
    return dict(id=mid, family=family, size=size, label=label, args=args, answers=answers,
                side=side, held=held)


def members():
    m = []
    for fn, ns in (("and", (2, 4, 8)), ("or", (2, 4, 8)), ("parity", (2, 3, 4, 5, 6, 8)),
                   ("majority", (3, 4, 5, 6, 8)), ("exactly", (3, 4, 5, 6, 8)),
                   ("table-lin", (3, 4, 5, 6, 8)), ("table-nonlin", (3, 4, 5, 6, 8))):
        for n in ns:
            extra = ["--fn-k", "1"] if fn == "exactly" else []
            name = "exactly-1" if fn == "exactly" else fn
            m.append(member("points " + name, n, "N=%d" % n,
                            ["--task", "points", "--fn", fn, "--fn-lines", str(n)] + extra, 2))
    for n in (2, 3, 4, 6, 8):
        m.append(member("points count", n, "N=%d" % n,
                        ["--task", "points", "--fn", "count", "--fn-lines", str(n)], n + 1))
    for n in (3, 4, 6, 8):
        m.append(member("points first", n, "N=%d" % n,
                        ["--task", "points", "--fn", "first", "--fn-lines", str(n)], n))
    # one number, place-coded on L lines, 2x2 sensor groups (32 lines do not fit at 3x3)
    def num(fn, lines, bands, hold=0):
        args = ["--task", "number", "--fn", fn, "--fn-lines", str(lines), "--fn-bands",
                str(bands)] + (["--fn-hold", str(hold)] if hold else [])
        family = "number " + fn + (" held-out" if hold else "")
        size = bands if fn != "identity" or hold else lines * 100 + bands
        label = "L=%d K=%d" % (lines, 2 if fn == "step" else bands)
        return member(family, size, label, args, answers_number(fn, lines, bands), side=2,
                      held=hold > 0)
    for lines in (8, 16, 32):
        for bands in (2, 4, 8):
            m.append(num("identity", lines, bands))
    m.append(num("step", 16, 2))
    for fn in ("tent", "sine", "cosine", "square"):
        m.append(num(fn, 16, 4))
    for bands in (2, 4):
        m.append(num("identity", 16, bands, hold=3))
    m.append(num("step", 16, 2, hold=3))
    for fn in ("tent", "sine", "cosine", "square"):
        m.append(num(fn, 16, 4, hold=3))
    # two numbers, 8 lines each
    for fn in ("sum", "diff", "larger", "product"):
        for bands in (2, 4):
            m.append(member("two-numbers " + fn, bands, "L=8 K=%d" % bands,
                            ["--task", "two-numbers", "--fn", fn, "--fn-lines", "8",
                             "--fn-bands", str(bands)], answers_pair(fn, 8, bands), side=2))
    m.append(member("two-numbers compare", 2, "L=8",
                    ["--task", "two-numbers", "--fn", "compare", "--fn-lines", "8"], 2, side=2))
    # a short past: items gap blank steps apart
    for fn, items in (("recall", 2), ("same", 2), ("majority", 3), ("parity", 3)):
        for gap in (0, 2):
            m.append(member("past " + fn, gap, "%d items, gap %d" % (items, gap),
                            ["--task", "past", "--fn", fn, "--fn-items", str(items), "--fn-gap",
                             str(gap)], 2))
    return m


QUICK_IDS = {"points-parity-n-2", "points-parity-n-3", "points-majority-n-5",
             "points-table-nonlin-n-4", "points-count-n-4", "number-identity-l-16-k-4",
             "number-sine-held-out-l-16-k-4", "two-numbers-product-l-8-k-2",
             "past-same-2-items-gap-0", "past-parity-3-items-gap-2"}


# --- running ---

def parse_seeds(text):
    seeds = []
    for part in text.split(","):
        if "-" in part:
            lo, hi = part.split("-")
            seeds += list(range(int(lo), int(hi) + 1))
        else:
            seeds.append(int(part))
    return seeds


def base_args(mem, life):
    return ["--ticks", "20", "--sensor-side", str(mem["side"]), "--motor-side", "3"] + mem["args"]


def jobs_for(mem, level, seed, life, extra, outdir):
    """(level key, output file, command) for one member, level and seed"""
    stem = os.path.join(outdir, level, "%s-s%d" % (mem["id"], seed))
    seeding = ["--seed", str(seed), "--task-seed", str(seed)]
    if level == "a":
        cmd = ([BLOBPROBE, "reader"] + base_args(mem, life) + seeding +
               ["--trials", str(life["trials"]), "--settle", str(life["settle"]),
                "--label", mem["id"]] + extra)
        return stem + ".txt", cmd
    life_args = ["--steps", str(life["steps"]), "--window", str(life["window"]),
                 "--alive-ticks", "0", "--rhythm-ticks", "0", "--out", stem + ".csv"]
    engine = {"b": READOUT, "min": ["--engine", "minimal"], "tab": ["--table"]}[level]
    cmd = [BLOBRUN] + base_args(mem, life) + life_args + seeding + engine + extra
    return stem + "-sum.csv", cmd


def run(item):
    done_file, cmd = item
    os.makedirs(os.path.dirname(done_file), exist_ok=True)
    log = re.sub(r"(-sum\.csv|\.txt)$", ".log", done_file)
    with open(log, "w") as f:
        f.write(" ".join(cmd) + "\n")
        f.flush()
        if done_file.endswith(".txt"):
            with open(done_file + ".part", "w") as out:
                code = subprocess.run(["nice", "-n", "19"] + cmd, stdout=out,
                                      stderr=f).returncode
            if code == 0:
                os.replace(done_file + ".part", done_file)
            return code
        return subprocess.run(["nice", "-n", "19"] + cmd, stdout=f,
                              stderr=subprocess.STDOUT).returncode


# --- reading ---

def read_sum(path):
    out = {}
    if not os.path.exists(path):
        return out
    for line in open(path).read().split("\n")[1:]:
        parts = line.split(",")
        if len(parts) == 3:
            out.setdefault(parts[0], {})[parts[1]] = float(parts[2])
    return out


def read_reader(path):
    if not os.path.exists(path):
        return None
    words = open(path).read().split()
    if len(words) < 2 or words[0] != "reader":
        return None
    return {words[i]: float(words[i + 1]) for i in range(2, len(words) - 1, 2)}


def mean(values):
    values = [v for v in values if v is not None]
    return statistics.mean(values) if values else None


def collect(mem, seeds, outdir):
    """level -> column -> mean over seeds (and the per-seed list for b)"""
    out = {}
    a = [read_reader(os.path.join(outdir, "a", "%s-s%d.txt" % (mem["id"], s))) for s in seeds]
    a = [r for r in a if r]
    if a:
        key = "_held" if mem["held"] else ""
        out["a"] = dict(sheet=mean([r["sheet" + key] for r in a]),
                        inputs=mean([r["inputs" + key] for r in a]), n=len(a))
    for level, names in (("b", ("blob", "learn_off")), ("min", ("blob", "learn_off")),
                         ("tab", ("table", "table_off"))):
        sums = [read_sum(os.path.join(outdir, level, "%s-s%d-sum.csv" % (mem["id"], s)))
                for s in seeds]
        sums = [s for s in sums if names[0] in s]
        if sums:
            out[level] = dict(run=mean([s[names[0]]["late"] for s in sums]),
                              off=mean([s[names[1]]["late"] for s in sums if names[1] in s]),
                              coin=mean([s["coin"]["late"] for s in sums if "coin" in s]),
                              seeds=[s[names[0]]["late"] for s in sums], n=len(sums))
    return out


def bar(mem):
    chance = 1.0 / mem["answers"]
    return chance + (1.0 - chance) / 2


def f2(v):
    return "  -  " if v is None else "%.2f" % v


def print_table(mems, results):
    print("A member is got at a level when late >= the bar: halfway from chance (1/answers)"
          " to 1.")
    print("a: linear reader of the sheet as born (and of the input lines alone).  b: the"
          " readout rule, frozen sheet.")
    print("c: minimal engine and table neurons, default rules.  'off' is each one's learning-"
          "off twin.\n")
    head = ("%-34s %4s %4s | %5s %5s | %5s %5s %5s | %5s %5s | %5s %5s | %s" %
            ("member", "ans", "bar", "sheet", "input", "rule", "off", "coin", "min", "off",
             "table", "off", "got (a b min table)"))
    print(head)
    family = None
    for mem in mems:
        if mem["family"] != family:
            family = mem["family"]
            print("-- " + family)
        r = results[mem["id"]]
        a, b, mi, ta = (r.get(k, {}) for k in ("a", "b", "min", "tab"))
        got = []
        for level, key in ((a, "sheet"), (b, "run"), (mi, "run"), (ta, "run")):
            v = level.get(key)
            got.append("." if v is None else ("Y" if v >= bar(mem) else "-"))
        print("%-34s %4d %4.2f | %5s %5s | %5s %5s %5s | %5s %5s | %5s %5s | %s" % (
            "  " + mem["label"] + (" (held-out x)" if mem["held"] else ""), mem["answers"],
            bar(mem), f2(a.get("sheet")), f2(a.get("inputs")), f2(b.get("run")),
            f2(b.get("off")), f2(b.get("coin")), f2(mi.get("run")), f2(mi.get("off")),
            f2(ta.get("run")), f2(ta.get("off")), " ".join(got)))


def print_ladder(mems, results):
    """per family and level: the largest size still got, and the first size not got"""
    print("\nThe ladder: per family, the largest size each level still gets (first size it"
          " does not, in brackets).")
    print("%-28s %-22s %-22s %-22s %-22s" % ("family", "a sheet", "b rule", "c minimal",
                                            "c table"))
    families = []
    for mem in mems:
        if mem["family"] not in families:
            families.append(mem["family"])
    for fam in families:
        rows = sorted([m for m in mems if m["family"] == fam], key=lambda m: m["size"])
        cells = []
        for level, key in (("a", "sheet"), ("b", "run"), ("min", "run"), ("tab", "run")):
            best, first_fail, any_run = None, None, False
            for mem in rows:
                v = results[mem["id"]].get(level, {}).get(key)
                if v is None:
                    continue
                any_run = True
                if v >= bar(mem):
                    if first_fail is None:
                        best = mem["label"]
                elif first_fail is None:
                    first_fail = mem["label"]
            text = "." if not any_run else ((best or "none") +
                                            (" (%s)" % first_fail if first_fail else ""))
            cells.append(text)
        print("%-28s %-22s %-22s %-22s %-22s" % (fam, *cells))


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 1
    name = args[0]
    opts = {"--levels": "abc", "--members": "", "--seeds": "", "--jobs": "2"}
    flags = {"--quick": False, "--table-only": False, "--list": False}
    extra = []
    i = 1
    while i < len(args):
        if args[i] in opts and i + 1 < len(args):
            opts[args[i]] = args[i + 1]
            i += 2
        elif args[i] in flags:
            flags[args[i]] = True
            i += 1
        else:
            extra.append(args[i])
            i += 1
    life = QUICK if flags["--quick"] else FULL
    seeds = parse_seeds(opts["--seeds"]) if opts["--seeds"] else life["seeds"]
    mems = members()
    if flags["--quick"]:
        mems = [m for m in mems if m["id"] in QUICK_IDS]
    if opts["--members"]:
        mems = [m for m in mems if re.search(opts["--members"], m["id"])]
    if flags["--list"]:
        for m in mems:
            print(m["id"], " ".join(m["args"]))
        return 0
    card = name + ("-quick" if flags["--quick"] else "")
    outdir = os.path.join(ROOT, "out", "functions", card)
    levels = []
    for ch in opts["--levels"]:
        levels += {"a": ["a"], "b": ["b"], "c": ["min", "tab"]}[ch]

    todo = []
    for level in levels:
        for mem in mems:
            for seed in seeds:
                done_file, cmd = jobs_for(mem, level, seed, life, extra, outdir)
                # blobrun opens its -sum.csv at the start: an empty one is a run cut short
                if not os.path.exists(done_file) or os.path.getsize(done_file) == 0:
                    todo.append((done_file, cmd))
    if todo and not flags["--table-only"]:
        jobs = int(opts["--jobs"])
        print("functions %s: %d runs, %d at a time, seeds %s, levels %s" % (
            card, len(todo), jobs, seeds, " ".join(levels)), flush=True)
        started = time.time()
        failed = []
        with ThreadPoolExecutor(max_workers=jobs) as pool:
            for n, (item, code) in enumerate(zip(todo, pool.map(run, todo))):
                if code != 0:
                    failed.append(os.path.basename(item[0]))
                if (n + 1) % 20 == 0 or n + 1 == len(todo):
                    print("  %d of %d done, %.0f min" % (n + 1, len(todo),
                                                        (time.time() - started) / 60),
                          flush=True)
        if failed:
            print("FAILED: " + " ".join(failed))
    results = {m["id"]: collect(m, seeds, outdir) for m in mems}
    print("\nfunctions %s, flags: %s, seeds %s\n" % (card, " ".join(extra) or "none",
                                                     ",".join(str(s) for s in seeds)))
    print_table(mems, results)
    print_ladder(mems, results)
    return 0


if __name__ == "__main__":
    sys.exit(main())
