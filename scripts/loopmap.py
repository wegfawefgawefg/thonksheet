#!/usr/bin/env python3
"""The closed loop on the whole quick map, an output that hears only good and bad, and
parity of 3 (idea 188; docs/results-2026-11-26-loop-everywhere.md). See scripts/loopmap.sh.
Each NAME is a fixed recipe; one line runs what is missing and prints its table."""

import os
import statistics
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
os.environ.setdefault("EVERYEVENT_BUILD", os.environ.get("LOOPMAP_BUILD", "build"))
import everyevent as ee  # noqa: E402  the MNIST sheet's jobs and arms
import hop2  # noqa: E402  running one job
import oneset  # noqa: E402  the quick map: rows, lives, the cache, the pass rule
import outputinside as oi  # noqa: E402  the closed loop's arms, the toys' jobs and tables

OUT = os.path.join(ROOT, "out", "loopmap")
CACHE = os.path.join(ROOT, "out", "oneset", "cache-loopmap.jsonl")
EV = ee.EV  # the one-hop rule on 12-bit planes, every marked input steps

# the quick map's columns. ALONE: the blob's own output on a sheet in which nothing else
# learns; LOOP: its steps are also the one-hop rule's errors. told the right line unless
# out_judge is given (then it hears only good or bad for the line it pressed)
ALONE = "--g.learn_mode 5 --motor-spread 128 --g.out_on 1 --g.out_top 1"
LOOP = ALONE + " " + EV + " --g.hop_teach 6"
KNOBS = " --g.out_window 10 --g.hop_who 3 --motor-spread-even 1"  # what validation liked
COMP = " --g.tile_winners 4 --g.compete_on 1 --g.compete_shift 5"  # competitive learning
COLS = {"alone": ALONE, "loop": LOOP, "loopw": LOOP + KNOBS, "alonew": ALONE + KNOBS,
        # competitive learning beside the output alone, and beside the loop (two stores)
        "alonecomp": ALONE + COMP, "loopcomp": LOOP + COMP,
        # one store: the one-hop rule on the tag store's bytes, as competitive learning
        "loopbyte": ALONE + " --g.hop_on 1 --g.hop_shift 3 --g.hop_teach 6",
        "loopbytecomp": ALONE + " --g.hop_on 1 --g.hop_shift 3 --g.hop_teach 6" + COMP,
        # one store: competitive learning's counts in the one-hop rule's planes
        "loopcomp1": LOOP + COMP + " --g.compete_store 1"}
for j in (1, 2, 3, 4):
    COLS["alonej%d" % j] = ALONE + " --g.out_judge %d" % j
    COLS["loopj%d" % j] = LOOP + " --g.out_judge %d" % j
    COLS["loopj%dt" % j] = LOOP + " --g.out_judge %d --g.out_tell 1" % j
    COLS["loopj%dg" % j] = LOOP + " --g.out_judge %d --g.out_tell 2" % j
# narrow counters: does breaking one-down-for-one-up (form 4) cost when the counters are short
for b in (6, 8):
    COLS["aloneb%d" % b] = ALONE + " --g.out_bits %d" % b
    COLS["alonej2b%d" % b] = ALONE + " --g.out_judge 2 --g.out_bits %d" % b
    COLS["alonej4b%d" % b] = ALONE + " --g.out_judge 4 --g.out_bits %d" % b
CACHED = ["base", "a2rucomp", "best"]  # columns of the one-setting docs, from the caches

# rows where the world cannot name a right line: its "right" is the scripted player's move,
# which the task is there to find from good and bad
ACTING = ["bandit", "chain", "cartpole", "catch"]
MANY = ["echo8", "echo16", "noisy", "pairs", "mnist"]  # more than two buttons

