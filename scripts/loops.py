#!/usr/bin/env python3
"""Making the learned loops hold (idea 172). See loops.sh; the tables of
docs/results-2026-11-26-loops.md. Lives, grids and tables come from timerule_lib."""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import timerule_lib as tl  # noqa: E402
from timerule_lib import (PRED, ROOT, SUITE, delayed, fmt, grid, mean, seeds_of)  # noqa: E402
from timerule import REACH_ROWS  # noqa: E402
import loops_probe as lp  # noqa: E402  part A: blobprobe looptrace and transfer

# the change of part B, and the runner-up: genome numbers added to every arm of a life
CHANGES = {
    "leak": "--g.leak_shift_min 4 --g.leak_shift_max 6",
    "leak56": "--g.leak_shift_min 5 --g.leak_shift_max 6",
}
for key, flags in CHANGES.items():
    tl.ARMS[key + "-on"] = tl.ARMS["on"] + " " + flags
    tl.ARMS[key + "-off"] = flags  # the setting adds the tile's winners, as in "off"
    tl.ARMS[key + "-churn"] = tl.ARMS["churn"] + " " + flags
    tl.ARMS[key + "-cap"] = tl.ARMS["oncap"] + " " + flags
# "off" arms of a change get the tile's winners from the setting too
_setting_flags = tl.setting_flags


def setting_flags(st, arm):
    if arm.endswith("-off") and st["offtw"]:
        return _setting_flags(st, arm) + tl.TW.split()
    return _setting_flags(st, arm)


tl.setting_flags = setting_flags

# part B's rows: the time and memory rows of the timerule job, the 1,000-tick hold, and the
# rows that must not break
B_ROWS = [(label, args, SUITE) for label, args, _ in REACH_ROWS
          if label not in ("sequence, 8 lines x 4", "same or different in turn")]
B_ROWS.insert(5, ("delayed echo, 50 steps", delayed(50), SUITE))
MUST_ROWS = [("exclusive-or (both-not-either)", ["--task", "both-not-either"], SUITE),
             ("echo, 16 cues", ["--task", "echo", "--k", "16"], SUITE),
             ("echo, cue and button 5 tiles apart", ["--task", "echo", "--distance", "5"], SUITE)]


CHURN_ROWS = ["delayed echo, 2 steps", "delayed echo, 4 steps", "delayed echo, 8 steps",
              "delayed echo, 16 steps", "1-back, 4 symbols", "copy 2 of 2", "gap"]


def b_table(title, rows, arms, res, opt, pairs):
    lines = [title, "seeds %s, the suite's sheet and life (40,000 steps); late score, rate in "
             "brackets" % ",".join(map(str, opt["seeds"])), "",
             "| row | " + " | ".join(arms) + " | " +
             " | ".join("%s - %s (seeds up)" % p for p in pairs) + " |",
             "|" + "---|" * (len(arms) + len(pairs) + 1)]
    for label, _, _ in rows:
        cells = ["%s (%s)" % (fmt(mean([x[0] for x in res[label, a]])),
                              fmt(mean([x[1] for x in res[label, a]]), 0)) for a in arms]
        for a, b in pairs:
            d = [x[0] - y[0] for x, y in zip(res[label, a], res[label, b])
                 if x[0] is not None and y[0] is not None]
            cells.append("%+.3f (%d/%d)" % (sum(d) / len(d), sum(v > 0 for v in d), len(d))
                         if d else "-")
        lines.append("| %s | %s |" % (label, " | ".join(cells)))
    return "\n".join(lines)


def best(opt):
    """part B: the change on every row, with rule off, churn and the rate held"""
    folder = os.path.join(opt["out"], "lives")
    lp.reuse_timerule(folder, opt)
    c = opt["change"]
    arms = ["off", "on", "oncap", c + "-off", c + "-on", c + "-cap"]
    rows = B_ROWS + MUST_ROWS
    res = grid(folder, rows, arms, opt, by_args=True)
    # plain churn: the timerule job's lives (no new rows)
    old = [r for r in rows if r[0] in [x[0] for x in REACH_ROWS]]
    res.update(grid(folder, old, ["churn"], opt, by_args=True))
    for label, _, _ in rows:
        res.setdefault((label, "churn"), [(None, None, None)])
    arms.insert(3, "churn")
    # churn under the change: only on the rows a held trace should move
    some = [r for r in rows if r[0] in CHURN_ROWS]
    res.update(grid(folder, some, [c + "-churn"], opt, by_args=True))
    for label, _, _ in rows:
        res.setdefault((label, c + "-churn"), [(None, None, None)])
    arms.append(c + "-churn")
    return b_table("## B. %s on every row (scripts/loops.sh best --change %s): %s" % (
        c, c, CHANGES[c]), rows, arms, res, opt,
        [(c + "-on", "on"), (c + "-on", c + "-off"), (c + "-cap", "oncap")])


def runner(opt):
    """part B: the runner-up change on the delayed-echo rows and the rows that must not break,
    rule on and at target rate"""
    folder = os.path.join(opt["out"], "lives")
    c = opt["change"]
    arms = ["on", c + "-on", c + "-cap"]
    rows = [r for r in B_ROWS if r[0].startswith("delayed")] + MUST_ROWS
    res = grid(folder, rows, arms, opt, by_args=True)
    return b_table("## B. %s, the runner-up (scripts/loops.sh runner --change %s): %s" % (
        c, c, CHANGES[c]), rows, arms, res, opt, [(c + "-on", "on")])


