#!/usr/bin/env python3
"""The blob's own output for the one-hop rule (docs/results-2026-11-26-output-inside.md).
See scripts/outputinside.sh. Each table is a fixed recipe; one line runs it and prints it."""

import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
os.environ.setdefault("EVERYEVENT_BUILD", os.environ.get("OUTPUTINSIDE_BUILD", "build"))
import everyevent as ee  # noqa: E402  the sheet's jobs, one run's numbers, the arms before
import hop  # noqa: E402  the toys' life and old readout
import hop2  # noqa: E402  running one job, the toy tables
import inside  # noqa: E402  the engine's readout as scripts/mnist.sh has it

EV = ee.EV  # the one-hop rule on 12-bit planes, every marked input steps
F4 = " --outside-form 4 --outside-margin %d"  # (the harness reads a flag's first value)
GATE = " --g.margin_percent 20 --g.margin_win 2"


def rung(flags, margin=2):
    """a life whose error lines come from the ladder's outside output (form 4)"""
    return ee.outside(EV + F4 % margin + " " + flags)


def engine(flags):
    """a judged life: the engine's own readout learns and its votes make the errors by
    margin (hop_teach 5, a quarter)"""
    return ee.judged(5, EV + " --g.hop_fine 8 " + flags)


def loop(flags=""):
    """the closed loop, no outside part: the blob's own output answers, learns from the
    right line, and its steps are the one-hop rule's errors (hop_teach 6)"""
    return "--g.learn_mode 5 --life-teach 6 " + EV + " --g.out_on 1 " + flags