# the MNIST sheet (blobmnist ceiling, a 60,000-digit life, then a fresh linear reader), told
# only good or bad. alone: the output on a sheet that does not learn; in: the closed loop
SHEET_ALONE = "--g.learn_mode 5 --life-teach 6 --g.out_on 1 --g.out_top 1 --motor-spread-even 1"
ARMS = {}
for j in (1, 2, 3, 4):
    ARMS["alonej%d" % j] = (SHEET_ALONE + " --g.out_judge %d" % j, None)
    ARMS["inj%d" % j] = (oi.loop(oi.TOPE + " --g.out_judge %d" % j), oi.COIN_IN)
    ARMS["inj%dt" % j] = (oi.loop(oi.TOPE + " --g.out_judge %d --g.out_tell 1" % j), oi.COIN_IN)
    ARMS["inj%dg" % j] = (oi.loop(oi.TOPE + " --g.out_judge %d --g.out_tell 2" % j), oi.COIN_IN)
    ARMS["inj%dw" % j] = (oi.loop(oi.TOPE + " --g.out_judge %d --g.out_window 10 --g.hop_who 3"
                                  % j), oi.COIN_IN)
# a mend for parity of 3 must not cost these: the closed loop with the motor cells learning
ARMS["intopecells"] = (oi.loop(oi.TOPE + " --g.out_cells 1"), oi.COIN_IN)
ee.ARMS.update(ARMS)

# parity of 3 (and xor): a 40,000-step life, the inside frozen, a fresh old readout's late
# score. OLD: the every-event arm (the old readout answers, the rule told the right button
# on bad steps). IN: the closed loop
OLD = EV + " --g.hop_teach 2"
IN = EV + oi.TOY_OUT
AFTER = oi.OLD_AFTER
OWN = ["--after", "out_on 1 out_top 1"]  # a fresh output of the new kind on an old-arm sheet
TOYS = [
    ("born", None, []), ("old", OLD, []), ("inold", IN, AFTER),
    # when the cells are told: the new output answers, the cells hear only of bad steps
    ("in2old", EV + " --g.hop_teach 2 --g.out_on 1 --g.out_top 1", AFTER),
    # the old readout answers, the cells told by margin on its spikes (bad and close steps)
    ("old5", EV + " --g.hop_teach 5 --g.hop_fine 8", []),
    # spikes: the cells may not count or wire a motor cell
    ("olddeaf", OLD + " --g.hop_deaf 1", []), ("indeaf", IN + " --g.hop_deaf 1", AFTER),
    # spikes: beside the new output the motor cells learn by the old readout's rule
    ("incells", IN + " --g.out_cells 1", ["--after", "out_on 0 out_cells 0"]),
    # the margin on a tiny world: none (a plain perceptron), one step's worth
    ("inm0old", IN + " --g.out_margin 0", AFTER), ("inm1old", IN + " --g.out_margin 1", AFTER),
    # the knobs validation liked on MNIST
    ("inwold", IN + " --g.out_window 10 --g.hop_who 3", AFTER),
    # round 2. the cross reads: each arm's sheet read by a fresh output of the other kind
    ("bornown", None, ["--g.out_on", "1", "--g.out_top", "1"]), ("in", IN, []),
    ("oldown", OLD, OWN), ("olddeafown", OLD + " --g.hop_deaf 1", OWN),
    ("in2own", EV + " --g.hop_teach 2 --g.out_on 1 --g.out_top 1", []),
    # everything level but who answers: told on bad steps only, no motor cell heard
    ("in2deaf", EV + " --g.hop_teach 2 --g.out_on 1 --g.out_top 1 --g.hop_deaf 1", AFTER),
    # less teaching: the cells' zone one step's worth; the cells re-pick every 64 verdicts
    ("inf1old", IN + " --g.out_fine 1", AFTER), ("in64old", IN + " --g.hop_rewire_every 64", AFTER),
    # which of the two knobs, the two with bad steps only, and their sheet read by the new kind
    ("inw10old", IN + " --g.out_window 10", AFTER), ("inallold", IN + " --g.hop_who 3", AFTER),
    ("in2wold", EV + " --g.hop_teach 2 --g.out_on 1 --g.out_top 1 --g.out_window 10 "
     "--g.hop_who 3", AFTER),
    ("inw", IN + " --g.out_window 10 --g.hop_who 3", []),
    ("oldall", OLD + " --g.hop_who 3", []),
]

