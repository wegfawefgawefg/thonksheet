"""The map, the inside-learning test and the speed check of scripts/capdefault.py (idea 180).
Quick-map lives go through scripts/oneset.py (its caches in out/oneset/); the inside test is
`blobmnist ceiling` with the commands of the jobs whose numbers it repeats (compete2, hop,
hop2), so their finished runs in the main checkout are copied in place of running again."""

import contextlib
import io
import json
import os
import re
import shutil
import statistics
import subprocess
import sys

import inside as I
import oneset as O
from sweep import twin_args

ROOT = O.ROOT
MAIN = "/media/vega/mdot2b/Coding2/AI/thonksheet/out/from-agents"
CAP = "--g.threshold_cap 1000"
COLS = ["base", "a2rucomp", "best"]
# what the map is run with besides cap 64 (nothing) and cap 1000: name -> genome numbers.
# set after the rate part was read (docs/results-2026-11-26-cap-default.md, part 2)
FIXES = {
    # the cap lifted and an honest target of 64 (the sector's target and the walk's own)
    "t64": CAP + " --g.sector_target_per_1024 64 --g.threshold_down_shift 8",
    # a cap between the two
    "c128": "--g.threshold_cap 128",
    # the cap lifted for every cell but the motor cells (through the probe: no genome number)
    "m64": CAP + " --cap-motor 64",
}
# the ones the map runs when --fixes does not say
MAP_FIXES = ["c128"]
MUST = ["xor", "echo16", "order", "mnist"]


def mean(v):
    got = [x for x in v if x is not None]
    return statistics.mean(got) if got else None


def fmt(v, d=4):
    return "-" if v is None else ("%." + str(d) + "f") % v


# ---- the quick map ----

def variants(opt):
    out = [("64", ""), ("1000", CAP)]
    for name in MAP_FIXES if opt.get("fixes") is None else opt["fixes"]:
        out.append((name, FIXES[name]))
    return out


PROBE_FLAGS = ("--cap-motor", "--cap-hidden", "--cap-memory", "--hold-motor", "--hold-hidden")
PROBE_CACHE = os.path.join(O.OUT, "cache-capsplit.jsonl")


def probe_life(args):
    """one quick-map life through `blobprobe capsplit` (for caps by kind of cell, which
    blobrun has no number for): a cache row as scripts/oneset.py keeps them"""
    own, probe, life = [], [], []
    i = 0
    while i < len(args) - 1:
        width = O.TWO_VALUES.get(args[i], 1)
        part = own if args[i].startswith("--g.") else probe if args[i] in PROBE_FLAGS else life
        part.extend(args[i:i + 1 + width])
        i += 1 + width
    cmd = ["nice", "-n", "19", os.path.join(ROOT, "build", "blobprobe"), "capsplit", "--seed",
           life[life.index("--seed") + 1], "--life", " ".join(life), "--look-steps", "200"]
    done = subprocess.run(cmd + probe + own, capture_output=True, text=True, cwd=ROOT)
    if done.returncode != 0:
        raise RuntimeError(" ".join(args) + "\n" + done.stderr)
    windows = [float(x) for x in re.findall(r"^window \d+ score (\S+)", done.stdout, re.M)]
    return {"key": " ".join(args), "windows": windows, "probe": 1}


def run_probe_column(flags, seeds, rows, opt):
    """a column with probe flags: lives and twins that are not in the cache yet"""
    from concurrent.futures import ThreadPoolExecutor
    cache = O.load_cache()
    todo = []
    for rname, rargs in rows:
        for s in seeds:
            for f in (flags, twin_args(flags)):
                args = O.life_cmd(rargs, f, s)
                if " ".join(args) not in cache and args not in todo:
                    todo.append(args)
    todo.sort(key=lambda a: ("mnist" not in a, "50" not in a))
    if todo:
        print("capdefault: %d probe lives, %d at a time" % (len(todo), opt["jobs"]),
              file=sys.stderr, flush=True)
    with ThreadPoolExecutor(opt["jobs"]) as ex:
        for row in ex.map(probe_life, todo):
            with open(PROBE_CACHE, "a") as f:
                f.write(json.dumps(row) + "\n")


