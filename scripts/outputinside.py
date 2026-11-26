#!/usr/bin/env python3
"""The blob's own output for the one-hop rule (docs/results-2026-11-26-output-inside.md).
See scripts/outputinside.sh. Each table is a fixed recipe; one line runs it and prints it."""

import os
import re
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
os.environ.setdefault("EVERYEVENT_BUILD", os.environ.get("OUTPUTINSIDE_BUILD", "build"))
import everyevent as ee  # noqa: E402  the sheet's jobs, one run's numbers, the arms before
import hop2  # noqa: E402  running one job, the toy tables
import inside  # noqa: E402  the engine's readout as scripts/mnist.sh has it

EV = ee.EV  # the one-hop rule on 12-bit planes, every marked input steps
F4 = " --outside-form 4 --outside-margin 2"
GATE = " --g.margin_percent 20 --g.margin_win 2"


def rung(flags):
    """a life whose error lines come from the ladder's outside output (form 4)"""
    return ee.outside(EV + F4 + " " + flags)


def engine(flags):
    """a judged life: the engine's own readout learns and its votes make the errors by
    margin (hop_teach 5, a quarter)"""
    return ee.judged(5, EV + " --g.hop_fine 8 " + flags)


COIN_OUT = "--life-outside-coin 1"
ARMS = {
    # the ladder, outside: one difference a rung from the whole-number output (outint2)
    "fld": (rung("--outside-read 1"), COIN_OUT),
    "fldbit": (rung("--outside-read 1 --outside-act 1"), COIN_OUT),
    "fld12": (rung("--outside-read 1 --outside-act 1 --outside-bits 12"), COIN_OUT),
    "fld8": (rung("--outside-read 1 --outside-act 1 --outside-bits 8"), COIN_OUT),
    "fld8s3": (rung("--outside-read 1 --outside-act 1 --outside-bits 8 --outside-step-shift 3"),
               COIN_OUT),
    "fld8s6": (rung("--outside-read 1 --outside-bits 8 --outside-step-shift 6"), COIN_OUT),
    "fld6": (rung("--outside-read 1 --outside-act 1 --outside-bits 6"), COIN_OUT),
    # the engine's rules in whole numbers on the same read (told good or bad only)
    "fldr1": (rung("--outside-read 1 --outside-bits 20 --outside-rule 1"), COIN_OUT),
    "fldr2": (rung("--outside-read 1 --outside-bits 20 --outside-rule 2"), COIN_OUT),
    "fldr3": (rung("--outside-read 1 --outside-bits 20 --outside-rule 3"), COIN_OUT),
    # cells that vote one bit each; cells with one-bit wiring at their born count
    "com": (rung("--outside-read 1 --outside-act 1 --outside-bits 12 --outside-vote 1"), COIN_OUT),
    "comz8": (rung("--outside-read 1 --outside-act 1 --outside-bits 12 --outside-vote 1 "
                   "--outside-zone-32 8"), COIN_OUT),
    "bit1": (rung("--outside-read 1 --outside-act 1 --outside-bits 12 --outside-vote 2 "
                  "--outside-zone-32 8"), COIN_OUT),
    "bit1c": (rung("--outside-read 1 --outside-bits 16 --outside-vote 2 --outside-zone-32 8"),
              COIN_OUT),
    # the ladder's last rungs, in the engine: its own readout under each of its rules
    "eng": (engine(""), "--g.hop_coin 3"),
    "enggate": (engine(GATE), "--g.hop_coin 3"),
    "engrun": (engine("--g.readout_good 2" + GATE), "--g.hop_coin 3"),
    "engrunstr": (engine("--g.readout_good 2 --g.readout_evidence_strong 2" + GATE),
                  "--g.hop_coin 3"),
}
ee.ARMS.update(ARMS)

RECIPES = {
    # the ladder on the validation slice, seeds 1 and 2
    "ladder": {"kind": "sheet", "val": True, "seeds": [1, 2], "arms": [
        "born", "outint2", "fld", "fldbit", "fld12", "fld8", "fld8s3", "fld8s6", "fldr1",
        "fldr2", "fldr3", "com", "comz8", "bit1", "bit1c"], "coins": [], "shuffled": []},
    "ladderengine": {"kind": "sheet", "val": True, "seeds": [1, 2], "folder": "ladder", "arms": [
        "eng", "enggate", "engrun", "engrunstr"], "coins": [], "shuffled": []},
    "smoke": {"kind": "sheet", "val": True, "seeds": [1], "arms": [
        "born", "outint2", "fld12", "com", "bit1", "fldr3", "engrun"], "coins": ["fld12"],
        "shuffled": ["fld12"]},
}


