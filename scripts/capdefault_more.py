"""The map, the inside-learning test and the speed check of scripts/capdefault.py (idea 180).
Quick-map lives go through scripts/oneset.py (its caches in out/oneset/); the inside test is
`blobmnist ceiling` with the commands of the jobs whose numbers it repeats (compete2, hop,
hop2), so their finished runs in the main checkout are copied in place of running again."""

import contextlib
import io
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
FIXES = {}
MUST = ["xor", "echo16", "order", "mnist"]


def mean(v):
    got = [x for x in v if x is not None]
    return statistics.mean(got) if got else None


def fmt(v, d=4):
    return "-" if v is None else ("%." + str(d) + "f") % v


# ---- the quick map ----

def variants(opt):
    out = [("64", ""), ("1000", CAP)]
    for name in opt.get("fixes") or FIXES:
        out.append((name, FIXES[name]))
    return out


def run_column(name, flags, seeds, rows, opt):
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


def map_block(cache, rows, cols, seeds, var, lines):
    """one table per column (each variant beside cap 64), then what changed"""
    for col in COLS:
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
    if not opt["table_only"]:
        for col in COLS:
            for tag, f in var:
                run_column("cap-%s-%s" % (col, tag), cols[col] + f.split(), seeds, only, opt)
                if not opt["quick"] and not opt["seeds"]:
                    run_column("cap-%s-%s" % (col, tag), cols[col] + f.split(), early,
                               early_rows, opt)
    cache = O.load_cache()
    lines = ["The quick map, seeds %s, each column at cap 64 and beside it with the cap at 1000"
             " (`@1000`)%s." % (",".join(map(str, seeds)), "".join(
                 " and with `%s` (`@%s`)" % (FIXES[n], n) for n, _ in var[2:])), ""]
    map_block(cache, O.pick_rows(rows, ",".join(only) if only else ""), cols, seeds, var, lines)
    if not opt["quick"] and not opt["seeds"]:
        lines += ["Seeds 1-4, the rows only a critic passed or lifted:", ""]
        map_block(cache, O.pick_rows(rows, ",".join(early_rows)), cols, early, var, lines)
    # the sheet's rate at the end of each life, as blobrun counts it (sensor cells included)
    lines += ["Rate over target at the end of the life (blobrun's count, sensor cells "
              "included; MNIST's is high for that reason) and share of thresholds at the cap, "
              "mean of the seeds, cap 64 / cap 1000:", ""]
    for col in COLS:
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
    ("mnist", "born"): ("the born sheet", ""),
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
OLD = ["compete2/mnist/b5", "compete2/mnist/b5cap", "hop/inside/hop2s7-report",
       "hop/inside/hop2s7cap-report", "hop2/same"]


def inside_cmd(test, arm, cap, seed, opt):
    life, fit = (600, 2000) if opt["quick"] else (60000, 60000)
    flags = INSIDE[test, arm][1]
    flags = flags % life if "%d" in flags else flags
    cmd = ["ceiling"] + I.SHEET + I.READER + ["--fit", str(fit), "--seed", str(seed)]
    cmd += flags.split() + (CAP.split() if cap == 1000 else [])
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
        for cap in (64, 1000):
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
             "paired by seed, with its standard error." % ",".join(map(str, seeds)), "",
             "| test | sheet | cap | score | rate | lived - born | per seed |",
             "|---|---|---|---|---|---|---|"]
    for test in ("mnist", "pair"):
        for cap in (64, 1000):
            born = [inside_read(inside_file(opt, test, "born", cap, s)) for s in seeds]
            for arm in [a for t, a in INSIDE if t == test]:
                got = [inside_read(inside_file(opt, test, arm, cap, s)) for s in seeds]
                if not any(got):
                    continue
                d = [g["score"] - b["score"] for g, b in zip(got, born) if g and b]
                diff = "" if arm == "born" or len(d) < 2 else "%+.4f ±%.4f" % (
                    statistics.mean(d), statistics.stdev(d) / len(d) ** 0.5)
                lines.append("| %s | %s | %d | %s | %s | %s | %s |" % (
                    test, arm, cap, fmt(mean([g["score"] for g in got if g])),
                    fmt(mean([g["rate"] for g in got if g]), 1), diff,
                    " ".join(fmt(g["score"]) if g else "-" for g in got)))
    lines += ["", "Sheets: " + "; ".join("%s %s: %s" % (t, a, INSIDE[t, a][0]) for t, a in INSIDE)]
    checks = []
    for (test, arm) in INSIDE:
        for cap in (64, 1000):
            a = inside_read(inside_file(opt, test, arm, cap, seeds[0]))
            b = inside_read(inside_file(opt, test, arm, cap, seeds[0], True))
            if b:
                checks.append("%s %s cap %d: copied %s, run again %s" % (
                    test, arm, cap, fmt(a["score"]) if a else "-", fmt(b["score"])))
    if checks:
        lines += ["", "Copied from the earlier jobs' outputs (same flags), seed %d run again "
                  "with this build: " % seeds[0] + "; ".join(checks)]
    return "\n".join(lines)


# ---- speed ----

def speed(opt):
    bench = os.path.join(ROOT, "build", "blobbench")
    rounds = 1 if opt["quick"] else 5
    got = {"64": {"off": [], "on": []}, "1000": {"off": [], "on": []}}
    log = os.path.join(opt["out"], "speed.txt")
    if not opt["table_only"]:
        with open(log, "w") as f:
            for r in range(rounds):
                for tag, flags in (("64", []), ("1000", CAP.split())):
                    done = subprocess.run(["nice", "-n", "19", bench, "speed", "--tiles", "8",
                                           "--seconds-ms", "200" if opt["quick"] else "1500"]
                                          + flags, capture_output=True, text=True, cwd=ROOT)
                    f.write("round %d cap %s\n%s" % (r, tag, done.stdout))
    tag = None
    for line in open(log) if os.path.exists(log) else []:
        p = line.split()
        if p and p[0] == "round":
            tag = p[3]
        elif len(p) == 4 and p[1] in ("off", "on") and tag:
            got[tag][p[1]].append(float(p[2]))
    lines = ["`blobbench speed --tiles 8` (4,096 cells, one core, this PC under the temperature "
             "governor: rough), %d rounds taking turns, ticks a second, lowest to highest "
             "(median):" % rounds, "", "| cap | learning off | learning on |", "|---|---|---|"]
    for tag in ("64", "1000"):
        cells = []
        for k in ("off", "on"):
            v = got[tag][k]
            cells.append("%.0f to %.0f (%.0f)" % (min(v), max(v), statistics.median(v)) if v
                         else "-")
        lines.append("| %s | %s |" % (tag, " | ".join(cells)))
    return "\n".join(lines)
