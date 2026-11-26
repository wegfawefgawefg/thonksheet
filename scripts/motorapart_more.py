"""The fresh readers, the quick map, the inside-learning test, the long lives and the speed
check of scripts/motorapart.py (idea 184). Quick-map lives go through scripts/oneset.py (its
caches in out/oneset/); the inside test is `blobmnist ceiling` with the commands of the jobs
whose numbers it stands beside (capdefault, everyevent), so their finished runs in the main
checkout are copied for the default arm in place of running again."""

import json
import os
import re
import shutil
import statistics
import subprocess

import capdefault as C
import capdefault_more as CM
import inside as I
import motorapart as A
import oneset as O

ROOT = O.ROOT
MAIN = "/media/vega/mdot2b/Coding2/AI/thonksheet/out/from-agents"
MNIST = os.path.join(ROOT, "build", "blobmnist")
# the arm the map, the inside test and the long lives run when --arms does not say; set
# after the screen was read (docs/results-2026-11-26-motor-apart.md, part 1)
BEST = ["out1"]
THEN = {"echo2": "echo", "bandit": "bandit", "xor": "both-not-either", "far3": "echo"}


def sd_err(d):
    return statistics.stdev(d) / len(d) ** 0.5 if len(d) > 1 else 0.0


# ---- part 1: the fresh reader after a life ----

def reader_file(opt, row, col, arm, seed):
    return os.path.join(opt["out"], "reader", "%s-%s-%s-s%d.txt" % (row, col, arm, seed))


def reader_job(opt, rows, cols, row, col, arm, seed):
    """MNIST: `blobmnist ceiling` after a judged life of the quick map's length under the
    column's rules (the motor cells put back as born, then a linear reader of the non-sensor
    cells). The toy rows: `blobprobe transfer --keep-output 1`, a fresh output of the same
    rule on the frozen lived sheet"""
    genome = A.ARMS[arm][1].split()
    out = reader_file(opt, row, col, arm, seed)
    q = opt["quick"]
    if row == "mnist":
        sheet = [f for i in range(0, len(rows[row]), 2) for f in rows[row][i:i + 2]
                 if rows[row][i] not in ("--task", "--sensor-side")]
        cmd = ["nice", "-n", "19", MNIST, "ceiling"] + sheet + I.READER + [
            "--fit", "2000" if q else "60000", "--seed", str(seed), "--life",
            "600" if q else "20000", "--life-teach", "1"] + cols[col] + genome
        return cmd + (["--test-count", "1000"] if q else []), out
    flags = O.merged(O.LIFE, rows[row], cols[col] + genome)
    if q:
        flags = O.merged(flags, ["--steps", "2000", "--window", "500"])
    own = [f for i in range(0, len(flags), 2) for f in flags[i:i + 2] if flags[i].startswith("--g.")]
    life = [f for i in range(0, len(flags), 2) for f in flags[i:i + 2]
            if not flags[i].startswith("--g.")]
    steps = life[life.index("--steps") + 1]
    cmd = ["nice", "-n", "19", A.PROBE, "transfer", "--seed", str(seed), "--life", " ".join(life),
           "--then", THEN[row], "--then-steps", steps, "--window", "500" if q else "1000",
           "--keep-output", "1"] + own
    return cmd, out


def reader_read(path, row):
    if not os.path.exists(path):
        return None
    text = open(path).read()
    if row == "mnist":
        m = re.search(r"sensor cells left out\s+\d+\s+([0-9.]+)", text)
        r = re.search(r"frozen, fit digits.*?: ([0-9.]+) spikes", text)
        right = re.findall(r"readout right ([0-9.]+)", text)
        return {"score": float(m.group(1)), "rate": float(r.group(1)) if r else None,
                "right": float(right[-1]) if right else None} if m else None
    m = re.search(r"first quarter ([0-9.]+) late ([0-9.]+)", text)
    return {"score": float(m.group(2)), "early": float(m.group(1))} if m else None


