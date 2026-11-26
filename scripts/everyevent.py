#!/usr/bin/env python3
"""Every-event counters in the engine's one-hop rule (docs/results-2026-11-26-every-event.md).
See scripts/everyevent.sh. Each table is a fixed recipe; one line runs it and prints it."""

import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import inside  # noqa: E402  the MNIST sheet, its reader and the taught life's readout
import hop  # noqa: E402  the toys' life and readout, the calc members
import hop2  # noqa: E402  reading one run, the toy and calc tables

BUILD = os.path.join(ROOT, os.environ.get("EVERYEVENT_BUILD", "build"))
MNIST = os.path.join(BUILD, "blobmnist")
PROBE = os.path.join(BUILD, "blobprobe")
TESTS = {"mnist": "", "pair": "--pair 2 --pair-mix 1"}

# the pieces. EV: counters as 12-bit planes, every marked input steps (no chance)
HOP = "--g.hop_on 1"
EV = HOP + " --g.hop_bits 12 --g.hop_shift 0"
ALL = " --g.hop_who 7"  # inhibitory, pacemaker and motor cells learn too
INH = " --g.hop_who 3"  # inhibitory and pacemaker cells learn too (the motor cells read out)


def judged(teach, flags):
    """a life judged as blobmnist online judges it: the sheet's readout learns, the rule
    hears the buttons (hop_teach = teach)"""
    return " ".join(inside.RULE) + " --life-teach %d %s" % (teach, flags)


def outside(flags):
    """a life whose error lines come from the outside softmax output (mnist_outside.cpp);
    not judged, so the sheet's readout does not learn"""
    return "--g.learn_mode 5 --life-outside 1 " + flags