def probe_check(opt):
    """the probe's life against blobrun's: every row, the base rule at cap 64, seed 5"""
    from concurrent.futures import ThreadPoolExecutor
    rows, cols = O.read_map(O.MORE)
    rows = O.pick_rows(rows, "echo2,chain" if opt["quick"] else ",".join(opt.get("rows") or []))
    cache = O.load_cache()
    log = os.path.join(opt["out"], "probe-check.txt")
    if not opt["table_only"]:
        jobs = [O.life_cmd(rargs, cols["base"], 5) for _, rargs in rows]
        with ThreadPoolExecutor(opt["jobs"]) as ex, open(log, "w") as f:
            for (rname, _), row in zip(rows, ex.map(probe_life, jobs)):
                old = cache.get(row["key"])
                diff = max(abs(a - b) for a, b in zip(row["windows"], old["windows"])) if old \
                    and len(old["windows"]) == len(row["windows"]) else None
                f.write("%s: %d windows, largest difference to blobrun's cached life %s\n" % (
                    rname, len(row["windows"]), "-" if diff is None else "%.6f" % diff))
    return open(log).read().rstrip() if os.path.exists(log) else "not run"


def run_column(name, flags, seeds, rows, opt):
    if any(f in PROBE_FLAGS for f in flags):
        all_rows = O.read_map(O.MORE)[0]
        run_probe_column(flags, seeds, O.pick_rows(all_rows, ",".join(rows) if rows else ""),
                         opt)
        return
    cmd = ["python3", os.path.join(ROOT, "scripts", "oneset.py"), "run", name, "--seeds",
           ",".join(map(str, seeds)), "--jobs", str(opt["jobs"])]
    cmd += (["--rows", ",".join(rows)] if rows else []) + ["--"] + flags
    subprocess.run(cmd, cwd=ROOT, stdout=subprocess.DEVNULL)


def passed(cache, rows, flags, seeds):
    """{row: (late per seed, passes)}; None where lives are missing"""
    out = {}
    for rname, rargs in rows:
        late = O.scores(cache, rargs, flags, seeds, False)
        twin = O.scores(cache, rargs, flags, seeds, True)
        if None in late or None in twin:
            out[rname] = None
            continue
        chance, perfect = O.calib_of(cache, rargs)
        out[rname] = (late, O.judge(late, twin, chance, perfect)[1])
    return out


def health(cache, rargs, flags, seeds, key):
    got = []
    for s in seeds:
        row = cache.get(" ".join(O.life_cmd(rargs, flags, s)))
        got.append(row.get(key) if row else None)
    return mean(got)


def map_block(cache, rows, cols, seeds, var, lines, names=COLS):
    """one table per column (each variant beside cap 64), then what changed"""
    for col in names:
        named = {("%s" % col if tag == "64" else "%s@%s" % (col, tag)): cols[col] + f.split()
                 for tag, f in var}
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            O.print_many(list(named), rows, named, cache, seeds, True)
        lines += [buf.getvalue().rstrip(), ""]
        base = passed(cache, rows, cols[col], seeds)
        for tag, f in var[1:]:
            got = passed(cache, rows, cols[col] + f.split(), seeds)
            if any(v is None for v in got.values()) or any(v is None for v in base.values()):
                continue
            won = [r for r in got if got[r][1] and not base[r][1]]
            lost = [r for r in got if base[r][1] and not got[r][1]]
            must = ", ".join("%s %+.3f" % (r, statistics.mean(got[r][0]) - statistics.mean(
                base[r][0])) for r in MUST if r in got)
            lines.append("- %s at %s against cap 64: rows passed %d (%d); gained %s; lost %s; %s"
                         % (col, tag, sum(v[1] for v in got.values()),
                            sum(v[1] for v in base.values()), ", ".join(won) or "none",
                            ", ".join(lost) or "none", must))
        lines.append("")


