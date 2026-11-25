#!/usr/bin/env python3
"""The invariance battery (docs/curriculum.md, "Invariance"; idea 101), at three levels.

    scripts/invariance.py NAME [--quick] [--resume] [--levels abc] [--jobs N] [flags ...]

Every row is a task whose held-out versions are never rewarded and are the only ones
scored. For each row and seed (5 to 12; --quick: 5 and 6, short lives):
  a  is the answer readable: `blobprobe invariance` on the sheet as born, a plain linear
     reader fitted on the rewarded versions, scored on the same versions and on the
     held-out ones; for the input lines, the whole sheet and the motors' view
  b  does the working readout get it: blobrun with the frozen sheet and the evidence
     readout (learn_mode 5 ... --motor-spread 128); full engine only
  c  does the whole thing learning get it: learn_mode 6 with the same readout (full
     engine), or the engine's own defaults (--engine minimal, --table)
b and c show the held-out score, the rewarded versions' score, the learning-off twin and
coin flips. The engine follows the flags: --engine minimal or --table pick another. Rows
"+ motion" spend half the life in unrewarded stretches where the shape drifts through its
versions; level a is the same as without (nothing learns there).

Writes out/invariance/NAME/ (runs) and out/scorecards/invariance-NAME.csv, prints a table.
At most --jobs runs at a time (2), each under nice -n 19.
"""

import csv
import os
import statistics
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLOBRUN = os.path.join(ROOT, "build", "blobrun")
BLOBPROBE = os.path.join(ROOT, "build", "blobprobe")

PLUG = ["--ticks", "20", "--motor-side", "3"]
READOUT = ["--g.readout_evidence", "1", "--g.readout_shift", "4", "--g.readout_balance", "1",
           "--motor-spread", "128"]
MOTION = ["--inv-motion", "50", "--inv-motion-steps", "8"]
FULL = {"steps": 40000, "window": 1000, "trials": 4000, "settle": 30000,
        "seeds": list(range(5, 13))}
QUICK = {"steps": 4000, "window": 200, "trials": 1500, "settle": 5000, "seeds": [5, 6]}

# (row id, label, task flags, has level a)
PICTURES = [("position", "position: a shape at 15 places, 4 held"),
            ("scale", "scale: 1x held, taught at 2x and 3x"),
            ("rotation", "rotation: half and 3/4 turns held"),
            ("bright", "bright: 1 line of 4 per pixel held (control)"),
            ("clutter", "clutter: 8 and 16 extra blocks held"),
            ("mixed", "mixed: 1x right and 2x left held"),
            ("view", "view: 3D object, 3 of 8 angles held")]
TIMES = [("shift", "shift: tune starting 4 ticks in held"),
         ("tempo", "tempo: 2-tick notes held, taught 4-8"),
         ("pitch", "pitch: tune at 2 of 6 pitches held")]


def rows():
    out = [("place", "place: the suite's place task", ["--task", "place", "--sensor-side", "3"],
            True)]
    for kind, label in PICTURES:
        out.append((kind, label, ["--task", "inv-" + kind, "--sensor-side", "1"], True))
    for kind, label in TIMES:
        side = "1" if kind == "pitch" else "3"
        out.append((kind, label, ["--task", "inv-" + kind, "--sensor-side", side], True))
    for kind, label in PICTURES:
        out.append((kind + "+motion", "  " + kind + " + motion (half the life drifting)",
                    ["--task", "inv-" + kind, "--sensor-side", "1"] + MOTION, False))
    return out


def engine_of(extra):
    if "--table" in extra:
        return "table"
    if "--engine" in extra:
        return extra[extra.index("--engine") + 1]
    return "blob"


def jobs_for(life, levels, engine):
    """(row, level, seed) -> command"""
    jobs = {}
    full = engine == "blob"
    spread = READOUT[-2:] if full else []
    for row, _, task, has_a in rows():
        for seed in life["seeds"]:
            s = ["--seed", str(seed), "--task-seed", str(seed)]
            if "a" in levels and has_a:
                jobs[(row, "a", seed)] = (
                    [BLOBPROBE, "invariance"] + PLUG + task + spread + s +
                    ["--trials", str(life["trials"]), "--settle", str(life["settle"]),
                     "--label", row])
            lifeflags = PLUG + task + s + ["--steps", str(life["steps"]), "--window",
                                           str(life["window"])]
            if "b" in levels and full:
                jobs[(row, "b", seed)] = [BLOBRUN] + lifeflags + ["--g.learn_mode", "5"] + READOUT
            if "c" in levels:
                own = ["--g.learn_mode", "6"] + READOUT if full else []
                jobs[(row, "c", seed)] = [BLOBRUN] + lifeflags + own
    return jobs


def stem(outdir, key):
    row, level, seed = key
    return os.path.join(outdir, "%s-%s-s%d" % (row.replace("+", "-"), level, seed))


def read_kv(path, driver_col):
    out = {}
    if not os.path.exists(path):
        return out
    with open(path) as f:
        next(f, None)
        for line in f:
            p = line.strip().split(",")
            if driver_col and len(p) == 3:
                out.setdefault(p[0], {})[p[1]] = float(p[2])
            elif not driver_col and len(p) == 2:
                out[p[0]] = float(p[1])
    return out


def mean(v):
    return statistics.mean(v) if v else None


