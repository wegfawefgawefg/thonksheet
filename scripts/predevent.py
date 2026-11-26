#!/usr/bin/env python3
"""Idea 183, prediction errors through the every-event one-hop rule. See scripts/predevent.sh.
Reuses the sheet, worlds and helpers of scripts/predhop.py (idea 164)."""

import os
import re
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import predhop as ph  # noqa: E402  the sheet, the worlds, one run, csv reading

OUT = os.path.join(ROOT, "out", "predevent")
# the every-event rule (idea 178): 12-bit plane counters, every marked input steps
EV = "--g.hop_on 1 --g.hop_bits 12 --g.hop_shift 0 --g.hop_teach 0".split()
CAP = "--g.threshold_cap 1000".split()  # the rate held at target (hop-real doc)
# the button rule of "the one setting" (scripts/variants/best.txt) on this battery's sheet
# (its --motor-spread stays 64 so the predictor cells have room). onenw: without the
# backward walk, which re-picks hidden cells from its own counters
ONE = ("--g.sensor_w 64 --g.motor_stride 1 --g.learn_mode 5 --g.readout_evidence 2 "
       "--g.readout_shift 4 --g.readout_balance 1 --g.time_percent 10 --g.hold_percent 10 "
       "--g.readout_good 1 --g.margin_percent 20 --g.margin_win 2 --g.tire_jump 4 "
       "--g.tire_shift 7 --g.tile_winners 2").split()
BASES = {"predhop": ph.GENOME, "onenw": ONE,
         "one": ONE + "--g.replay_depth 1 --g.replay_bad_only 1".split()}
# worlds beyond idea 164's: two rows of the quick map with time in them
WORLDS = dict(ph.WORLDS)
WORLDS.update({"order": [], "gap": [], "delayed2": ["--delay", "2"]})
TASK_OF = {"delayed2": "delayed-echo"}
EV_ARM = re.compile(r"^ev(\d+)f(\d+)(n?)(b?)(c?)(s?)(k?)$")


def arm_flags(arm):
    """an arm's flags, whether it lives, whether its life is the shuffled-frame replay.
    born, pred (the rule off), old (idea 164's rule), then ev<set>f<fine> with letters:
    ev15f8 is every-event counters taught by all four horizons (the set is --predict-hop's
    bits) with a fine zone of 8 32nds; n marks near-not-fired cells only (--g.hop_fired 0),
    b adds the buttons' errors (hop_teach 2). endings for any lived arm: c its coin control,
    s the shuffled-frame life, k the rate held at target (born too: bornk)."""
    if arm in ("born", "bornk"):
        return (CAP if arm == "bornk" else []), False, False
    m = re.match(r"^(pred|old)(c?)(s?)(k?)$", arm)
    if m:
        flags = [] if m.group(1) == "pred" else list(ph.FROM_PRED)
        flags += ["--g.hop_coin", "1"] if m.group(2) else []
        return flags + (CAP if m.group(4) else []), True, bool(m.group(3))
    m = EV_ARM.match(arm)
    if not m:
        sys.exit("predevent: no arm '%s'" % arm)
    flags = EV + ["--predict-hop", m.group(1), "--g.hop_pred_fine", m.group(2)]
    flags += ["--g.hop_fired", "0"] if m.group(3) else []
    flags += ["--g.hop_teach", "2"] if m.group(4) else []
    flags += ["--g.hop_coin", "3"] if m.group(5) else []
    return flags + (CAP if m.group(7) else []), True, bool(m.group(6))


def parse(argv):
    if not argv or argv[0].startswith("-"):
        sys.stderr.write(open(os.path.join(ROOT, "scripts", "predevent.sh")).read()
                         .split("set -")[0])
        sys.exit(1)
    opt = {"name": argv[0], "seeds": [1, 2], "jobs": 3, "steps": 20000, "trials": 3000,
           "worlds": ["catch", "forage"], "arms": ["born", "pred", "old", "ev1f0"],
           "base": "predhop", "table_only": False, "extra": []}
    i = 1
    while i < len(argv):
        a = argv[i]
        val = argv[i + 1] if i + 1 < len(argv) else None
        if a == "--quick":
            opt.update(seeds=[1], steps=1500, trials=500)
        elif a == "--table-only":
            opt["table_only"] = True
        elif a == "--seeds":
            opt["seeds"] = ph.seeds_of(val)
            i += 1
        elif a in ("--jobs", "--steps", "--trials"):
            opt[a[2:]] = int(val)
            i += 1
        elif a in ("--worlds", "--arms"):
            opt[a[2:]] = val.split(",")
            i += 1
        elif a == "--base":
            opt["base"] = val
            i += 1
        else:
            opt["extra"].append(a)
        i += 1
    if opt["worlds"] == ["all"]:
        opt["worlds"] = list(ph.WORLDS)
    return opt