TOPE = "--g.out_top 1 --motor-spread-even 1"
TOY_OUT = " --g.hop_teach 6 --g.out_on 1 --g.out_top 1"  # the toys' life with the new output
COIN_OUT = "--life-outside-coin 1"
COIN_IN = "--g.hop_coin 3"
ARMS = {
    # the blob's own output alone (the one-hop rule off), then the closed loop
    "alone": ("--g.learn_mode 5 --life-teach 6 --g.out_on 1", None),
    "in": (loop(), COIN_IN),
    "inm1": (loop("--g.out_margin 1"), COIN_IN),
    "inm4": (loop("--g.out_margin 4"), COIN_IN),
    "inm8": (loop("--g.out_margin 8"), COIN_IN),
    "inf1": (loop("--g.out_fine 1"), COIN_IN),
    "inf4": (loop("--g.out_fine 4"), COIN_IN),
    "inf8": (loop("--g.out_fine 8"), COIN_IN),
    "inw5": (loop("--g.out_window 5"), COIN_IN),
    "inb8": (loop("--g.out_bits 8"), COIN_IN),
    "in64": (loop("--g.hop_rewire_every 64"), COIN_IN),
    # what the ladder found: no sensor cell counted; only the top wrong line steps; 16 bits
    "aloneh": ("--g.learn_mode 5 --life-teach 6 --g.out_on 1 --g.out_hidden 1", None),
    "alonehb16": ("--g.learn_mode 5 --life-teach 6 --g.out_on 1 --g.out_hidden 1 "
                  "--g.out_bits 16", None),
    "inh": (loop("--g.out_hidden 1"), COIN_IN),
    "inb16": (loop("--g.out_bits 16"), COIN_IN),
    "inhb16": (loop("--g.out_hidden 1 --g.out_bits 16"), COIN_IN),
    "intop": (loop("--g.out_top 1"), COIN_IN),
    "intop2": (loop("--g.out_top 2"), COIN_IN),
    "inhtop": (loop("--g.out_hidden 1 --g.out_top 1"), COIN_IN),
    "inhtop2": (loop("--g.out_hidden 1 --g.out_top 2"), COIN_IN),
    # knobs on the closed loop with only the top wrong line stepping
    "alonetop": ("--g.learn_mode 5 --life-teach 6 --g.out_on 1 --g.out_top 1", None),
    "intopm1": (loop("--g.out_top 1 --g.out_margin 1"), COIN_IN),
    "intopm4": (loop("--g.out_top 1 --g.out_margin 4"), COIN_IN),
    "intopm8": (loop("--g.out_top 1 --g.out_margin 8"), COIN_IN),
    "intopf1": (loop("--g.out_top 1 --g.out_fine 1"), COIN_IN),
    "intopf4": (loop("--g.out_top 1 --g.out_fine 4"), COIN_IN),
    "intopf8": (loop("--g.out_top 1 --g.out_fine 8"), COIN_IN),
    "intopw5": (loop("--g.out_top 1 --g.out_window 5"), COIN_IN),
    "intopb8": (loop("--g.out_top 1 --g.out_bits 8"), COIN_IN),
    "intopb16": (loop("--g.out_top 1 --g.out_bits 16"), COIN_IN),
    "intop64": (loop("--g.out_top 1 --g.hop_rewire_every 64"), COIN_IN),
    "intopall": (loop("--g.out_top 1 --g.hop_who 3"), COIN_IN),
    "paytop": (loop("--g.out_top 1 --g.out_pay 1"), COIN_IN),
    "paytop2": (loop("--g.out_top 2 --g.out_pay 1"), COIN_IN),
    # the pair test has two buttons, and --motor-spread 102 gives them the 204 motor cells
    # nearest the top edge: the same count spread evenly over the sheet, or all 512 a button
    "alonee": ("--g.learn_mode 5 --life-teach 6 --g.out_on 1 --g.out_top 1 "
               "--motor-spread-even 1", None),
    "intope": (loop("--g.out_top 1 --motor-spread-even 1"), COIN_IN),
    "inhtope": (loop("--g.out_top 1 --g.out_hidden 1 --motor-spread-even 1"), COIN_IN),
    "intop512": (loop("--g.out_top 1 --motor-spread 512"), COIN_IN),
    # knobs on that: the closed loop, top wrong line only, the buttons' cells spread evenly
    "intopem1": (loop(TOPE + " --g.out_margin 1"), COIN_IN),
    "intopem4": (loop(TOPE + " --g.out_margin 4"), COIN_IN),
    "intopem8": (loop(TOPE + " --g.out_margin 8"), COIN_IN),
    "intopef1": (loop(TOPE + " --g.out_fine 1"), COIN_IN),
    "intopef4": (loop(TOPE + " --g.out_fine 4"), COIN_IN),
    "intopew5": (loop(TOPE + " --g.out_window 5"), COIN_IN),
    "intopew10": (loop(TOPE + " --g.out_window 10"), COIN_IN),
    "intopeb8": (loop(TOPE + " --g.out_bits 8"), COIN_IN),
    "intopeb16": (loop(TOPE + " --g.out_bits 16"), COIN_IN),
    "intope64": (loop(TOPE + " --g.hop_rewire_every 64"), COIN_IN),
    "intopeall": (loop(TOPE + " --g.hop_who 3"), COIN_IN),
    "paytope": (loop(TOPE + " --g.out_pay 1"), COIN_IN),
    # what the knobs liked, together: a 10-tick window, inhibitory and pacemaker learners
    "intopeallw10": (loop(TOPE + " --g.hop_who 3 --g.out_window 10"), COIN_IN),
    "paytopeall": (loop(TOPE + " --g.out_pay 1 --g.hop_who 3"), COIN_IN),
    "paytopew10": (loop(TOPE + " --g.out_pay 1 --g.out_window 10"), COIN_IN),
    "paytopeallw10": (loop(TOPE + " --g.out_pay 1 --g.hop_who 3 --g.out_window 10"), COIN_IN),
    # the errors back along the output's own sources instead of the hashed wires
    "pay": (loop("--g.out_pay 1"), COIN_IN),
    # the ladder, outside: one difference a rung from the whole-number output (outint2)
    "fld": (rung("--outside-read 1"), COIN_OUT),
    "fldbit": (rung("--outside-read 1 --outside-act 1"), COIN_OUT),
    "fld12": (rung("--outside-read 1 --outside-act 1 --outside-bits 12"), COIN_OUT),
    "fld8": (rung("--outside-read 1 --outside-act 1 --outside-bits 8"), COIN_OUT),
    "fld8s3": (rung("--outside-read 1 --outside-act 1 --outside-bits 8 --outside-step-shift 3"),
               COIN_OUT),
    "fld8s6": (rung("--outside-read 1 --outside-bits 8 --outside-step-shift 6"), COIN_OUT),
    "fld6": (rung("--outside-read 1 --outside-act 1 --outside-bits 6"), COIN_OUT),
    # the same read with no sensor cell among the sources; only the top wrong line steps
    "fldh": (rung("--outside-read 2"), COIN_OUT),
    "fldhbit": (rung("--outside-read 2 --outside-act 1"), COIN_OUT),
    "fldh12": (rung("--outside-read 2 --outside-act 1 --outside-bits 12"), COIN_OUT),
    "outtop": (rung("--outside-top 1"), COIN_OUT),
    "fldtop": (rung("--outside-read 1 --outside-act 1 --outside-bits 12 --outside-top 1"),
               COIN_OUT),
    "fldhtop": (rung("--outside-read 2 --outside-act 1 --outside-bits 12 --outside-top 1"),
                COIN_OUT),
    # with only the top wrong line stepping: narrower counters, cells that vote one bit,
    # one-bit wiring at the born count
    "fld8top": (rung("--outside-read 1 --outside-act 1 --outside-bits 8 --outside-top 1"),
                COIN_OUT),
    "fld6top": (rung("--outside-read 1 --outside-act 1 --outside-bits 6 --outside-top 1"),
                COIN_OUT),
    "fld8s3top": (rung("--outside-read 1 --outside-act 1 --outside-bits 8 --outside-top 1 "
                       "--outside-step-shift 3"), COIN_OUT),
    "comtop": (rung("--outside-read 1 --outside-act 1 --outside-bits 12 --outside-vote 1 "
                    "--outside-top 1"), COIN_OUT),
    "comregtop": (rung("--outside-read 1 --outside-act 1 --outside-bits 12 --outside-vote 3 "
                       "--outside-top 1"), COIN_OUT),
    "bit1top": (rung("--outside-read 1 --outside-act 1 --outside-bits 12 --outside-vote 2 "
                     "--outside-zone-32 8 --outside-top 1"), COIN_OUT),
    "outtopm1": (rung("--outside-top 1", 1), COIN_OUT),
    "outtopm8": (rung("--outside-top 1", 8), COIN_OUT),
    # the engine's rules in whole numbers on the same read (told good or bad only)
    "fldr1": (rung("--outside-read 1 --outside-bits 20 --outside-rule 1"), COIN_OUT),
    "fldr2": (rung("--outside-read 1 --outside-bits 20 --outside-rule 2"), COIN_OUT),
    "fldr3": (rung("--outside-read 1 --outside-bits 20 --outside-rule 3"), COIN_OUT),
    # cells that vote one bit each; cells with one-bit wiring at their born count
    "com": (rung("--outside-read 1 --outside-act 1 --outside-bits 12 --outside-vote 1"), COIN_OUT),
    "comreg": (rung("--outside-read 1 --outside-act 1 --outside-bits 12 --outside-vote 3"),
               COIN_OUT),
    "comregz8": (rung("--outside-read 1 --outside-act 1 --outside-bits 12 --outside-vote 3 "
                      "--outside-zone-32 8"), COIN_OUT),
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
    # the engine's motor cells as they are, told the right line: the margin perceptron's rule
    "engans": (engine("--g.readout_good 3"), "--g.hop_coin 3"),
    "engansstr": (engine("--g.readout_good 3 --g.readout_evidence_strong 2"), "--g.hop_coin 3"),
    "engans4": (engine("--g.readout_good 3 --g.readout_shift 4"), "--g.hop_coin 3"),
    "engansf4": (ee.judged(5, EV + " --g.hop_fine 4 --g.readout_good 3"), "--g.hop_coin 3"),
    "enganstop": (engine("--g.readout_good 4"), "--g.hop_coin 3"),
    "enganstop4": (engine("--g.readout_good 4 --g.readout_shift 4"), "--g.hop_coin 3"),
    "enganstope": (engine("--g.readout_good 4 --motor-spread-even 1"), "--g.hop_coin 3"),
    "enganstopf2": (ee.judged(5, EV + " --g.hop_fine 2 --g.readout_good 4"), "--g.hop_coin 3"),
    "enganstopf4": (ee.judged(5, EV + " --g.hop_fine 4 --g.readout_good 4"), "--g.hop_coin 3"),
    "enganstopf4s4": (ee.judged(5, EV + " --g.hop_fine 4 --g.readout_good 4 --g.readout_shift 4"),
                      "--g.hop_coin 3"),
    # the runner-up rule's votes with a thinner zone, and as "told the right line on bad steps"
    "engrunf2": (ee.judged(5, EV + " --g.hop_fine 2 --g.readout_good 2" + GATE), "--g.hop_coin 3"),
    "engrun2": (ee.judged(2, EV + " --g.readout_good 2" + GATE), "--g.hop_coin 3"),
}
ee.ARMS.update(ARMS)