PARITY8 = ["born", "old", "olddeaf", "inold", "inwold", "bornown", "in", "oldown", "inw"]  # the arms run on 8 seeds
RECIPES = {
    # the quick map. screen: seeds 1-3; report: seeds 5-12
    "mapscreen": {"kind": "map", "seeds": "1-3", "cols": ["alone", "loop"]},
    "map": {"kind": "map", "seeds": "5-12", "cols": ["alone", "loop"]},
    "mapknobs": {"kind": "map", "seeds": "1-3", "cols": ["loop", "loopw"]},
    # good and bad only, the rows with more than two buttons and the acting rows first
    "judgescreen": {"kind": "map", "seeds": "1-3", "rows": MANY + ACTING, "cols": [
        "alone", "loop", "alonej1", "loopj1", "alonej2", "loopj2", "alonej3", "loopj3",
        "alonej4", "loopj4", "loopj1t", "loopj2t", "loopj3t", "loopj2g"]},
    "judge": {"kind": "map", "seeds": "5-12", "cols": ["alone", "loop", "alonej2", "loopj2"]},
    # the check that with two buttons good-or-bad is the told rule: the same lives
    "same": {"kind": "map", "seeds": "1-3", "run_same": True, "cols": ["alonej2", "loopj2"],
             "rows": ["echo2", "shuffled", "far3", "far5", "xor", "parity3", "order", "gap",
                      "delayed2", "hold1000", "copy2", "inv-scale", "inv-position"]},
    # the control that breaks the discipline, at 12, 8 and 6 bits a counter
    "judgebits": {"kind": "map", "seeds": "1-3", "rows": ["mnist", "echo16"], "cols": [
        "alone", "alonej2", "alonej4", "aloneb8", "alonej2b8", "alonej4b8", "aloneb6",
        "alonej2b6", "alonej4b6"]},
    # MNIST told only good or bad: validation, seeds 1 and 2; then the test digits
    "judgesheet": {"kind": "sheet", "val": True, "seeds": [1, 2], "tests": ["mnist"], "arms": [
        "born", "alonee", "intope", "alonej2", "inj2", "inj2t", "inj2g", "inj2w", "alonej4",
        "inj4", "inj1", "inj3"], "coins": [], "shuffled": []},
    "judgereport": {"kind": "sheet", "val": False, "seeds": list(range(5, 13)),
                    "tests": ["mnist"], "arms": ["born", "alonee", "intope", "intopeallw10",
                                                 "alonej2", "inj2", "inj2w"],
                    "coins": ["intope", "inj2"], "shuffled": []},
    # parity of 3
    "parityscreen": {"kind": "toy", "tasks": ["parity3"], "seeds": [5, 6, 7], "settings": TOYS},
    "parity": {"kind": "toy", "tasks": ["parity3"], "seeds": list(range(5, 13)),
               "folder": "parityscreen", "settings": [t for t in TOYS if t[0] in PARITY8]},
    # a mend must not cost MNIST or the pair test: validation, 3 seeds
    "paritycost": {"kind": "sheet", "val": True, "seeds": [1, 2, 3], "arms": [
        "intope", "intopecells"], "coins": [], "shuffled": []},
    # one counter store: competitive learning beside the loop, on two rows
    "store": {"kind": "map", "seeds": "1-3", "rows": ["gap", "delayed2", "parity3"], "cols": [
        "alone", "loop", "alonecomp", "loopcomp", "loopcomp1", "loopbyte", "loopbytecomp"]},
    "smoke": {"kind": "map", "seeds": "5", "rows": ["echo8", "xor", "bandit"], "cols": [
        "alone", "loop", "alonej2", "loopj2"], "then": ["parityscreen", "judgesheet"]},
}