def long_life(opt):
    """a 160,000-step life with the change: rate, silent and stuck cells, late score"""
    folder = os.path.join(opt["out"], "long")
    c = opt["change"]
    rows = [("delayed echo, 8 steps, 160,000 steps", delayed(8), dict(SUITE, steps=160000)),
            ("1-back, 160,000 steps", ["--task", "n-back", "--k", "4", "--back", "1"],
             dict(SUITE, steps=160000))]
    arms = ["on", c + "-on", "oncap", c + "-cap"]
    jobs, where = [], {}
    for label, args, st in rows:
        for arm in arms:
            for s in opt["seeds"]:
                name = "%s-%s" % (tl.label_key(label), arm)
                job, path = tl.life_job(folder, name, args, st, arm, s, opt)
                jobs.append(job)
                where.setdefault((label, arm), []).append(path)
    tl.run_all(jobs, opt)
    keys = ["late", "rate_per_1024", "share_silent", "share_stuck", "share_threshold_cap"]
    lines = ["## Long life (scripts/loops.sh long --change %s)" % c,
             "seeds %s; late score, rate per 1,024, share of cells silent and stuck at the end, "
             "share of thresholds at their cap" % ",".join(map(str, opt["seeds"])), "",
             "| row | arm | " + " | ".join(keys) + " |", "|" + "---|" * (len(keys) + 2)]
    for label, _, _ in rows:
        for arm in arms:
            d = [tl.read_sum(p) for p in where[label, arm]]
            lines.append("| %s | %s | %s |" % (label, arm, " | ".join(
                fmt(mean([x.get(("blob", k)) for x in d]), 3) for k in keys)))
    return "\n".join(lines)


def mnist(opt):
    """the MNIST inside read (scripts/inside.sh), competitive learning with and without"""
    import subprocess
    seeds = ",".join(map(str, opt["seeds"]))
    b5 = "--g.learn_mode 5 --g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5".split()
    texts = []
    cap = ["--g.threshold_cap", "1000"]
    change = CHANGES[opt["change"]].split()
    for name, flags in (("loops-b5", b5), ("loops-b5-" + opt["change"], b5 + change),
                        ("loops-b5cap", b5 + cap),
                        ("loops-b5-" + opt["change"] + "cap", b5 + change + cap)):
        cmd = [os.path.join(ROOT, "scripts", "inside.sh"), name, "--seeds", seeds, "--only",
               "mnist", "--jobs", str(opt["jobs"])] + (["--table-only"] if opt["table_only"]
                                                       else []) + flags
        if opt["quick"]:
            cmd += ["--life", "2000", "--fit", "2000"]
        out = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True).stdout
        texts.append("### %s: %s\n\n%s" % (name, " ".join(flags), out.strip()))
    return "## MNIST inside read (scripts/inside.sh)\n\n" + "\n\n".join(texts)


MODES = {"trace": lp.trace, "suspects": lp.suspects, "pred": lp.pred, "best": best,
         "runner": runner, "controls": lp.controls, "long": long_life, "mnist": mnist}
DEFAULT_SEEDS = {"trace": [5, 6, 7, 8], "suspects": [5, 6, 7, 8], "pred": [5, 6, 7, 8],
                 "best": list(range(5, 13)), "runner": list(range(5, 13)),
                 "controls": list(range(5, 13)), "long": [5, 6, 7, 8], "mnist": [5, 6, 7, 8]}


def main():
    argv = sys.argv[1:]
    if not argv or argv[0].startswith("-"):
        sys.stderr.write(open(os.path.join(ROOT, "scripts", "loops.sh")).read().split("set -")[0])
        return 1
    opt = {"mode": argv[0], "seeds": None, "jobs": 3, "quick": False, "table_only": False,
           "extra": [], "change": "leak"}
    i = 1
    while i < len(argv):
        a = argv[i]
        if a == "--quick":
            opt["quick"] = True
        elif a == "--table-only":
            opt["table_only"] = True
        elif a in ("--seeds", "--jobs", "--change", "--arms"):
            v = argv[i + 1]
            opt[a[2:]] = (seeds_of(v) if a == "--seeds" else int(v) if a == "--jobs" else
                          v.split(",") if a == "--arms" else v)
            i += 1
        else:
            opt["extra"].append(a)
        i += 1
    if opt["change"] not in CHANGES:
        print("loops: changes are %s" % ", ".join(CHANGES))
        return 1
    opt["out"] = os.path.join(ROOT, "out", "loops" + ("-quick" if opt["quick"] else ""))
    modes = [m for m in MODES if m != "mnist"] if opt["mode"] == "all" else opt["mode"].split(",")
    for m in modes:
        if m not in MODES:
            print("loops: modes are %s, all" % ", ".join(MODES))
            return 1
        o = dict(opt)
        o["seeds"] = opt["seeds"] or ([5, 6] if opt["quick"] else DEFAULT_SEEDS[m])
        text = MODES[m](o)
        os.makedirs(opt["out"], exist_ok=True)
        name = m + ("-" + opt["change"] if m in ("best", "runner", "long", "mnist") else "")
        with open(os.path.join(opt["out"], name + ".md"), "w") as f:
            f.write(text + "\n")
        print(text + "\n", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