# arm -> (life flags or None for the born sheet, its coin's extra flags or None)
ARMS = {
    "born": (None, None),
    "off": (judged(3, ""), None),  # the readout alone in the life, the rule off
    # the engine's rule as it was: from the answer, chance 2^-9, 6-bit counters
    "old": (judged(3, HOP + " --g.hop_shift 9"), "--g.hop_coin 1"),
    # piece 1 alone: every event, from the answer, the engine's learners
    "ev3": (judged(3, EV), "--g.hop_coin 1"),
    "ev3b6": (judged(3, HOP + " --g.hop_shift 0"), "--g.hop_coin 1"),  # in the 6-bit bytes
    "ev3b8": (judged(3, HOP + " --g.hop_shift 0 --g.tag_bits 8"), "--g.hop_coin 1"),
    "ev3p9": (judged(3, HOP + " --g.hop_bits 9 --g.hop_shift 0"), "--g.hop_coin 1"),
    # the same with each cell taught "more" and "less" equally often (hop_balance)
    "ev3bal": (judged(3, EV + " --g.hop_balance 1"), "--g.hop_coin 1"),
    "ev3bal8": (judged(3, EV + " --g.hop_balance 8"), "--g.hop_coin 1"),
    "ev3ibal": (judged(3, EV + INH + " --g.hop_balance 1"), "--g.hop_coin 1"),
    "ev3allbal": (judged(3, EV + ALL + " --life-reborn 0 --g.hop_balance 1"), "--g.hop_coin 1"),
    "ev4bal": (judged(4, EV + " --g.hop_balance 1"), "--g.hop_coin 3"),
    "ev5bal": (judged(5, EV + " --g.hop_fine 8 --g.hop_balance 1"), "--g.hop_coin 3"),
    "oldbal": (judged(3, HOP + " --g.hop_shift 9 --g.hop_balance 1"), "--g.hop_coin 1"),
    "outallbal": (outside(EV + ALL + " --g.hop_balance 1"), "--life-outside-coin 1"),
    # told the right button on bad steps (hop_teach 2), every event; other margins
    "ev2": (judged(2, EV), "--g.hop_coin 3"),
    "ev5f4": (judged(5, EV + " --g.hop_fine 4"), "--g.hop_coin 3"),
    "ev5f16": (judged(5, EV + " --g.hop_fine 16"), "--g.hop_coin 3"),
    # what of the outside output's error matters: only when it teaches (form 1: the answer
    # on the digits it is unsure of), or nothing of it (form 2: the answer as signs always)
    "outwhen": (outside(EV + " --outside-form 1"), "--life-outside-coin 1"),
    "outsigns": (outside(EV + " --outside-form 2"), "--life-outside-coin 1"),
    # the softmax output with a wider fine zone (the ladder's was 0.1, not tuned)
    "outf3": (outside(EV + " --outside-fine-1000 300"), "--life-outside-coin 1"),
    "outf5": (outside(EV + " --outside-fine-1000 500"), "--life-outside-coin 1"),
    # the outside output in whole numbers (a perceptron with a margin, no floats)
    "outint": (outside(EV + " --outside-form 3"), "--life-outside-coin 1"),
    "outint0": (outside(EV + " --outside-form 3 --outside-margin 0"), "--life-outside-coin 1"),
    "outint1": (outside(EV + " --outside-form 3 --outside-margin 1"), "--life-outside-coin 1"),
    "outint2": (outside(EV + " --outside-form 3 --outside-margin 2"), "--life-outside-coin 1"),
    "outint32": (outside(EV + " --outside-form 3 --outside-margin 32"), "--life-outside-coin 1"),
    # piece 3 on it: who learns
    "ev3i": (judged(3, EV + INH), "--g.hop_coin 1"),
    "ev3all": (judged(3, EV + ALL + " --life-reborn 0"), "--g.hop_coin 1"),
    # piece 2: the sheet's own output's signed error per line, by share and by margin
    "ev4": (judged(4, EV), "--g.hop_coin 3"),
    "ev5": (judged(5, EV + " --g.hop_fine 8"), "--g.hop_coin 3"),
    "ev4i": (judged(4, EV + INH), "--g.hop_coin 3"),
    "ev5i": (judged(5, EV + " --g.hop_fine 8" + INH), "--g.hop_coin 3"),
    # piece 2 with the model's output: an outside softmax layer's signed error per line
    "out": (outside(EV), "--life-outside-coin 1"),
    "outi": (outside(EV + INH), "--life-outside-coin 1"),
    "outall": (outside(EV + ALL), "--life-outside-coin 1"),
    # the outside output's errors in a judged life, so the sheet's own readout learns beside
    "outj": (" ".join(inside.RULE) + " --life-teach 3 --life-outside 1 " + EV,
             "--life-outside-coin 1"),
    "outintj": (" ".join(inside.RULE) + " --life-teach 3 --life-outside 1 " + EV +
                " --outside-form 3 --outside-margin 2", "--life-outside-coin 1"),
    # the same errors and learners on the old counters (6-bit, chance 2^-3): the ladder's T7
    "out6all": (outside(HOP + " --g.hop_shift 3" + ALL), "--life-outside-coin 1"),
    # piece 4: the model's other differences, on out and on outall
    "out64": (outside(EV + " --g.hop_rewire_every 64"), "--life-outside-coin 1"),
    "outloose": (outside(EV + " --g.hop_loose 1"), "--life-outside-coin 1"),
    "outboth": (outside(EV + " --g.hop_rewire_every 64 --g.hop_loose 1"), "--life-outside-coin 1"),
    "out9": (outside(HOP + " --g.hop_bits 9 --g.hop_shift 0"), "--life-outside-coin 1"),
    "out16": (outside(HOP + " --g.hop_bits 16 --g.hop_shift 0"), "--life-outside-coin 1"),
    "outall64": (outside(EV + ALL + " --g.hop_rewire_every 64"), "--life-outside-coin 1"),
    "outallloose": (outside(EV + ALL + " --g.hop_loose 1"), "--life-outside-coin 1"),
    "outallboth": (outside(EV + ALL + " --g.hop_rewire_every 64 --g.hop_loose 1"),
                   "--life-outside-coin 1"),
    "outall9": (outside(HOP + " --g.hop_bits 9 --g.hop_shift 0" + ALL), "--life-outside-coin 1"),
}
CAP = "--g.threshold_cap 1000"  # the rate held at target (hop-real doc, targetcap)