def twin_flags(col):
    """the learning-off twin of a column: oneset's (the learning-only numbers dropped,
    learning off) with the one-hop rule's, the output's and competitive learning's numbers
    dropped too. off, they change nothing (every vote stays zero and a tie goes to the
    lowest line), so every column here with the same layout shares one twin"""
    t = oneset.twin_args(col)
    out = []
    for i in range(0, len(t) - 1, 2):
        k = t[i]
        drop = (k.startswith("--g.hop_") or k.startswith("--g.compete_") or
                (k.startswith("--g.out_") and k != "--g.out_on"))
        out += [] if drop else [t[i], t[i + 1]]
    return out


def told_flags(flags):
    """a good-or-bad column's flags with out_judge and out_tell taken off"""
    out = []
    for i in range(0, len(flags) - 1, 2):
        out += [] if flags[i] in ("--g.out_judge", "--g.out_tell") else [flags[i], flags[i + 1]]
    return out


def same_as_told(rname, flags):
    """on a two-button row whose verdict is "the pressed button was (not) the answer", a
    good-or-bad life (forms 1 to 3) is the told-the-right-line life byte for byte (checked
    by the recipe `same`), so the column reads the told life there"""
    judge = flags[flags.index("--g.out_judge") + 1] if "--g.out_judge" in flags else "0"
    tell = flags[flags.index("--g.out_tell") + 1] if "--g.out_tell" in flags else "0"
    return judge in ("1", "2", "3") and tell == "0" and rname not in MANY + ACTING


def lates(cache, rargs, flags, seeds, rname=None):
    if rname and same_as_told(rname, flags):
        flags = told_flags(flags)
    out = []
    for s in seeds:
        row = cache.get(" ".join(oneset.life_cmd(rargs, flags, s)))
        out.append(oneset.edge(row["windows"]) if row else None)
    return out


def cell(late, off, chance, perfect):
    if None in late or None in off:
        return "missing", False
    _, ok = oneset.judge(late, off, chance, perfect)
    return oneset.num(statistics.mean(late)) + ("*" if ok else ""), ok


def map_table(r, cols, rows, seeds, cached_cols):
    cache = oneset.load_cache()
    names = [c for c in CACHED if r.get("cached", True)] + r["cols"]
    lines = ["quick map, seeds %s: late score, `*` passes (halfway from chance to perfect and "
             "over its learning-off twin by 2 standard errors)" % r["seeds"], "",
             "| row | " + " | ".join(names) + " |", "|---" * (len(names) + 1) + "|"]
    passes = {n: 0 for n in names}
    counted = {n: 0 for n in names}
    for rname, rargs in rows:
        chance, perfect = oneset.calib_of(cache, rargs)
        cells = []
        for n in names:
            if n in CACHED:
                flags = cached_cols[n]
                off = lates(cache, rargs, oneset.twin_args(flags), seeds)
            else:
                flags = cols[n].split()
                off = lates(cache, rargs, twin_flags(flags), seeds)
            text, ok = cell(lates(cache, rargs, flags, seeds, rname), off, chance, perfect)
            # a column told the right line is not counted on the acting rows
            told = n not in CACHED and "out_judge" not in cols[n]
            skip = rname == "chain" or (told and rname in ACTING)
            passes[n] += ok and not skip
            counted[n] += not skip and text != "missing"
            cells.append(text + (" (told)" if told and rname in ACTING and text != "missing"
                                 else ""))
        lines.append("| %s | %s |" % (rname, " | ".join(cells)))
    lines.append("| rows passed, of those counted | " + " | ".join(
        "%d of %d" % (passes[n], counted[n]) for n in names) + " |")
    # the cells' share: each loop column beside its output alone, paired by seed
    pairs = [(c, c.replace("loop", "alone").rstrip("tg")) for c in r["cols"]
             if c.startswith("loop")]
    pairs = [(a, b) for a, b in pairs if b in r["cols"]]
    if pairs:
        lines += ["", "the loop less the output alone, paired by seed (standard error); `+`/`-` "
                  "at 2 standard errors", "", "| row | " + " | ".join(
                      "%s - %s" % p for p in pairs) + " |", "|---" * (len(pairs) + 1) + "|"]
        for rname, rargs in rows:
            cells = []
            for a, b in pairs:
                la = lates(cache, rargs, cols[a].split(), seeds, rname)
                lb = lates(cache, rargs, cols[b].split(), seeds, rname)
                if None in la or None in lb:
                    cells.append("missing")
                    continue
                d, se = oneset.paired(la, lb)
                mark = "+" if d > 2 * se and d > 0 else ("-" if d < -2 * se and d < 0 else "")
                cells.append("%+.3f (%.3f)%s" % (d, se, mark))
            lines.append("| %s | %s |" % (rname, " | ".join(cells)))
    return "\n".join(lines)