def quick_map(opt):
    rows, cols = O.read_map(O.MORE)
    var = variants(opt)
    seeds = [5] if opt["quick"] else (opt["seeds"] or list(range(5, 13)))
    only = ["echo2", "chain"] if opt["quick"] else opt.get("rows")
    # the tuning seeds: the chain and the other rows only a critic passed or lifted
    early, early_rows = [1, 2, 3, 4], ["chain", "cartpole", "noisy"]
    names = opt.get("cols") or COLS
    if not opt["table_only"]:
        for col in names:
            # --skip-caps: only the changes (a machine without the cap-64 and cap-1000 caches)
            for tag, f in var[2:] if opt.get("skip_caps") else var:
                run_column("cap-%s-%s" % (col, tag), cols[col] + f.split(), seeds, only, opt)
                if not opt["quick"] and not opt["seeds"]:
                    run_column("cap-%s-%s" % (col, tag), cols[col] + f.split(), early,
                               early_rows, opt)
    cache = O.load_cache()
    lines = ["The quick map, seeds %s, each column at cap 64 and beside it with the cap at 1000"
             " (`@1000`)%s." % (",".join(map(str, seeds)), "".join(
                 " and with `%s` (`@%s`)" % (FIXES[n], n) for n, _ in var[2:])), ""]
    map_block(cache, O.pick_rows(rows, ",".join(only) if only else ""), cols, seeds, var, lines,
              names)
    if not opt["quick"] and not opt["seeds"]:
        lines += ["Seeds 1-4, the rows only a critic passed or lifted:", ""]
        map_block(cache, O.pick_rows(rows, ",".join(early_rows)), cols, early, var, lines,
                  names)
    # the sheet's rate at the end of each life, as blobrun counts it (sensor cells included)
    lines += ["Rate over target at the end of the life (blobrun's count, sensor cells "
              "included; MNIST's is high for that reason) and share of thresholds at the cap, "
              "mean of the seeds, cap 64 / cap 1000:", ""]
    for col in names:
        cells = []
        for rname, rargs in O.pick_rows(rows, ",".join(only) if only else ""):
            r = [health(cache, rargs, cols[col] + f.split(), seeds, "rate_over_target")
                 for _, f in var[:2]]
            c = health(cache, rargs, cols[col], seeds, "share_threshold_cap")
            cells.append("%s %s / %s (%s)" % (rname, fmt(r[0], 2), fmt(r[1], 2), fmt(c, 3)))
        lines.append("- %s: %s" % (col, "; ".join(cells)))
    return "\n".join(lines)


# ---- the inside-learning test ----

PAIR = "--pair 2 --pair-mix 1"
B5 = "--g.learn_mode 5 --g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5"
COMPETE = "--g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5"
# (test, arm): what it is, flags after --seed. MNIST as the compete2 job (a life with no
# reward; born with the rule's flags) and the hop job (a taught life, told the answer); the
# pair test as the hop2 job's "same" recipe (taught lives for both rules)
TAUGHT = "--life %d " + " ".join(I.RULE) + " --life-teach 2"
INSIDE = {
    ("mnist", "born"): ("the born sheet (the hop job's: the rule's numbers set, no life)",
                        "--g.hop_on 1 --g.hop_shift 7"),
    ("mnist", "born-tw"): ("born, the tile's winners at 4 in the read", B5),
    ("mnist", "comp"): ("competitive learning, a life with no reward", "--life %d " + B5),
    ("mnist", "hop"): ("the one-hop rule, told the answer", TAUGHT + " --g.hop_on 1 "
                       "--g.hop_shift 7"),
    ("pair", "born"): ("the born sheet", PAIR),
    ("pair", "comp"): ("competitive learning in a taught life", TAUGHT + " " + PAIR + " " +
                       COMPETE),
    ("pair", "hop"): ("the one-hop rule, told the answer", TAUGHT + " " + PAIR +
                      " --g.hop_on 1 --g.hop_shift 7 --g.hop_fired 0"),
}
INSIDE_CAPS = (64, 128, 1000)
OLD = ["compete2/mnist/b5", "compete2/mnist/b5cap", "hop/inside/hop2s7-report",
       "hop/inside/hop2s7cap-report", "hop2/same"]