# name -> recipe. kinds: sheet (blobmnist ceiling per test), toy, calc
RECIPES = {
    # every arm once, validation slice, seeds 1 and 2: which pieces matter
    "pick": {"kind": "sheet", "val": True, "seeds": [1, 2], "arms": [
        "born", "old", "ev3", "ev3i", "ev3all", "ev4", "ev5", "out", "outi", "outall",
        "out6all", "ev3b8"], "coins": ["old", "ev3", "ev5", "outall"], "shuffled": []},
    # after pick: why the answer alone and the sheet's own votes fail with every-event steps
    "pick2": {"kind": "sheet", "val": True, "seeds": [1, 2], "arms": [
        "ev3bal", "ev5bal", "ev2", "ev5f4", "ev5f16", "outwhen", "outsigns"],
        "coins": [], "shuffled": []},
    "pick3": {"kind": "sheet", "val": True, "seeds": [1, 2], "arms": [
        "outint", "outint2", "outint32", "out16"], "coins": [], "shuffled": []},
    "pick5": {"kind": "sheet", "val": True, "seeds": [1, 2], "arms": [
        "outint0", "outint1"], "coins": [], "shuffled": []},
    "pick6": {"kind": "sheet", "val": True, "seeds": [1, 2], "arms": ["outf3", "outf5"],
              "coins": [], "shuffled": []},
    # piece 4 on the validation slice (only if pieces 1 to 3 leave a gap)
    "pick4": {"kind": "sheet", "val": True, "seeds": [1, 2], "arms": [
        "out64", "outloose", "outboth", "out9", "outallboth"], "coins": [], "shuffled": []},
    # the report: test digits, seeds 5-12, the key arms beside born, coin and shuffled lives
    "report": {"kind": "sheet", "val": False, "seeds": list(range(5, 13)), "arms": [
        "born", "out", "outall", "outint2"], "coins": ["out", "outall", "outint2"],
        "shuffled": ["out", "outall", "outint2"]},
    # the pieces beside them, seeds 5-8 (same folder as report: one table)
    "pieces": {"kind": "sheet", "val": False, "seeds": [5, 6, 7, 8], "folder": "report", "arms": [
        "born", "old", "ev3", "ev2", "ev5", "out6all", "outi", "out", "outall", "outint2"],
        "coins": ["ev3", "ev5", "out", "outall", "outint2"], "shuffled": []},
    # the rate held at target, MNIST, seeds 5-8
    "cap": {"kind": "sheet", "val": False, "seeds": [5, 6, 7, 8], "extra": CAP,
            "arms": ["born", "out", "outall"], "coins": ["out", "outall"], "shuffled": [],
            "tests": ["mnist"]},
    # the sheet's own readout during a judged MNIST life, seeds 5-8: the rule off, the old
    # rule, every-event from the answer and from the votes, and the outside errors
    "online": {"kind": "sheet", "val": False, "seeds": [5, 6, 7, 8], "folder": "report",
               "arms": ["off", "old", "ev3", "ev2", "ev5", "outj", "outintj"], "coins": [],
               "shuffled": []},
    # the life's speed, one run at a time (--jobs 1): judged MNIST lives and unjudged ones
    # (20,000-digit lives and a short read: only the t/s column means anything)
    "speed": {"kind": "sheet", "val": False, "seeds": [5], "arms": [
        "off", "old", "ev3", "ev3b6", "outj", "out6all", "out", "out64", "outall", "outint2"],
        "coins": [], "shuffled": [], "tests": ["mnist"], "life": (20000, 2000)},
    # the toys the old rule taught: told the answer on bad steps (hop_teach 2), old against
    # every-event counters
    "toys": {"kind": "toy", "tasks": ["xor", "parity3"], "seeds": list(range(5, 13)), "settings": [
        ("born", None, ""), ("m2", HOP + " --g.hop_teach 2", ""),
        ("ev", EV + " --g.hop_teach 2", ""),
        ("evcoin", EV + " --g.hop_teach 2 --g.hop_coin 1", ""),
        ("evshuffled", EV + " --g.hop_teach 2", "--life-scramble 1")]},
    "calc": {"kind": "calc", "members": ["xor4", "add4"], "seeds": [5, 6], "settings": [
        ("born", None, ""), ("m2", HOP, ""), ("ev", EV, ""),
        ("evcoin", EV + " --g.hop_coin 1", ""), ("evshuffled", EV, "--life-scramble 1")]},
    # one seed, short lives, every part
    "smoke": {"kind": "sheet", "val": True, "seeds": [1], "arms": [
        "born", "old", "ev3", "ev3all", "ev5", "outall"], "coins": ["ev5", "outall"],
        "shuffled": ["outall"], "then": ["toys", "calc"]},
}