def fmt(v):
    return "  -  " if v is None else "%.2f" % v


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 1
    name = args[0]
    quick = "--quick" in args
    resume = "--resume" in args
    levels, jobs_at_once, extra = "abc", 2, []
    i = 1
    while i < len(args):
        a = args[i]
        if a in ("--quick", "--resume"):
            pass
        elif a == "--levels":
            levels = args[i + 1]
            i += 1
        elif a == "--jobs":
            jobs_at_once = int(args[i + 1])
            i += 1
        else:
            extra.append(a)
        i += 1
    life = QUICK if quick else FULL
    card = name + ("-quick" if quick else "")
    outdir = os.path.join(ROOT, "out", "invariance", card)
    os.makedirs(outdir, exist_ok=True)
    engine = engine_of(extra)
    jobs = jobs_for(life, levels, engine)

    def launch(item):
        key, cmd = item
        st = stem(outdir, key)
        done = st + ("-sum.txt" if key[1] == "a" else "-sum.csv")
        if resume and os.path.exists(done):
            return key, 0
        if key[1] == "a":
            cmd = cmd + ["--sum", done] + extra
        else:
            cmd = cmd + ["--out", st + ".csv"] + extra
        with open(st + ".txt", "w") as f:
            f.write(" ".join(cmd) + "\n")
            f.flush()
            code = subprocess.run(["nice", "-n", "19"] + cmd, stdout=f,
                                  stderr=subprocess.STDOUT).returncode
        return key, code

    order = sorted(jobs.items(), key=lambda kv: (kv[0][1] == "a", kv[0][0], kv[0][2]))
    print("invariance %s: engine %s, levels %s, %d runs, %d at a time, seeds %s" % (
        card, engine, levels, len(order), jobs_at_once, life["seeds"]), flush=True)
    started = time.time()
    failed, done = [], 0
    with ThreadPoolExecutor(max_workers=jobs_at_once) as pool:
        for key, code in pool.map(launch, order):
            done += 1
            if code:
                failed.append("%s-%s-s%d" % key)
            if done % 10 == 0 or done == len(order):
                print("  %d of %d runs, %.0f min" % (done, len(order),
                                                     (time.time() - started) / 60), flush=True)
    minutes = (time.time() - started) / 60
    table = collect(outdir, life, levels, engine)
    print_table(table, levels)
    write_card(os.path.join(ROOT, "out", "scorecards", "invariance-" + card + ".csv"), table,
               [("name", card), ("engine", engine), ("flags", " ".join(extra)),
                ("seeds", " ".join(map(str, life["seeds"]))),
                ("life", "%d steps, window %d" % (life["steps"], life["window"])),
                ("date", time.strftime("%Y-%m-%d")), ("wall", "%.0f min" % minutes)])
    print("scorecard: out/scorecards/invariance-%s.csv (%.0f min)" % (card, minutes))
    if failed:
        print("FAILED (see the .txt in %s): %s" % (outdir, " ".join(failed)))
        return 1
    return 0


A_KEYS = [("input", "input"), ("whole", "sheet"), ("view", "motors' view")]
LIFE_KEYS = [("late", "blob", "held"), ("taught_late", "blob", "taught"),
             ("late", "learn_off", "twin held"), ("late", "coin", "coin held")]


def collect(outdir, life, levels, engine):
    table = []
    on, off = ("table", "table_off") if engine == "table" else ("blob", "learn_off")
    for row, label, _, has_a in rows():
        r = {"row": row, "label": label}
        for seed in life["seeds"]:
            if has_a:
                kv = read_kv(stem(outdir, (row, "a", seed)) + "-sum.txt", False)
                for key, _ in A_KEYS:
                    for part in ("same", "held"):
                        v = kv.get(key + "_" + part)
                        if v is not None:
                            r.setdefault("a_%s_%s" % (key, part), []).append(v)
            for level in "bc":
                kv = read_kv(stem(outdir, (row, level, seed)) + "-sum.csv", True)
                for key, driver, col in LIFE_KEYS:
                    d = {"blob": on, "learn_off": off}.get(driver, driver)
                    v = kv.get(d, {}).get(key)
                    if v is not None:
                        r.setdefault("%s_%s" % (level, col), []).append(v)
        for k in list(r):
            if isinstance(r[k], list):
                vals = r[k]
                r[k + "_sd"] = statistics.stdev(vals) if len(vals) > 1 else None
                r[k + "_n"] = len(vals)
                r[k] = mean(vals)
        table.append(r)
    return table


def print_table(table, levels):
    head = "%-46s" % "row (held-out score; chance 0.50)"
    if "a" in levels:
        head += " | a: same/held  input   sheet   view"
    for level in "bc":
        if level in levels:
            head += " | %s: held  taught  twin  coin" % level
    print(head)
    for r in table:
        line = "%-46s" % r["label"][:46]
        if "a" in levels:
            line += " |            "
            for key, _ in A_KEYS:
                line += " %s/%s" % (fmt(r.get("a_%s_same" % key)), fmt(r.get("a_%s_held" % key)))
        for level in "bc":
            if level in levels:
                line += " |    " + "  ".join(fmt(r.get("%s_%s" % (level, c)))
                                            for c in ("held", "taught", "twin held", "coin held"))
        print(line)


def write_card(path, table, about):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    cols = ["a_input_same", "a_input_held", "a_whole_same", "a_whole_held", "a_view_same",
            "a_view_held", "b_held", "b_taught", "b_twin held", "b_coin held", "c_held",
            "c_taught", "c_twin held", "c_coin held"]
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["row", "label"] + [c.replace(" ", "_") for c in cols] + ["b_held_sd",
                                                                            "c_held_sd", "seeds"])
        for k, v in about:
            w.writerow(["about", k] + [""] * len(cols) + ["", "", v])
        for r in table:
            cells = [r["row"], r["label"].strip()]
            for c in cols + ["b_held_sd", "c_held_sd"]:
                v = r.get(c)
                cells.append("" if v is None else "%.4f" % v)
            n = max([r.get(c + "_n", 0) for c in cols] + [0])
            cells.append(str(n))
            w.writerow(cells)


if __name__ == "__main__":
    sys.exit(main())