def reader(opt):
    rows, cols = O.read_map(O.MORE)
    rows = dict(rows)
    rnames, cnames, arms, seeds = A.SCREEN
    rnames, cnames = A.pick(opt, "rows", rnames), A.pick(opt, "cols", cnames)
    arms = A.pick(opt, "arms", arms)
    seeds = A.seeds_for(opt, seeds)
    if opt["quick"]:
        rnames, cnames, arms = ["mnist", "xor"], cnames[:1], arms[:2]
    jobs = [reader_job(opt, rows, cols, row, col, arm, s) for row in rnames for col in cnames
            for arm in arms for s in seeds]
    jobs.sort(key=lambda j: "mnist-" not in j[1])
    if not opt["table_only"]:
        C.pool(jobs, opt["jobs"])
    lines = ["The fresh reader after a life, seeds %s. MNIST: a linear reader of the 3,072 "
             "non-sensor cells, fit on 60,000 digits and scored on test digits, after a judged "
             "life of 20,000 digits under the column's rules (motor cells put back as born); "
             "in brackets the hidden cells' spikes per 1,000 cell-ticks in the read, and the "
             "share the sheet's own output got right over the last tenth of that life. The "
             "toy rows: a fresh output of the same rule on the frozen lived sheet, late score "
             "(its first quarter in brackets). Beside each arm its paired difference to std." %
             ",".join(map(str, seeds)), ""]
    for row in rnames:
        lines += ["", "%s:" % row, "", "| arm | " + " | ".join(cnames) + " |",
                  "|---" * (len(cnames) + 1) + "|"]
        for arm in A.pick(opt, "arms", list(A.ARMS)):
            cells = []
            for col in cnames:
                got = [reader_read(reader_file(opt, row, col, arm, s), row) for s in seeds]
                std = [reader_read(reader_file(opt, row, col, "std", s), row) for s in seeds]
                have = [g for g in got if g]
                if not have:
                    cells.append("-")
                    continue
                side = ("[%s, %s]" % (C.fmt(C.mean([g["rate"] for g in have]), 1),
                                      C.fmt(C.mean([g["right"] for g in have]), 3))
                        if row == "mnist" else "(%s)" % C.fmt(C.mean([g["early"] for g in have])))
                d = [g["score"] - b["score"] for g, b in zip(got, std) if g and b]
                diff = "" if arm == "std" or not d else " %+.4f ±%.4f" % (
                    statistics.mean(d), sd_err(d))
                cells.append("%s %s%s" % (C.fmt(C.mean([g["score"] for g in have]), 4), side,
                                          diff))
            if any(c != "-" for c in cells):
                lines.append("| %s | %s |" % (arm, " | ".join(cells)))
    return "\n".join(lines)


# ---- part 2: the quick map ----

def quick_map(opt):
    """base, a2rucomp and best with the arm's numbers, each life beside its twin under the same
    numbers, against the cached cap-64 lives"""
    rows, cols = O.read_map(O.MORE)
    arms = A.pick(opt, "arms", BEST)
    names = A.pick(opt, "cols", A.COLS)
    seeds = A.seeds_for(opt, list(range(5, 13)))
    only = ["echo2", "chainfirst"] if opt["quick"] else opt.get("rows")
    var = [("64", "")] + [(a, A.ARMS[a][1]) for a in arms]
    if not opt["table_only"]:
        for col in names:
            for tag, f in var:
                CM.run_column("ma-%s-%s" % (col, tag), cols[col] + f.split(), seeds, only, opt)
    cache = O.load_cache()
    lines = ["The quick map, seeds %s: each column as it is (the cached cap-64 lives) and "
             "beside it with %s. `*` passes (halfway from chance to perfect and above its "
             "learning-off twin, which runs under the same numbers, by 2 standard errors); in "
             "brackets the paired difference to the column as it is, `+` / `-` at 2 standard "
             "errors." % (",".join(map(str, seeds)), ", ".join(
                 "`%s` (`@%s`)" % (A.ARMS[a][1], a) for a in arms)), ""]
    CM.map_block(cache, O.pick_rows(rows, ",".join(only) if only else ""), cols, seeds, var,
                 lines, names)
    return "\n".join(lines)


# ---- part 2: the inside-learning test ----

PAIR = "--pair 2 --pair-mix 1"
OUT = "--life %d --g.learn_mode 5 --life-outside 1 --g.hop_on 1 --g.hop_bits 12 --g.hop_shift 0"
# (test, sheet): what, flags after --seed. born, competitive learning as the cap-default
# job ran them (capdefault_more.INSIDE); the one-hop rule with every-event counters as the
# every-event job's headline arms (everyevent.py: `out` on MNIST, `outf3` on the pair test)
INSIDE = {
    ("mnist", "born"): ("the born sheet", ""),
    ("mnist", "born-tw"): ("born, the tile's winners at 4 in the read (competitive "
                           "learning's own born sheet)", CM.B5),
    ("mnist", "comp"): ("competitive learning, a life with no reward", "--life %d " + CM.B5),
    ("mnist", "hop"): ("the one-hop rule, 12-bit every-event counters, an outside output's "
                       "error (fine under 0.1)", OUT),
    ("pair", "born"): ("the born sheet", PAIR),
    ("pair", "comp"): ("competitive learning in a taught life", CM.TAUGHT + " " + PAIR + " " +
                       CM.COMPETE),
    ("pair", "hop"): ("the one-hop rule, the same, fine under 0.3", OUT +
                      " --outside-fine-1000 300 " + PAIR),
}
OLD = ["capdefault/inside", "everyevent/report"]