def sheet_runs(r):
    """(run name, arm, life flags or None) for every arm, coin and shuffled life"""
    runs = []
    for arm in r["arms"]:
        flags, coin = ARMS[arm]
        runs.append((arm, flags))
        if arm in r["coins"]:
            runs.append((arm + "-coin", flags + " " + coin))
        if arm in r["shuffled"]:
            runs.append((arm + "-shuffled", flags + " --life-shuffle 1"))
    return runs


def sheet_jobs(r, outdir, quick):
    jobs = []
    life, fit = (3000, 5000) if quick else r.get("life", (60000, 60000))
    short = quick or "life" in r
    for test in r.get("tests", list(TESTS)):
        for name, flags in sheet_runs(r):
            for s in r["seeds"]:
                cmd = ["nice", "-n", "19", MNIST, "ceiling"] + inside.SHEET + inside.READER + [
                    "--fit", str(fit), "--seed", str(s)]
                if flags is not None:
                    cmd += ["--life", str(life)] + flags.split()
                cmd += TESTS[test].split() + r.get("extra", "").split()
                cmd += ["--val", "1"] if r["val"] else []
                cmd += ["--test-count", "2000"] if short else []
                jobs.append((cmd, os.path.join(outdir, "%s-%s-%d.txt" % (test, name, s))))
    return jobs


def sheet_read(path):
    """one run: score, rate while read, wiring, the life's own outputs, counter steps, speed"""
    d = hop2.mnist_read(path)
    if not d:
        return None
    text = open(path).read()
    out = re.findall(r"outside output right ([0-9.]+)", text)
    steps = re.findall(r"counter steps (\d+), rewires (\d+)", text)
    speed = [float(x) for x in re.findall(r"^life \d+ \(([0-9.]+) t/s\)\s+hidden \d", text, re.M)]
    d["outside"] = float(out[-1]) if out else None
    d["steps"] = int(steps[-1][0]) / 1e6 if steps else None
    d["rewires"] = int(steps[-1][1]) / 1e6 if steps else None
    d["speed"] = sum(speed) / len(speed) if speed else None
    return d


def sheet_table(r, outdir, name):
    seeds = r["seeds"]
    fmt, mean = hop2.fmt, hop2.mean
    lines = []
    for test in r.get("tests", list(TESTS)):
        lines += ["", "%s, %s: a fresh linear reader of the non-sensor cells (%s), after a life "
                  "of 60,000" % (name, test, "validation" if r["val"] else "test digits"),
                  "%-16s %s  %-7s %-5s %-6s %-5s %-6s %-7s %-6s %-6s %-6s" % (
                      "arm", " ".join("%-6d" % s for s in seeds), "mean", "rate", "pixels",
                      "kept", "life", "outside", "steps", "rewire", "t/s")]
        got = {}
        for run, _ in sheet_runs(r):
            v = [sheet_read(os.path.join(outdir, "%s-%s-%d.txt" % (test, run, s))) for s in seeds]
            got[run] = v
            avg = lambda k: mean([x[k] for x in v if x])
            lines.append("%-16s %s  %s  %s  %s  %s  %s  %s  %s  %s  %s" % (
                run, " ".join(fmt(x["score"]) if x else "  -   " for x in v), fmt(avg("score")),
                fmt(avg("rate"), "%4.1f"), fmt(avg("pixels"), "%4.1f"), fmt(avg("kept"), "%4.2f"),
                fmt(avg("readout"), "%5.3f"), fmt(avg("outside"), "%5.3f"),
                fmt(avg("steps"), "%6.0f"), fmt(avg("rewires"), "%6.1f"),
                fmt(avg("speed"), "%6.0f")))
        for run, _ in sheet_runs(r):
            arm = run.split("-")[0]
            others = ["born", arm + "-coin", arm + "-shuffled", "old"]
            for other in others:
                if run != arm or other == run or run not in got or other not in got or arm == "born":
                    continue
                d = [a["score"] - b["score"] for a, b in zip(got[run], got[other]) if a and b]
                if d:
                    lines.append("%-10s - %-16s %+.4f  (%d of %d seeds above; %+.4f to %+.4f)" % (
                        run, other, sum(d) / len(d), sum(x > 0 for x in d), len(d), min(d), max(d)))
    lines.append("(rate: hidden spikes per 1,000 cell-ticks while read; pixels: pixel inputs per "
                 "hidden excitatory cell; kept: born inputs kept; life: the sheet's own readout's "
                 "share right in the last tenth of the life; outside: the outside output's; "
                 "steps, rewire: counter steps and re-picks that changed wiring, millions; t/s: "
                 "the life's ticks a second)")
    return "\n".join(lines)