def inside_cmd(test, arm, cap, seed, opt):
    life, fit = (600, 2000) if opt["quick"] else (60000, 60000)
    flags = INSIDE[test, arm][1]
    flags = flags % life if "%d" in flags else flags
    cmd = ["ceiling"] + I.SHEET + I.READER + ["--fit", str(fit), "--seed", str(seed)]
    cmd += flags.split() + ([] if cap == 64 else ["--g.threshold_cap", str(cap)])
    return cmd + (["--test-count", "1000"] if opt["quick"] else [])


def inside_file(opt, test, arm, cap, seed, check=False):
    return os.path.join(opt["out"], "inside", "%s-%s-c%d-%d%s.txt" % (
        test, arm, cap, seed, "-check" if check else ""))


def old_runs():
    """finished runs of the earlier jobs, by their flags"""
    found = {}
    for sub in OLD:
        folder = os.path.join(MAIN, sub)
        for f in sorted(os.listdir(folder)) if os.path.isdir(folder) else []:
            path = os.path.join(folder, f)
            if not f.endswith(".txt") or not os.path.isfile(path):
                continue
            with open(path) as h:
                first = h.readline().split()
                rest = h.read()
            if "total wall" in rest and "ceiling" in first:
                found[" ".join(first[first.index("ceiling"):])] = path
    return found


def inside_read(path):
    if not os.path.exists(path):
        return None
    text = open(path).read()
    m = re.search(r"sensor cells left out\s+\d+\s+([0-9.]+)", text)
    r = re.search(r"frozen, fit digits.*?: ([0-9.]+) spikes", text)
    return {"score": float(m.group(1)), "rate": float(r.group(1)) if r else None} if m else None


def inside_test(opt):
    seeds = [5] if opt["quick"] else (opt["seeds"] or list(range(5, 13)))
    binary = os.path.join(ROOT, "build", "blobmnist")
    old = {} if opt["quick"] or opt["table_only"] else old_runs()
    jobs, reused = [], {}
    from capdefault import pool
    for (test, arm) in INSIDE:
        for cap in INSIDE_CAPS:
            for s in seeds:
                cmd = inside_cmd(test, arm, cap, s, opt)
                out = inside_file(opt, test, arm, cap, s)
                src = old.get(" ".join(cmd))
                if src and not os.path.exists(out):
                    os.makedirs(os.path.dirname(out), exist_ok=True)
                    shutil.copyfile(src, out)
                if src:
                    reused[test, arm, cap] = reused.get((test, arm, cap), 0) + 1
                    # one seed of every copied group is run again with this build
                    if s == seeds[0]:
                        jobs.append((["nice", "-n", "19", binary] + cmd,
                                     inside_file(opt, test, arm, cap, s, True)))
                else:
                    jobs.append((["nice", "-n", "19", binary] + cmd, out))
    if not opt["table_only"]:
        pool(jobs, opt["jobs"])
    lines = ["The inside-learning test: a fresh linear reader of the non-sensor cells, fit on "
             "60,000 training digits, scored on test digits; seeds %s. rate: the hidden cells' "
             "spikes per 1,000 cell-ticks while the reader's digits are shown. lived - born: "
             "paired by seed, with its standard error (competitive learning on MNIST against "
             "born-tw)." % ",".join(map(str, seeds)), "",
             "| test | sheet | cap | score | rate | lived - born | per seed |",
             "|---|---|---|---|---|---|---|"]
    for test in ("mnist", "pair"):
        for cap in INSIDE_CAPS:
            born = [inside_read(inside_file(opt, test, "born", cap, s)) for s in seeds]
            for arm in [a for t, a in INSIDE if t == test]:
                got = [inside_read(inside_file(opt, test, arm, cap, s)) for s in seeds]
                if not any(got):
                    continue
                # competitive learning on MNIST is read with the tile's winners: its own born
                base = born if (test, arm) != ("mnist", "comp") else [inside_read(
                    inside_file(opt, test, "born-tw", cap, s)) for s in seeds]
                d = [g["score"] - b["score"] for g, b in zip(got, base) if g and b]
                diff = "" if arm.startswith("born") or len(d) < 2 else "%+.4f ±%.4f" % (
                    statistics.mean(d), statistics.stdev(d) / len(d) ** 0.5)
                lines.append("| %s | %s | %d | %s | %s | %s | %s |" % (
                    test, arm, cap, fmt(mean([g["score"] for g in got if g])),
                    fmt(mean([g["rate"] for g in got if g]), 1), diff,
                    " ".join(fmt(g["score"]) if g else "-" for g in got)))
    lines += ["", "Sheets: " + "; ".join("%s %s: %s" % (t, a, INSIDE[t, a][0]) for t, a in INSIDE)]
    checks = []
    for (test, arm) in INSIDE:
        for cap in INSIDE_CAPS:
            a = inside_read(inside_file(opt, test, arm, cap, seeds[0]))
            b = inside_read(inside_file(opt, test, arm, cap, seeds[0], True))
            if b:
                checks.append("%s %s cap %d: copied %s, run again %s" % (
                    test, arm, cap, fmt(a["score"]) if a else "-", fmt(b["score"])))
    if checks:
        lines += ["", "Copied from the earlier jobs' outputs (same flags), seed %d run again "
                  "with this build: " % seeds[0] + "; ".join(checks)]
    return "\n".join(lines)