def inside_cmd(test, sheet, genome, seed, opt):
    life, fit = (600, 2000) if opt["quick"] else (60000, 60000)
    flags = INSIDE[test, sheet][1]
    flags = flags % life if "%d" in flags else flags
    cmd = ["ceiling"] + I.SHEET + I.READER + ["--fit", str(fit), "--seed", str(seed)]
    return cmd + flags.split() + genome.split() + (["--test-count", "1000"] if opt["quick"]
                                                   else [])


def flag_key(words):
    """a command's flags whatever their order: the earlier jobs wrote them in other orders"""
    pairs = sorted(" ".join(words[i:i + 2]) for i in range(1, len(words) - 1, 2))
    return "|".join(pairs)


def old_runs():
    found = {}
    for sub in OLD:
        folder = os.path.join(MAIN, sub)
        for f in sorted(os.listdir(folder)) if os.path.isdir(folder) else []:
            path = os.path.join(folder, f)
            if not f.endswith(".txt") or f.endswith("-check.txt") or not os.path.isfile(path):
                continue
            with open(path) as h:
                first = h.readline().split()
                rest = h.read()
            if "total wall" in rest and "ceiling" in first:
                found[flag_key(first[first.index("ceiling"):])] = path
    return found


def inside_file(opt, test, sheet, arm, seed, check=False):
    return os.path.join(opt["out"], "inside", "%s-%s-%s-%d%s.txt" % (
        test, sheet, arm, seed, "-check" if check else ""))


def inside_test(opt):
    seeds = A.seeds_for(opt, list(range(5, 13)))
    # --skip-std: only the arms (a machine without the earlier jobs' outputs to copy)
    arms = ([] if opt.get("skip_std") else ["std"]) + A.pick(opt, "arms", BEST)
    old = {} if opt["quick"] or opt["table_only"] else old_runs()
    jobs, copied = [], set()
    for (test, sheet) in INSIDE:
        for arm in arms:
            for s in seeds:
                cmd = inside_cmd(test, sheet, A.ARMS[arm][1], s, opt)
                out = inside_file(opt, test, sheet, arm, s)
                src = old.get(flag_key(cmd)) if arm == "std" else None
                if src and not os.path.exists(out):
                    os.makedirs(os.path.dirname(out), exist_ok=True)
                    shutil.copyfile(src, out)
                if src:
                    copied.add((test, sheet))
                    # one seed of every copied group is run again with this build
                    if s == seeds[0]:
                        jobs.append((["nice", "-n", "19", MNIST] + cmd,
                                     inside_file(opt, test, sheet, arm, s, True)))
                else:
                    jobs.append((["nice", "-n", "19", MNIST] + cmd, out))
    if not opt["table_only"]:
        C.pool(jobs, opt["jobs"])
    lines = ["The inside-learning test: a fresh linear reader of the non-sensor cells, fit on "
             "60,000 training digits, scored on test digits; seeds %s. rate: the hidden cells' "
             "spikes per 1,000 cell-ticks while the reader's digits are shown. lived - born: "
             "paired by seed under the same arm (competitive learning on MNIST against "
             "born-tw). arm - std: paired by seed." % ",".join(map(str, seeds)), "",
             "| test | sheet | arm | score | rate | lived - born | arm - std | per seed |",
             "|---|---|---|---|---|---|---|---|"]
    read = lambda t, sh, a, s, c=False: CM.inside_read(inside_file(opt, t, sh, a, s, c))  # noqa: E731
    for (test, sheet) in INSIDE:
        for arm in arms:
            got = [read(test, sheet, arm, s) for s in seeds]
            if not any(got):
                continue
            born_of = "born-tw" if (test, sheet) == ("mnist", "comp") else "born"
            born = [read(test, born_of, arm, s) for s in seeds]
            std = [read(test, sheet, "std", s) for s in seeds]
            d = [g["score"] - b["score"] for g, b in zip(got, born) if g and b]
            e = [g["score"] - b["score"] for g, b in zip(got, std) if g and b]
            lines.append("| %s | %s | %s | %s | %s | %s | %s | %s |" % (
                test, sheet, arm, C.fmt(C.mean([g["score"] for g in got if g]), 4),
                C.fmt(C.mean([g["rate"] for g in got if g]), 1),
                "" if sheet.startswith("born") or len(d) < 2 else "%+.4f ±%.4f" % (
                    statistics.mean(d), sd_err(d)),
                "" if arm == "std" or len(e) < 2 else "%+.4f ±%.4f" % (
                    statistics.mean(e), sd_err(e)),
                " ".join(C.fmt(g["score"], 4) if g else "-" for g in got)))
    lines += ["", "Sheets: " + "; ".join("%s %s: %s" % (t, a, INSIDE[t, a][0]) for t, a in INSIDE)]
    checks = []
    for (test, sheet) in INSIDE:
        a, b = read(test, sheet, "std", seeds[0]), read(test, sheet, "std", seeds[0], True)
        if b:
            checks.append("%s %s: copied %s, run again %s" % (
                test, sheet, C.fmt(a["score"], 4) if a else "-", C.fmt(b["score"], 4)))
    if checks:
        lines += ["", "The std rows are the earlier jobs' runs, copied (same flags); seed %d run "
                  "again with this build: " % seeds[0] + "; ".join(checks)]
    return "\n".join(lines)