RECIPES = {
    # the ladder on the validation slice, seeds 1 and 2
    "ladder": {"kind": "sheet", "val": True, "seeds": [1, 2], "arms": [
        "born", "outint2", "fld", "fldbit", "fld12", "fld8", "fld8s3", "fld8s6", "fldr1",
        "fldr2", "fldr3", "com", "comz8", "bit1", "bit1c"], "coins": [], "shuffled": []},
    "ladderengine": {"kind": "sheet", "val": True, "seeds": [1, 2], "folder": "ladder", "arms": [
        "eng", "enggate", "engrun", "engrunstr"], "coins": [], "shuffled": []},
    "ladderengine2": {"kind": "sheet", "val": True, "seeds": [1, 2], "folder": "ladder", "arms": [
        "enganstop", "engans", "engrun2", "enganstop4", "engrunf2", "engansf4"], "coins": [],
        "shuffled": []},
    "ladderengine3": {"kind": "sheet", "val": True, "seeds": [1, 2], "folder": "ladder", "arms": [
        "enganstopf4", "enganstopf2", "enganstopf4s4"], "coins": [], "shuffled": []},
    "ladder2": {"kind": "sheet", "val": True, "seeds": [1, 2], "folder": "ladder", "arms": [
        "fldh", "fldhbit", "fldh12", "outtop", "fldtop", "fldhtop", "comreg", "comregz8"],
        "coins": [], "shuffled": []},
    "ladder3": {"kind": "sheet", "val": True, "seeds": [1, 2], "folder": "ladder", "arms": [
        "fld8top", "fld6top", "fld8s3top", "comtop", "comregtop", "bit1top", "outtopm1",
        "outtopm8"], "coins": [], "shuffled": [], "tests": ["mnist"]},
    # the blob's own output, validation, seeds 1 and 2: alone, the closed loop, its knobs
    "loop": {"kind": "sheet", "val": True, "seeds": [1, 2], "arms": [
        "born", "alone", "aloneh", "alonehb16", "in", "inh", "inb16", "inhb16", "intop",
        "intop2", "inhtop", "inhtop2"], "coins": [], "shuffled": []},
    "looppair": {"kind": "sheet", "val": True, "seeds": [1, 2], "folder": "loop", "arms": [
        "alonee", "intope", "inhtope", "intop512"], "coins": [], "shuffled": [],
        "tests": ["pair"]},
    "loopknobs": {"kind": "sheet", "val": True, "seeds": [1, 2], "folder": "loop", "arms": [
        "alonee", "intope", "paytope", "intopem1", "intopem4", "intopem8", "intopef1",
        "intopef4", "intopew5", "intopew10", "intopeb8", "intopeb16", "intope64", "intopeall"],
        "coins": ["intope", "paytope"], "shuffled": []},
    "loopcombo": {"kind": "sheet", "val": True, "seeds": [1, 2], "folder": "loop", "arms": [
        "intopeallw10", "paytopeall", "paytopew10", "paytopeallw10"], "coins": [],
        "shuffled": []},
    "looppay": {"kind": "sheet", "val": True, "seeds": [1, 2], "folder": "loop", "arms": [
        "paytop", "paytop2", "pay"], "coins": ["paytop"], "shuffled": []},
    # the report: test digits, seeds 5-12. the closed loop beside born, its coin, a
    # shuffled-digit life, the output alone on a sheet that does not learn, and the same
    # rule as an outside output
    "report": {"kind": "sheet", "val": False, "seeds": list(range(5, 13)), "arms": [
        "born", "intope", "paytope", "alonee", "outtop"], "coins": ["intope", "paytope"],
        "shuffled": ["intope"]},
    # picked on validation after the first report: a 10-tick window and inhibitory and
    # pacemaker learners too, with the hashed wires and with payment
    "report2": {"kind": "sheet", "val": False, "seeds": list(range(5, 13)), "folder": "report",
                "arms": ["intopeallw10", "paytopeallw10"],
                "coins": ["intopeallw10"], "shuffled": []},
    # the report's main arm again, after "a tie goes to the lowest line" went in (the
    # report ran before it): the same eight lives a test, to compare line for line
    "recheck": {"kind": "sheet", "val": False, "seeds": list(range(5, 13)), "arms": ["intope"],
                "coins": [], "shuffled": []},
    # the smaller change: the motor cells as they are under the same rule
    "reportcells": {"kind": "sheet", "val": False, "seeds": list(range(5, 13)),
                    "folder": "report", "arms": ["enganstope"], "coins": ["enganstope"],
                    "shuffled": []},
    # the rate held at target (threshold regulation uncapped), MNIST, seeds 5-8
    "cap": {"kind": "sheet", "val": False, "seeds": [5, 6, 7, 8], "extra": ee.CAP,
            "arms": ["born", "intope"], "coins": ["intope"], "shuffled": [], "tests": ["mnist"]},
    # the toys: a life with the blob's own output teaching the cells, then a fresh output
    # on the frozen sheet (the old readout, as the every-event doc scored them; "own": a
    # fresh output of the new kind instead)
    "toys": {"kind": "toy", "tasks": ["xor", "parity3"], "seeds": list(range(5, 13)), "settings": [
        ("born", None, ""), ("old", EV + " --g.hop_teach 2", ""),
        ("in", EV + TOY_OUT, ""), ("incoin", EV + TOY_OUT + " --g.hop_coin 3", ""),
        ("bornown", None, "--g.out_on 1 --g.out_top 1"),
        ("inown", EV + TOY_OUT, "--g.out_on 1 --g.out_top 1")]},
    # two rows of the quick map that lean on the output (scripts/oneset.sh), seeds 5-12
    "map": {"kind": "map", "rows": "echo16,mnist", "columns": [
        ("base", ""),
        ("own", "--g.learn_mode 5 --motor-spread 128 --g.out_on 1 --g.out_top 1"),
        ("ownloop", "--g.learn_mode 5 --motor-spread 128 --g.out_on 1 --g.out_top 1 " + EV +
         " --g.hop_teach 6")]},
    # ticks a second of an MNIST life, one at a time (--jobs 1)
    "speed": {"kind": "sheet", "val": False, "seeds": [5], "arms": [
        "eng", "engrun", "enganstop", "outint2", "alonee", "intope", "paytope"], "coins": [],
        "shuffled": [],
        "tests": ["mnist"], "life": (20000, 2000)},
    "smoke": {"kind": "sheet", "val": True, "seeds": [1], "arms": [
        "born", "outint2", "fldtop", "bit1top", "fldr3", "enganstop", "alonee", "intope",
        "paytope"], "coins": ["intope"], "shuffled": ["intope"], "then": ["toys", "map"]},
}