def map_detail(r, cols, rows, seeds):
    """each column of this job row by row: spread, twin, how far from chance to perfect"""
    cache = oneset.load_cache()
    lines = []
    for n in r["cols"]:
        flags = cols[n].split()
        lines += ["", "%s: `%s`" % (n, cols[n]), "",
                  "| row | late (lowest to highest seed) | twin | late - twin | of the way | pass |",
                  "|---|---|---|---|---|---|"]
        for rname, rargs in rows:
            late = lates(cache, rargs, flags, seeds, rname)
            off = lates(cache, rargs, twin_flags(flags), seeds)
            if None in late or None in off:
                continue
            chance, perfect = oneset.calib_of(cache, rargs)
            norm, ok = oneset.judge(late, off, chance, perfect)
            d, se = oneset.paired(late, off)
            lines.append("| %s | %s (%s to %s) | %s | %+.3f ±%.3f | %s | %s |" % (
                rname, oneset.num(statistics.mean(late)), oneset.num(min(late)),
                oneset.num(max(late)), oneset.num(statistics.mean(off)), d, se,
                "-" if norm is None else "%.2f" % norm, "yes" if ok else ""))
    return "\n".join(lines)


def same_table(r, rows, seeds):
    """the check behind same_as_told: every window of a good-or-bad life against the told
    life's, row by row"""
    cache = oneset.load_cache()
    lines = ["good or bad only against told the right line, every window of every life, "
             "seeds %s" % ",".join(map(str, seeds)), ""]
    for n in r["cols"]:
        flags = COLS[n].split()
        for rname, rargs in rows:
            same = 0
            for s in seeds:
                a = cache.get(" ".join(oneset.life_cmd(rargs, flags, s)))
                b = cache.get(" ".join(oneset.life_cmd(rargs, told_flags(flags), s)))
                same += bool(a and b and a["windows"] == b["windows"])
            lines.append("%-10s %-13s %d of %d lives the same" % (n, rname, same, len(seeds)))
    return "\n".join(lines)


