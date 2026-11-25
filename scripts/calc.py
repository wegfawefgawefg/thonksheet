#!/usr/bin/env python3
"""The calculation battery (ideas 133, 134; docs/curriculum.md "Calculation").

    scripts/calc.py NAME [--quick] [--jobs N] [--seeds 5-12] [--only fam,fam] [--configs a,b]
                    [--all-sizes] [--table-only] [blobrun flags and --g.name value ...]

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

`probe` and `bit` run on every member; the rest on the first size of each op (marked * in
the table) unless --all-sizes. Runs go to out/calc/NAME/, at most --jobs
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
BUILD = os.path.join(ROOT, os.environ.get("CALC_BUILD", "build"))
BLOBRUN = os.path.join(BUILD, "blobrun")
BLOBPROBE = os.path.join(BUILD, "blobprobe")

BASE = ["--ticks", "20", "--sensor-side", "3", "--motor-side", "3", "--motor-spread", "128"]
WORKING = ["--g.learn_mode", "5", "--g.readout_evidence", "1", "--g.readout_shift", "4",
           "--g.readout_balance", "1"]
FULL = {"steps": 40000, "window": 1000, "trials": 3000, "seeds": list(range(5, 13))}
QUICK = {"steps": 4000, "window": 200, "trials": 1000, "seeds": [5, 6]}


def member(fam, name, args, first=False):
    return dict(fam=fam, name=name, args=["--task", "calc"] + args, first=first,
                op=args[1])


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
    for i, n in enumerate([1, 2, 3, 4, 5, 6, 7]):
        m.append(member("program", "program3x%d" % n, ["--calc", "program", "--calc-bits", "3",
                                                         "--calc-len", str(n)], i == 0))
    ladder("lookup", "max", [2, 4])
    ladder("lookup", "sort3", [2])
    m.append(member("lookup", "kth2x4", ["--calc", "kth", "--calc-bits", "2",
                                          "--calc-list", "4"], True))
    for op, size in (("turn", 4), ("mirror", 4), ("move", 4), ("scale", 4), ("turn3d", 3),
                     ("move3d", 3), ("angle", 2), ("turn3", 2)):
        label = "turn3pts%d" % size if op == "turn3" else "%s%d" % (op, size)
        m.append(member("geometry", label, ["--calc", op, "--calc-size", str(size)],
                        op == "turn"))
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
    # one button per possible answer (members with at most 16 answers), whole answer graded:
    # the engines that press one button, and the working rule in the same form
    "btn": ("run", WORKING + ["--calc-form", "buttons"], True),
    "memory": ("run", ["--engine", "memory", "--calc-form", "buttons"], True),
    "gru": ("run", ["--engine", "gru", "--calc-form", "buttons"], True),
    "mlp": ("run", ["--engine", "mlp", "--calc-form", "buttons"], True),
}
FIRST_ONLY = {"whole", "good", "m6", "minimal", "table"}
BUTTON_ONLY = {"btn", "memory", "gru", "mlp"}


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


def finished(config, stem):
    """a run that was cut off leaves a summary without its scores"""
    path = done_path(config, stem)
    if not os.path.exists(path):
        return False
    with open(path) as f:
        text = f.read()
    return ("bits_seen" in text) if config == "probe" else (",late," in text)


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
                    rows = {}
                    for reader, keys in d.items():
                        rows.update({reader + "." + k: v for k, v in keys.items()})
                else:
                    d = read_sum(done_path(config, stem))
                    rows = {}
                    for driver, keys in d.items():
                        for k, v in keys.items():
                            rows[driver + "." + k] = v
                for k, v in rows.items():
                    if v == v:  # a held-out score with nothing held out is nan
                        per.setdefault(k, []).append(v)
            res.setdefault(mem["name"], {})[config] = {k: mean(v) for k, v in per.items()}
            res[mem["name"]][config]["n"] = max((len(v) for v in per.values()), default=0)
    return res


def cell(d, key, width=5):
    v = d.get(key) if d else None
    return ("%.2f" % v).rjust(width) if v is not None else "-".rjust(width)


def print_table(mems, res, configs, out):
    """A: every member, bits right, seen | held side by side. B: the one-button engines,
    whole answers right, seen | held. C: each output bit on seen cases."""
    head = ("member      in/out | nearest | inputs    | sheet     | rule bit  whole twin coin"
            " | whole good | m6        | minimal twin | table twin")
    lines = ["A. share of output bits right, seen | held-out cases (late; probe columns: the "
             "offline readers on the frozen sheet as born)", head, "-" * len(head)]
    fam = None
    for mem in mems:
        r = res[mem["name"]]
        if mem["fam"] != fam:
            fam = mem["fam"]
            lines.append("-- " + fam)
        pr, b = r.get("probe", {}), r.get("bit", {})
        two = lambda d, k1, k2: cell(d, k1, 4) + cell(d, k2, 5)
        lines.append(
            "%-11s %6s | %s | %s | %s | %s %s %s %s | %s %s | %s | %s %s | %s %s   n=%d" % (
                mem["name"] + ("*" if mem["first"] else ""), layout_of(mem),
                cell(pr, "nearest.bits_held", 7),
                two(pr, "inputs.bits_seen", "inputs.bits_held"),
                two(pr, "sheet.bits_seen", "sheet.bits_held"),
                two(b, "blob.late", "blob.bits_held_late"), cell(b, "blob.whole_seen_late"),
                cell(b, "learn_off.late", 4), cell(b, "coin.late", 4),
                cell(r.get("whole", {}), "blob.late"), cell(r.get("good", {}), "blob.late", 4),
                two(r.get("m6", {}), "blob.late", "blob.bits_held_late"),
                cell(r.get("minimal", {}), "blob.late", 7),
                cell(r.get("minimal", {}), "learn_off.late"),
                cell(r.get("table", {}), "table.late"), cell(r.get("table", {}), "table_off.late"),
                b.get("n", 0)))
    lines += ["", "B. one button per answer: whole answers right, seen | held-out (nearest: the "
              "stored nearest seen trial)",
              "member      | nearest    | rule btn   | memory     | gru        | mlp        | "
              "twins: rule memory gru mlp | coin"]
    for mem in mems:
        r = res[mem["name"]]
        if not any(r.get(c, {}).get("n") for c in BUTTON_ONLY):
            continue
        pr = r.get("probe", {})
        cols = [cell(pr, "nearest.whole_seen", 4) + cell(pr, "nearest.whole_held")]
        for c in ("btn", "memory", "gru", "mlp"):
            d = r.get(c, {})
            cols.append(cell(d, "blob.whole_seen_late", 4) + cell(d, "blob.whole_held_late"))
        twins = " ".join(cell(r.get(c, {}), "learn_off.whole_seen_late", 5)
                         for c in ("btn", "memory", "gru", "mlp"))
        coin = cell(r.get("memory", r.get("btn", {})), "coin.whole_seen_late")
        lines.append("%-11s | %s |  %s | %s" % (mem["name"], " | ".join(cols), twins, coin))
    lines += ["", "B2. the same, share of bits right, seen | held-out",
              "member      | nearest    | rule btn   | memory     | gru        | mlp"]
    for mem in mems:
        r = res[mem["name"]]
        if not any(r.get(c, {}).get("n") for c in BUTTON_ONLY):
            continue
        pr = r.get("probe", {})
        cols = [cell(pr, "nearest.bits_seen", 4) + cell(pr, "nearest.bits_held")]
        for c in ("btn", "memory", "gru", "mlp"):
            d = r.get(c, {})
            cols.append(cell(d, "blob.late", 4) + cell(d, "blob.bits_held_late"))
        lines.append("%-11s | %s" % (mem["name"], " | ".join(cols)))
    lines += ["", "C. each output bit, seen cases: probe sheet / working rule (bit grading) / twin"]
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


BUTTONS = {}


def has_buttons(mem):
    """whether the member has a one-button-per-answer form (at most 16 answers)"""
    if mem["name"] in BUTTONS:
        return BUTTONS[mem["name"]]
    r = subprocess.run([BLOBRUN] + BASE + mem["args"] + ["--calc-form", "buttons", "--steps",
                       "1", "--window", "1", "--no-baselines", "--out",
                       os.path.join(ROOT, "out", "calc", "form-check.csv")],
                       capture_output=True, text=True)
    BUTTONS[mem["name"]] = r.returncode == 0
    return BUTTONS[mem["name"]]


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
        if a in ("--quick", "--table-only", "--all-sizes"):
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
    explicit = "--all-sizes" in rest
    configs = configs or ["probe", "bit", "whole", "good", "m6", "minimal", "table", "btn",
                          "memory", "gru", "mlp"]
    outdir = os.path.join(ROOT, "out", "calc", name + ("-quick" if quick else ""))
    os.makedirs(outdir, exist_ok=True)

    todo = []
    for mem in mems:
        for config in configs:
            if config in FIRST_ONLY and not mem["first"] and not explicit:
                continue
            if config in BUTTON_ONLY and not has_buttons(mem):
                continue
            for seed in seeds:
                stem = os.path.join(outdir, "%s-%s-s%d" % (mem["name"], config, seed))
                if not finished(config, stem):
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
