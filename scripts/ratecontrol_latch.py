"""The learned latch under a rate control (idea 175; scripts/ratecontrol.sh learned NAME): a
loop cell's inputs from cells that fire with it may become strong, by the counters
(competitive learning with tag_counts 1: strength from the counter), living under the
candidate. Delayed echo at 4, 8, 16, 50 steps, 1-back and copy, beside the rule without
strength from the counter, the rule off and churn under the same candidate, and the standard
control's lives; then the frozen read, a time-shuffled life and churn read the same way
(blobprobe transfer)."""

import os

import loops_probe as lp
import timerule_lib as tl
from timerule_lib import READOUT, SUITE, TW, delayed, fmt, mean

ROWS = [("delayed echo, 2 steps", delayed(2)), ("delayed echo, 4 steps", delayed(4)),
        ("delayed echo, 8 steps", delayed(8)), ("delayed echo, 16 steps", delayed(16)),
        ("delayed echo, 50 steps", delayed(50)),
        ("1-back, 4 symbols", ["--task", "n-back", "--k", "4", "--back", "1"]),
        ("copy 2 of 2", ["--task", "copy", "--k", "2", "--length", "2"])]
STRONG = "--g.tag_counts 1"


def arms_for(name, cand):
    """the arms under the candidate: the rule, the rule with strength from the counter (the
    learned latch), the rule off, churn with strength from the counter"""
    c = " ".join(cand)
    arms = {"rule": "rc-" + name, "latch": "rcl-" + name, "off": "rco-" + name,
            "churn": "rcc-" + name}
    tl.ARMS[arms["rule"]] = tl.ARMS["on"] + " " + c
    tl.ARMS[arms["latch"]] = tl.ARMS["on"] + " " + STRONG + " " + c
    tl.ARMS[arms["off"]] = TW + " " + c
    tl.ARMS[arms["churn"]] = tl.ARMS["churn"] + " " + STRONG + " " + c
    return arms


def control_job(folder, d, arm, flags, cand, seed, opt):
    """a life of half the steps at delay d, then the inside frozen and a fresh readout for the
    other half at the same delay; the candidate's numbers stay on the sheet for the read"""
    half = tl.scaled(SUITE["steps"], opt) // 2
    tag = arm + "".join(flags[::2]).replace("--", "_")
    log = os.path.join(folder, "d%d-%s-s%d.txt" % (d, tag, seed))
    cmd = (["nice", "-n", "19", lp.PROBE, "transfer", "--seed", str(seed), "--life",
            lp.life_text(delayed(d), SUITE, arm, opt, half), "--then-steps", str(half),
            "--window", "200" if opt["quick"] else "1000"] + flags +
           (READOUT + " " + TW).split() + cand + opt["extra"])
    return (cmd, log, log), log


def learned(name, cand, opt):
    arms = arms_for(name, cand)
    rows = [(label, args, SUITE) for label, args in ROWS]
    folder = os.path.join(opt["out"], "lives")
    order = ["on", "oncap", arms["off"], arms["rule"], arms["latch"], arms["churn"]]
    res = tl.grid(folder, rows, order, opt, by_args=True)
    lines = ["## The learned latch under %s (scripts/ratecontrol.sh learned %s)" % (name, name),
             "seeds %s, the suite's sheet and life (40,000 steps); late score, rate in brackets. "
             "on / oncap: competitive learning under the standard control, cap 64 / 1000; then "
             "under %s: the rule off, the rule, the rule with strength from the counter (%s), "
             "churn with it" % (",".join(map(str, opt["seeds"])), name, STRONG), "",
             "| row | " + " | ".join(order) + " | latch - rule (seeds up) |",
             "|" + "---|" * (len(order) + 2)]
    for label, _, _ in rows:
        cells = ["%s (%s)" % (fmt(mean([x[0] for x in res[label, a]])),
                              fmt(mean([x[1] for x in res[label, a]]), 0)) for a in order]
        d = [x[0] - y[0] for x, y in zip(res[label, arms["latch"]], res[label, arms["rule"]])
             if x[0] is not None and y[0] is not None]
        cells.append("%+.3f (%d/%d)" % (sum(d) / len(d), sum(v > 0 for v in d), len(d))
                     if d else "-")
        lines.append("| %s | %s |" % (label, " | ".join(cells)))
    # the controls read by a fresh readout on the frozen inside
    kinds = [("born", arms["off"], ["--born", "1"]), ("rule", arms["rule"], []),
             ("latch", arms["latch"], []), ("latch, shuffled", arms["latch"],
                                            ["--life-scramble", "1"]),
             ("churn", arms["churn"], [])]
    jobs, where = [], {}
    cfold = os.path.join(opt["out"], "learned-controls")
    for d in (4, 8):
        for kind, arm, flags in kinds:
            for s in opt["seeds"]:
                job, path = control_job(cfold, d, arm, flags, cand, s, opt)
                jobs.append(job)
                where.setdefault((d, kind), []).append(path)
    tl.run_all(jobs, opt)
    lines += ["", "Frozen read: a life of 20,000 steps at the delay under %s, then the inside "
              "frozen and a fresh readout for 20,000 steps; late score. shuffled: each life step "
              "shows another step's lines." % name, "",
              "| delay | " + " | ".join(k for k, _, _ in kinds) + " |",
              "|" + "---|" * (len(kinds) + 1)]
    for d in (4, 8):
        lines.append("| %d steps | %s |" % (d, " | ".join(
            fmt(mean([lp.transfer_late(p) for p in where[d, k]])) for k, _, _ in kinds)))
    return "\n".join(lines)