def run_map(name, r, opt):
    rows, cached_cols = oneset.read_map(oneset.MORE)
    if r.get("rows") or opt.get("rows"):
        want = opt.get("rows") or r["rows"]
        rows = [x for x in rows if x[0] in want]
    seeds = oneset.parse_seeds(r["seeds"])
    if opt["quick"]:
        seeds = seeds[:1]
        oneset.LIFE[oneset.LIFE.index("--steps") + 1] = "2000"
        rows = [(n, a if "--steps" not in a else
                 a[:a.index("--steps")] + a[a.index("--steps") + 2:]) for n, a in rows]
    blobrun = os.path.join(ee.BUILD, "blobrun")
    os.makedirs(os.path.join(oneset.OUT, "runs"), exist_ok=True)
    cache = oneset.load_cache()
    todo = []
    for rname, rargs in rows:
        for n in r["cols"]:
            if "out_judge" not in COLS[n] and rname in ACTING and not r.get("told_acting"):
                continue  # told the right line: these rows have none to tell
            flags = COLS[n].split()
            if same_as_told(rname, flags) and not r.get("run_same"):
                flags = told_flags(flags)  # the told life stands in: run that if missing
            for s in seeds:
                for f in (flags, twin_flags(flags)):
                    args = oneset.life_cmd(rargs, f, s)
                    if " ".join(args) not in cache and (args, False) not in todo:
                        todo.append((args, False))
    todo.sort(key=lambda t: ("mnist" not in " ".join(t[0]), "--delay 50" not in " ".join(t[0])))
    if not opt["table_only"]:
        oneset.run_all(todo, opt["jobs"], blobrun, CACHE)
    if r.get("run_same"):
        return same_table(r, rows, seeds)
    r = dict(r, seeds=",".join(map(str, seeds)), cached=not opt["quick"])
    text = map_table(r, COLS, rows, seeds, cached_cols)
    return text + ("\n" + map_detail(r, COLS, rows, seeds) if opt.get("detail") else "")


def one(name, r, opt):
    quick = opt["quick"]
    kind = r["kind"]
    if quick and kind != "map":
        r["seeds"] = r["seeds"][:1]
    outdir = os.path.join(OUT, r.get("folder", name) + ("-quick" if quick else ""))
    os.makedirs(outdir, exist_ok=True)
    if kind == "map":
        text = run_map(name, r, opt)
    else:
        if kind == "sheet":
            jobs, show = ee.sheet_jobs(r, outdir, quick), lambda: oi.table(r, outdir, name)
        else:
            jobs, show = oi.toy_jobs(r, outdir, quick), lambda: oi.toy_table(r, outdir)
        if not opt["table_only"]:
            jobs = jobs[::-1] if opt["reverse"] else jobs
            with ThreadPoolExecutor(max_workers=opt["jobs"]) as pool:
                list(pool.map(hop2.run, jobs))
        text = show()
    print(text)
    open(os.path.join(outdir, "table%s.txt" % ("-" + name if "folder" in r else "")),
         "w").write(text + "\n")


def main():
    args = sys.argv[1:]
    if not args or args[0] not in RECIPES:
        print(open(os.path.join(ROOT, "scripts", "loopmap.sh")).read().split("set -")[0])
        print("tables: " + " ".join(RECIPES))
        print("map columns: " + " ".join(COLS))
        return 1
    name = args.pop(0)
    r = dict(RECIPES[name])
    # the map's own options, then the ones scripts/everyevent.py knows
    rest, more = [], {}
    i = 0
    while i < len(args):
        if args[i] in ("--cols", "--rows") and r["kind"] == "map":
            more[args[i][2:]] = args[i + 1].split(",")
            i += 2
        elif args[i] == "--seeds" and r["kind"] == "map":
            r["seeds"] = args[i + 1]
            i += 2
        elif args[i] == "--detail":
            more["detail"] = True
            i += 1
        else:
            rest.append(args[i])
            i += 1
    if "cols" in more:
        r["cols"] = more["cols"]
    shape = dict(r) if r["kind"] != "map" else {"kind": "sheet"}
    opt = ee.parse(rest, shape)
    if opt is None:
        return 1
    if r["kind"] != "map":
        r = shape
    opt["rows"] = more.get("rows")
    opt["detail"] = more.get("detail")
    one(name, r, opt)
    for then in r.get("then", []) if opt["quick"] else []:
        r2 = dict(RECIPES[then])
        if r2["kind"] == "toy":
            r2["settings"] = [s for s in r2["settings"] if s[0] in ("born", "inold", "incells")]
        if r2["kind"] == "sheet":
            r2["arms"] = ["born", "alonej2", "inj2"]
        one(name + "-" + then, r2, dict(opt, rows=None))
    return 0


if __name__ == "__main__":
    sys.exit(main())