# ---- the first press, step by step ----

def first_press(opt):
    """the chain with three buttons at cap 1000, the last 2,000 steps one by one: what is
    pressed on a stage-0 step that follows a stage-1 step (fresh), and on one that follows a
    stage-0 step where the third button or B was pressed"""
    import capdefault as C
    rows, cols = O.read_map(O.MORE)
    rows = dict(rows)
    seeds = [5] if opt["quick"] else (opt["seeds"] or list(range(5, 13)))
    names = ["base"] if opt["quick"] else COLS
    jobs = []
    for col in names:
        for s in seeds:
            cmd, _ = C.look_job(opt, rows, cols, "chain", col, "k3-1000", s)
            out = os.path.join(opt["out"], "trace", "chain-%s-k3-1000-s%d.txt" % (col, s))
            jobs.append((cmd + ["--trace", "400" if opt["quick"] else "2000"], out))
    if not opt["table_only"]:
        C.pool(jobs, opt["jobs"])
    lines = ["The chain with three buttons at cap 1000, the last 2,000 steps one by one, seeds "
             "%s. Each cell: share of those stage-0 steps on which A / B / the third button was "
             "pressed (steps counted)." % ",".join(map(str, seeds)), "",
             "| column | after a stage-1 step | after the third button on stage 0 | after B on "
             "stage 0 |", "|---|---|---|---|"]
    for col in names:
        count = {k: [0, 0, 0, 0] for k in ("fresh", "third", "b")}
        for s in seeds:
            path = os.path.join(opt["out"], "trace", "chain-%s-k3-1000-s%d.txt" % (col, s))
            text = open(path).read() if os.path.exists(path) else ""
            m = re.search(r"^chain: A (\d+) B (\d+)", text, re.M)
            if not m:
                continue
            a, b = int(m.group(1)), int(m.group(2))
            # the stage is the right answer: A on stage 0, B on stage 1
            steps = [(int(r), int(p)) for r, p in re.findall(
                r"^step \d+ right (-?\d+) pressed (-?\d+)", text, re.M)]
            for (r0, p0), (r1, p1) in zip(steps, steps[1:]):
                if r1 != a:
                    continue
                key = "fresh" if r0 == b else "b" if p0 == b else "third" if p0 not in (a, -1) \
                    else None
                if key:
                    count[key][0 if p1 == a else 1 if p1 == b else 2 if p1 >= 0 else 3] += 1
        cells = []
        for k in ("fresh", "third", "b"):
            n = sum(count[k])
            cells.append("- " if not n else "%.2f / %.2f / %.2f (%d)" % (
                count[k][0] / n, count[k][1] / n, count[k][2] / n, n))
        lines.append("| %s | %s |" % (col, " | ".join(cells)))
    return "\n".join(lines)


# ---- alive: long lives under a cap (the rate-control job's first test) ----

ALIVE = {"std": "", "c128": "--g.threshold_cap 128", "c256": "--g.threshold_cap 256", "e": CAP}