def teach_share(path):
    """the share of the last tenth's digits on which any error was sent"""
    text = open(path).read()
    v = [int(x) for x in re.findall(r"hop: verdicts (\d+)", text)]
    n = [int(x) for x in re.findall(r"^life (\d+) \([0-9.]+ t/s\)\s+readout right", text, re.M)]
    if len(v) < 2 or len(n) < 2 or n[-1] == n[-2]:
        return None
    return (v[-1] - v[-2]) / (n[-1] - n[-2])


def table(r, outdir, name):
    seeds = r["seeds"]
    fmt, mean = hop2.fmt, hop2.mean
    lines = []
    for test in r.get("tests", list(ee.TESTS)):
        lines += ["", "%s, %s: a fresh linear reader of the non-sensor cells (%s) after the life"
                  % (name, test, "validation" if r["val"] else "test digits"),
                  "%-16s %s  %-7s %-5s %-6s %-6s %-6s %-5s %-6s" % (
                      "arm", " ".join("%-6d" % s for s in seeds), "mean", "rate", "pixels",
                      "own", "outsd", "teach", "t/s")]
        got = {}
        for run, _ in ee.sheet_runs(r):
            paths = [os.path.join(outdir, "%s-%s-%d.txt" % (test, run, s)) for s in seeds]
            v = [ee.sheet_read(p) for p in paths]
            got[run] = v
            avg = lambda k: mean([x[k] for x in v if x])
            teach = mean([teach_share(p) for p, x in zip(paths, v) if x])
            lines.append("%-16s %s  %s  %s  %s  %s  %s  %s  %s" % (
                run, " ".join(fmt(x["score"]) if x else "  -   " for x in v), fmt(avg("score")),
                fmt(avg("rate"), "%4.1f"), fmt(avg("pixels"), "%4.1f"),
                fmt(avg("readout"), "%5.3f"), fmt(avg("outside"), "%5.3f"),
                fmt(teach, "%4.2f"), fmt(avg("speed"), "%6.0f")))
        for run, _ in ee.sheet_runs(r):
            arm = run.split("-")[0]
            for other in ["born", arm + "-coin", arm + "-shuffled"]:
                if run != arm or run not in got or other not in got or arm == "born":
                    continue
                d = [a["score"] - b["score"] for a, b in zip(got[run], got[other]) if a and b]
                if d:
                    lines.append("%-10s - %-16s %+.4f  (%d of %d seeds above; %+.4f to %+.4f)" % (
                        run, other, sum(d) / len(d), sum(x > 0 for x in d), len(d), min(d), max(d)))
    lines.append("(rate: hidden spikes per 1,000 cell-ticks while read; pixels: pixel inputs per "
                 "hidden excitatory cell; own: the blob's own answer's share right in the last "
                 "tenth of the life; outsd: the outside output's; teach: the share of the last "
                 "tenth's digits that sent an error; t/s: the life's ticks a second)")
    return "\n".join(lines)


def one(name, r, opt):
    quick = opt["quick"]
    if quick and "seeds" in r:
        r["seeds"] = r["seeds"][:1]
    outdir = os.path.join(ROOT, "out", "outputinside",
                          r.get("folder", name) + ("-quick" if quick else ""))
    os.makedirs(outdir, exist_ok=True)
    kind = r["kind"]
    if kind == "sheet":
        jobs, show = ee.sheet_jobs(r, outdir, quick), lambda: table(r, outdir, name)
    elif kind == "toy":
        jobs, show = hop2.toy_jobs(r, outdir, quick), lambda: hop2.toy_table(r, outdir)
    else:
        jobs, show = hop2.calc_jobs(r, outdir, quick), lambda: hop2.calc_table(r, outdir)
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
        print(open(os.path.join(ROOT, "scripts", "outputinside.sh")).read().split("set -")[0])
        print("tables: " + " ".join(RECIPES))
        print("arms: " + " ".join(ARMS))
        return 1
    name = args.pop(0)
    r = dict(RECIPES[name])
    opt = ee.parse(args, r)
    if opt is None:
        return 1
    one(name, r, opt)
    for more in r.get("then", []):
        r2 = dict(RECIPES[more])
        if opt["quick"]:
            one(name + "-" + more, r2, dict(opt))
    return 0


if __name__ == "__main__":
    sys.exit(main())