def teach_share(path):
    """the share of the last tenth's digits on which any error was sent"""
    text = open(path).read()
    v = [int(x) for x in re.findall(r"hop: verdicts (\d+)", text)]
    n = [int(x) for x in re.findall(r"^life (\d+) \([0-9.]+ t/s\)\s+readout right", text, re.M)]
    if len(v) < 2 or len(n) < 2 or n[-1] == n[-2]:
        return None
    return (v[-1] - v[-2]) / (n[-1] - n[-2])


def own_frozen(path):
    """the blob's own answer on the scored digits after the life, frozen"""
    m = re.search(r"own answer on the \w+ digits, frozen: ([0-9.]+)", open(path).read())
    return float(m.group(1)) if m else None


def table(r, outdir, name):
    seeds = r["seeds"]
    fmt, mean = hop2.fmt, hop2.mean
    lines = []
    for test in r.get("tests", list(ee.TESTS)):
        lines += ["", "%s, %s: a fresh linear reader of the non-sensor cells (%s) after the life"
                  % (name, test, "validation" if r["val"] else "test digits"),
                  "%-16s %s  %-7s %-5s %-6s %-6s %-6s %-6s %-5s %-6s" % (
                      "arm", " ".join("%-6d" % s for s in seeds), "mean", "rate", "pixels",
                      "own", "ownT", "outsd", "teach", "t/s")]
        got = {}
        for run, _ in ee.sheet_runs(r):
            paths = [os.path.join(outdir, "%s-%s-%d.txt" % (test, run, s)) for s in seeds]
            v = [ee.sheet_read(p) for p in paths]
            got[run] = v
            avg = lambda k: mean([x[k] for x in v if x])
            teach = mean([teach_share(p) for p, x in zip(paths, v) if x])
            frozen = mean([own_frozen(p) for p, x in zip(paths, v) if x])
            lines.append("%-16s %s  %s  %s  %s  %s  %s  %s  %s  %s" % (
                run, " ".join(fmt(x["score"]) if x else "  -   " for x in v), fmt(avg("score")),
                fmt(avg("rate"), "%4.1f"), fmt(avg("pixels"), "%4.1f"),
                fmt(avg("readout"), "%5.3f"), fmt(frozen, "%5.3f"), fmt(avg("outside"), "%5.3f"),
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
                 "tenth of the life; ownT: its own answer on the scored digits after the life, "
                 "frozen (it means something only with out_on); outsd: the outside output's; teach: the share of the last "
                 "tenth's digits that sent an error; t/s: the life's ticks a second)")
    return "\n".join(lines)


def toy_jobs(r, outdir, quick):
    """(life flags or None for a born sheet, flags of the fresh output's phase) per setting"""
    jobs = []
    life = hop.LIFE.replace("--steps 40000", "--steps 4000") if quick else hop.LIFE
    for task in r["tasks"]:
        for name, life_flags, then_flags in r["settings"]:
            for s in r["seeds"]:
                text = "%s %s %s %s" % (hop.TASKS[task], life, hop.READOUT, life_flags or "")
                cmd = ["nice", "-n", "19", ee.PROBE, "transfer", "--life", " ".join(text.split()),
                       "--then", hop.TASKS[task].split()[1], "--then-steps",
                       "2000" if quick else "20000", "--seed", str(s)]
                cmd += ["--born", "1"] if life_flags is None else []
                cmd += then_flags.split() + (["--window", "100"] if quick else [])
                jobs.append((cmd, os.path.join(outdir, "toy-%s-%s-%d.txt" % (task, name, s))))
    return jobs


def toy_table(r, outdir):
    seeds = r["seeds"]
    lines = ["a fresh output on the frozen sheet after a 40,000-step life, late score (first "
             "quarter); life: the blob's own answer late in the life; seeds %s" %
             ",".join(map(str, seeds))]
    for task in r["tasks"]:
        lines.append("%-8s %-9s %s   mean (first quarter)  life" % (
            task, "setting", " ".join("%-6d" % s for s in seeds)))
        for name, _, _ in r["settings"]:
            v = [hop.toy_score(os.path.join(outdir, "toy-%s-%s-%d.txt" % (task, name, s)))
                 for s in seeds]
            got = [x for x in v if x]
            cells = " ".join("%.3f " % x[1] if x else "  -   " for x in v)
            avg = ("%.3f (%.3f)" % (hop2.mean([x[1] for x in got]), hop2.mean([x[0] for x in got]))
                   if got else "-")
            life = hop2.mean([x[2] for x in got])
            lines.append("%-8s %-9s %s   %s  %s" % ("", name, cells, avg, hop2.fmt(life, "%.3f")))
    return "\n".join(lines)


def map_rows(r, outdir, opt):
    """scripts/oneset.py for each column on the recipe's rows; its tables kept as text"""
    seeds = "5" if opt["quick"] else "5-12"
    text = []
    for name, flags in r["columns"]:
        cmd = ["python3", os.path.join(ROOT, "scripts", "oneset.py"), "run", name, "--rows",
               r["rows"], "--seeds", seeds, "--jobs", str(opt["jobs"]), "--blobrun",
               os.path.join(ee.BUILD, "blobrun")]
        cmd += ["--"] + flags.split() if flags else []
        done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                              cwd=ROOT)
        text.append("column %s: %s\n%s" % (name, flags or "(scripts/variants/oneset.txt)",
                                           done.stdout))
    return "\n".join(text)


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
        jobs, show = toy_jobs(r, outdir, quick), lambda: toy_table(r, outdir)
    else:
        jobs, show = [], lambda: map_rows(r, outdir, opt)
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