# ---- long lives (the rate-control job's first test) ----

def alive(opt):
    """160,000-step lives (echo at 3 tiles, delayed echo of 2) and the 20,000-digit MNIST
    world under base, base with competitive learning and the one setting, as
    scripts/ratecontrol.sh's alive test (`blobprobe ratehealth`). std is that job's runs"""
    import ratecontrol as RC
    import ratecontrol_tests as T
    seeds = A.seeds_for(opt, [5, 6])
    names = ["out1"] if opt["quick"] else ["std"] + A.pick(opt, "arms", BEST)
    lines = ["Long lives, seeds %s (std: the rate-control job's runs). Mean of the seeds at the "
             "end of the life: rate over target; silent and stuck shares of non-sensor cells; "
             "hidden thresholds at the cap; silent share of plain hidden cells; the first 100 "
             "ticks back after a 2,000-tick gap over the rate before; late score." %
             ",".join(map(str, seeds)), "",
             "| arm | setting | world | rate | silent | stuck | at cap | hidden silent | gap "
             "jump | late |", "|---|---|---|---|---|---|---|---|---|---|"]
    for name in names:
        old = os.path.join(MAIN, "ratecontrol", "alive", name)
        copied = name == "std" and not opt["quick"]
        if copied and os.path.isdir(old):
            folder = os.path.join(opt["out"], "alive", name)
            os.makedirs(folder, exist_ok=True)
            for f in os.listdir(old):
                if f.endswith(".txt") and not os.path.exists(os.path.join(folder, f)):
                    shutil.copyfile(os.path.join(old, f), os.path.join(folder, f))
        o = {"out": opt["out"], "seeds": seeds, "quick": opt["quick"], "jobs": opt["jobs"],
             "table_only": opt["table_only"] or copied}
        _, detail, _ = RC.alive_row(T.alive(name, A.ARMS[name][1].split(), o))
        for x in detail:
            lines.append("| %s | %s | %s | %s |" % (name, x[0], x[1], " | ".join(
                C.fmt(v, 3) for v in (x[2], x[3], x[4], x[5], x[6], x[8], x[9]))))
    return "\n".join(lines)


# ---- speed ----

def speed(opt):
    """blobbench: the build before this job (out/motorapart/before/blobbench, kept from the
    branch point) and this build with every number off and with each kind on, taking turns"""
    after = os.path.join(ROOT, "build", "blobbench")
    before = os.path.join(ROOT, "out", "motorapart", "before", "blobbench")
    rounds = 1 if opt["quick"] else 5
    confs = [("before", before, "")] if os.path.exists(before) else []
    confs += [("after, all off", after, "")] + [
        ("after, %s" % a, after, A.ARMS[a][1]) for a in ("out1", "out2", "own", "walk128")]
    got = {tag: {"off": [], "on": []} for tag, _, _ in confs}
    log = os.path.join(opt["out"], "speed.txt")
    if not opt["table_only"]:
        with open(log, "w") as f:
            for r in range(rounds):
                for tag, bench, flags in confs:
                    done = subprocess.run(["nice", "-n", "19", bench, "speed", "--tiles", "8",
                                           "--seconds-ms", "200" if opt["quick"] else "1500"]
                                          + flags.split(), capture_output=True, text=True,
                                          cwd=ROOT)
                    f.write("round %d conf %s\n%s" % (r, json.dumps(tag), done.stdout))
    tag = None
    for line in open(log) if os.path.exists(log) else []:
        p = line.split()
        if p and p[0] == "round":
            tag = json.loads(line.split(" conf ", 1)[1])
        elif len(p) == 4 and p[1] in ("off", "on") and tag in got:
            got[tag][p[1]].append(float(p[2]))
    lines = ["`blobbench speed --tiles 8` (4,096 cells, one core, this PC under the temperature "
             "governor: rough), %d rounds taking turns, ticks a second, lowest to highest "
             "(median):" % rounds, "", "| build | learning off | learning on |", "|---|---|---|"]
    for tag, _, _ in confs:
        cells = []
        for k in ("off", "on"):
            v = got[tag][k]
            cells.append("%.0f to %.0f (%.0f)" % (min(v), max(v), statistics.median(v)) if v
                         else "-")
        lines.append("| %s | %s |" % (tag, " | ".join(cells)))
    return "\n".join(lines)