def job(world, arm, seed, opt, folder):
    flags, lived, shuffled = arm_flags(arm)
    base = os.path.join(folder, f"{world}-{arm}-s{seed}")
    cmd = (["nice", "-n", "19", ph.PROBE, "predict", "--task", TASK_OF.get(world, world)] +
           ph.PLUG + WORLDS[world] + BASES[opt["base"]] + ph.WORLD_GENOME.get(world, []) +
           ph.PRED + flags +
           ["--seed", str(seed), "--trials", str(opt["trials"]),
            "--life", str(opt["steps"] if lived else 0), "--label", arm, "--future", "1,8",
            "--lean", "1", "--out", base + ".csv"])
    if shuffled:
        cmd += ["--shuffle", "1"]
    return cmd + opt["extra"], base + ".txt", base + ".csv"


def controls(arm, arms):
    """the arms an arm is set against: born, the rule off, its coin, its shuffled life"""
    if arm in ("born", "bornk", "pred", "predk") or arm[-1] in "cs" or arm[-2:] in ("ck", "sk"):
        return []
    k = "k" if arm.endswith("k") else ""
    stem = arm[:-1] if k else arm
    wanted = ["born" + k, "pred" + k, stem + "c" + k, stem + "s" + k]
    return [c for c in wanted if c in arms]