def parse(args, r):
    opt = {"quick": False, "table_only": False, "jobs": 3, "reverse": False}
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--quick":
            opt["quick"] = True
        elif a == "--table-only":
            opt["table_only"] = True
        elif a == "--reverse":  # the job list from its end (a second machine on the same list)
            opt["reverse"] = True
        elif a == "--jobs":
            opt["jobs"] = int(args[i + 1])
            i += 1
        elif a == "--seeds":
            r["seeds"] = hop.seeds_of(args[i + 1])
            i += 1
        elif a == "--tests":
            r["tests"] = args[i + 1].split(",")
            i += 1
        elif a == "--arms":
            keep = args[i + 1].split(",")
            if r["kind"] == "sheet":
                r["arms"] = keep
            else:
                r["settings"] = [x for x in r["settings"] if x[0] in keep]
            i += 1
        elif a in ("--coins", "--shuffled"):
            r[a[2:]] = [x for x in args[i + 1].split(",") if x != "none"]
            i += 1
        else:
            print("everyevent: unknown option %s" % a)
            return None
        i += 1
    return opt


def one(name, r, opt):
    quick = opt["quick"]
    if quick and "seeds" in r:
        r["seeds"] = r["seeds"][:1]
    outdir = os.path.join(ROOT, "out", "everyevent",
                          r.get("folder", name) + ("-quick" if quick else ""))
    os.makedirs(outdir, exist_ok=True)
    kind = r["kind"]
    if kind == "sheet":
        jobs, table = sheet_jobs(r, outdir, quick), lambda: sheet_table(r, outdir, name)
    elif kind == "toy":
        jobs, table = hop2.toy_jobs(r, outdir, quick), lambda: hop2.toy_table(r, outdir)
    else:
        jobs, table = hop2.calc_jobs(r, outdir, quick), lambda: hop2.calc_table(r, outdir)
    if not opt["table_only"]:
        jobs = jobs[::-1] if opt["reverse"] else jobs
        with ThreadPoolExecutor(max_workers=opt["jobs"]) as pool:
            list(pool.map(hop2.run, jobs))
    text = table()
    print(text)
    open(os.path.join(outdir, "table%s.txt" % ("-" + name if "folder" in r else "")),
         "w").write(text + "\n")


def main():
    args = sys.argv[1:]
    if not args or args[0] not in RECIPES:
        print(open(os.path.join(ROOT, "scripts", "everyevent.sh")).read().split("set -")[0])
        print("tables: " + " ".join(RECIPES))
        print("arms: " + " ".join(ARMS))
        return 1
    name = args.pop(0)
    r = dict(RECIPES[name])
    opt = parse(args, r)
    if opt is None:
        return 1
    one(name, r, opt)
    for more in r.get("then", []):
        r2 = dict(RECIPES[more])
        r2["seeds"] = r2["seeds"][:1]
        if opt["quick"]:
            one(name + "-" + more, r2, dict(opt))
    return 0


if __name__ == "__main__":
    sys.exit(main())