def alive(opt):
    """160,000-step lives (echo at 3 tiles, delayed echo of 2) and the 20,000-digit MNIST
    world under base, base with competitive learning and the one setting, as
    scripts/ratecontrol.sh's alive test (blobprobe ratehealth). std (cap 64) and e (cap 1000)
    are that job's runs, copied"""
    import ratecontrol as RC
    import ratecontrol_tests as T
    seeds = [5] if opt["quick"] else (opt["seeds"] or [5, 6])
    names = ["c256"] if opt["quick"] else (opt.get("arms") or list(ALIVE))
    lines = ["Long lives, seeds %s (cap 64 and cap 1000: the rate-control job's runs). Mean of "
             "the seeds at the end of the life: rate over target; silent and stuck shares of "
             "non-sensor cells; hidden thresholds at the cap; silent share of plain hidden "
             "cells; the first 100 ticks back after a 2,000-tick gap over the rate before; late "
             "score." % ",".join(map(str, seeds)), "",
             "| cap | setting | world | rate | silent | stuck | at cap | hidden silent | gap "
             "jump | late |", "|---|---|---|---|---|---|---|---|---|---|"]
    for name in names:
        old = os.path.join(MAIN, "ratecontrol", "alive", name)
        copied = name in ("std", "e") and not opt["quick"]
        if copied and os.path.isdir(old):
            folder = os.path.join(opt["out"], "alive", name)
            os.makedirs(folder, exist_ok=True)
            for f in os.listdir(old):
                if f.endswith(".txt") and not os.path.exists(os.path.join(folder, f)):
                    shutil.copyfile(os.path.join(old, f), os.path.join(folder, f))
        o = {"out": opt["out"], "seeds": seeds, "quick": opt["quick"], "jobs": opt["jobs"],
             "table_only": opt["table_only"] or copied}
        _, detail, _ = RC.alive_row(T.alive(name, ALIVE[name].split(), o))
        cap = {"std": "64", "e": "1000"}.get(name, name[1:])
        for x in detail:
            lines.append("| %s | %s | %s | %s |" % (cap, x[0], x[1], " | ".join(
                fmt(v, 3) for v in (x[2], x[3], x[4], x[5], x[6], x[8], x[9]))))
    return "\n".join(lines)


# ---- speed ----

def speed(opt):
    bench = os.path.join(ROOT, "build", "blobbench")
    rounds = 1 if opt["quick"] else 5
    confs = [("64", []), ("1000", CAP.split()), ("1000, target 64", FIXES["t64"].split())]
    got = {tag: {"off": [], "on": []} for tag, _ in confs}
    log = os.path.join(opt["out"], "speed.txt")
    if not opt["table_only"]:
        with open(log, "w") as f:
            for r in range(rounds):
                for tag, flags in confs:
                    done = subprocess.run(["nice", "-n", "19", bench, "speed", "--tiles", "8",
                                           "--seconds-ms", "200" if opt["quick"] else "1500"]
                                          + flags, capture_output=True, text=True, cwd=ROOT)
                    f.write("round %d cap %s\n%s" % (r, tag.replace(" ", "_"), done.stdout))
    tag = None
    for line in open(log) if os.path.exists(log) else []:
        p = line.split()
        if p and p[0] == "round":
            tag = p[3].replace("_", " ")
        elif len(p) == 4 and p[1] in ("off", "on") and tag:
            got[tag][p[1]].append(float(p[2]))
    lines = ["`blobbench speed --tiles 8` (4,096 cells, one core, this PC under the temperature "
             "governor: rough), %d rounds taking turns, ticks a second, lowest to highest "
             "(median):" % rounds, "", "| cap | learning off | learning on |", "|---|---|---|"]
    for tag, _ in confs:
        cells = []
        for k in ("off", "on"):
            v = got[tag][k]
            cells.append("%.0f to %.0f (%.0f)" % (min(v), max(v), statistics.median(v)) if v
                         else "-")
        lines.append("| %s | %s |" % (tag, " | ".join(cells)))
    return "\n".join(lines)