def tables(opt, folder):
    seeds, worlds, arms = opt["seeds"], opt["worlds"], opt["arms"]
    data = {(w, a, s): ph.read_csv(os.path.join(folder, f"{w}-{a}-s{s}.csv"))
            for w in worlds for a in arms for s in seeds}

    def got(w, a, q, k):
        return [data.get((w, a, s), {}).get((q, k)) for s in seeds]

    def cell(w, a, q, k, d=3):
        return ph.fmt(ph.mean(got(w, a, q, k)), 6, d)

    out = [f"# predevent {opt['name']}: seeds {','.join(map(str, seeds))}, {opt['steps']}-step "
           f"lives, {opt['trials']} read steps, base {opt['base']}; extra flags: "
           f"{' '.join(opt['extra']) or 'none'}\n"]
    csv = ["table,world,key,arm,seed,value"]
    for (w, a, s), d in data.items():
        for (q, k), v in d.items():
            csv.append(f"{q},{w},{k},{a},{s},{v:.6g}")
    open(os.path.join(OUT, opt["name"] + ".csv"), "w").write("\n".join(csv) + "\n")
    head = "| world | what | " + " | ".join(arms) + " |"
    rule = "|---|---|" + "---|" * len(arms)

    out.append("## 1. The inside read: a fresh linear reader of the hidden cells\n")
    out.append("Share right / hidden spikes per 1,000 cell-ticks while read. now: the same "
               "reader on this step's lines; maj: the most common answer.\n")
    out.append(head + " now | maj |")
    out.append(rule + "---|---|")
    qrows = []
    for w in worlds:
        qs = sorted({q for a in arms for s in seeds for (q, k) in data[(w, a, s)]
                     if q not in ("future", "life", "read")})
        for q in qs:
            qrows.append((w, q, "hidden"))
            cells = [f"{cell(w, a, q, 'hidden')} / "
                     f"{ph.fmt(ph.mean(got(w, a, q, 'rate_hidden')), 4, 1)}" for a in arms]
            out.append(f"| {w} | {q} | " + " | ".join(cells) + f" | "
                       f"{cell(w, arms[0], q, 'lines_now')} | {cell(w, arms[0], q, 'majority')} |")

    out.append("\n## 2. The future: a fresh ridge reader of every predicted line h steps on\n")
    out.append("From the hidden cells: share of line-steps right; balanced (the mean of right "
               "on on- and off-line-steps); changed (right on line-steps that differ from h "
               "steps before). lines: the same reader on this step's lines.\n")
    out.append(head + " lines |")
    out.append(rule + "---|")
    for w in worlds:
        for h in (1, 8):
            for key, name in (("", "right"), ("_bal", "balanced"), ("_chg", "changed")):
                qrows.append((w, "future", f"h{h}_hidden{key}"))
                cells = [cell(w, a, "future", f"h{h}_hidden{key}") for a in arms]
                out.append(f"| {w} | h={h} {name} | " + " | ".join(cells) +
                           f" | {cell(w, arms[0], 'future', f'h{h}_lines{key}')} |")

    out.append("\n## 3. Paired over seeds: an arm less its controls (mean, seeds above / "
               "below)\n")
    out.append("| world | what | arm | - born | - rule off | - its coin | - its shuffled life |")
    out.append("|---|---|---|---|---|---|---|")
    for w, q, k in qrows:
        for a in arms:
            cs = controls(a, arms)
            if not cs:
                continue
            kk = "k" if a.endswith("k") else ""
            stem = a[:-1] if kk else a
            cells = [ph.diff(got(w, a, q, k), got(w, c, q, k))[0] if c in cs else "-"
                     for c in ("born" + kk, "pred" + kk, stem + "c" + kk, stem + "s" + kk)]
            what = q if q != "future" else k
            out.append(f"| {w} | {what} | {a} | " + " | ".join(cells) + " |")

    lived = [a for a in arms if arm_flags(a)[1] and not arm_flags(a)[2]]
    out.append("\n## 4. The life: the task's late score, the predictor cells' skill "
               "(balanced, last quarter), the rule's work\n")
    out.append("| world | what | " + " | ".join(lived) + " |")
    out.append("|---|---|" + "---|" * len(lived))
    for w in worlds:
        for key, name, d in (("task_late", "task, late score", 3),
                             ("pred_h1_balanced", "predictor h=1", 3),
                             ("pred_h8_balanced", "predictor h=8", 3),
                             ("ticks_per_s", "ticks a second", 0)):
            out.append(f"| {w} | {name} | " + " | ".join(cell(w, a, "life", key, d)
                                                         for a in lived) + " |")
        rows = {"lines that taught, share": [], "teaching verdicts": [],
                "cells taught a verdict": [], "counter steps": [], "re-picks": []}
        for a in lived:
            v = {k: ph.mean(got(w, a, "life", k)) or 0 for k in
                 ("pred_seen", "pred_sent", "hop_verdicts", "hop_taught", "hop_steps",
                  "hop_rewires")}
            rows["lines that taught, share"].append(
                f"{v['pred_sent'] / v['pred_seen']:.3f}" if v["pred_seen"] else "-")
            rows["teaching verdicts"].append(f"{v['hop_verdicts']:.0f}")
            rows["cells taught a verdict"].append(
                f"{v['hop_taught'] / v['hop_verdicts']:.0f}" if v["hop_verdicts"] else "-")
            rows["counter steps"].append(f"{v['hop_steps']:.3g}")
            rows["re-picks"].append(f"{v['hop_rewires']:.0f}")
        for name, cells in rows.items():
            out.append(f"| {w} | {name} | " + " | ".join(cells) + " |")
        if any(v is not None for a in arms for v in got(w, a, "read", "own_score")):
            out.append(f"| {w} | read on its own moves: score | " +
                       " | ".join(cell(w, a, "read", "own_score") for a in lived) + " |")
    out.append("\nTask, late score, paired with the rule-off life (mean, seeds above / below):\n")
    for w in worlds:
        ref = "pred" if "pred" in arms else None
        if ref:
            out.append(f"- {w}: " + "; ".join(
                f"{a} {ph.diff(got(w, a, 'life', 'task_late'), got(w, ref, 'life', 'task_late'))[0]}"
                for a in lived if a != ref))
    text = "\n".join(out) + "\n"
    open(os.path.join(OUT, opt["name"] + ".md"), "w").write(text)
    print(text)


def main():
    opt = parse(sys.argv[1:])
    folder = os.path.join(OUT, opt["name"])
    os.makedirs(folder, exist_ok=True)
    if not opt["table_only"]:
        jobs = [job(w, a, s, opt, folder) for s in opt["seeds"] for w in opt["worlds"]
                for a in opt["arms"]]
        with ThreadPoolExecutor(max_workers=opt["jobs"]) as pool:
            list(pool.map(ph.run, jobs))
    tables(opt, folder)


if __name__ == "__main__":
    main()
