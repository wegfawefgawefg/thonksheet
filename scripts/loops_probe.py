"""Part A of idea 172 (scripts/loops.py): what ends the learned loop. blobprobe looptrace on a
lived sheet with one suspect switched after the life (the wiring is the life's), or with the
rule living under the change; blobprobe transfer for the task scores. Also the controls of
part B and the prediction sheet's rate (blobprobe looptrace's state line)."""

import os
import re

import timerule_lib as tl
from timerule_lib import (CAP, PRED, PROBE, READOUT, SUITE, TW, delayed, fmt, label_key, mean,
                          run_all, scaled)

# suspects switched after a normal life: (name, probe flags). the wiring stays the life's
AFTER = [
    ("as lived", []),
    ("sector bias held at 0", ["--after", "sector_reg_on 0", "--set-bias", "0"]),
    ("sector bias at most 4", ["--after", "sector_bias_max 4"]),
    ("thresholds frozen", ["--after", "threshold_reg_on 0"]),
    ("thresholds 4x slower", ["--after", "threshold_up_shift 6 threshold_down_shift 11"]),
    ("no tile winners", ["--after", "tile_winners 0"]),
    ("tile winners 8", ["--after", "tile_winners 8"]),
    ("inhibition 3 (was 5)", ["--after", "inhibit_gain 3"]),
    ("cooldown 1", ["--set-cool", "1"]),
    ("leak shift 6 (was 1-4)", ["--set-leak", "6"]),
    ("loop inputs all strong", ["--loop-strong", "1"]),
    ("no noise, no pacemakers", ["--after", "noise_on 0", "--quiet-pace", "1"]),
]
# the rule living under the change: (name, life flags added to the rule-on arm)
UNDER = [
    ("life: sector bias off", "--g.sector_reg_on 0"),
    ("life: sector bias at most 4", "--g.sector_bias_max 4"),
    ("life: thresholds 4x slower", "--g.threshold_up_shift 6 --g.threshold_down_shift 11"),
    ("life: no tile winners", "--g.tile_winners 0"),
    ("life: inhibition 3", "--g.inhibit_gain 3"),
    ("life: cooldown 1", "--g.cooldown_min 1 --g.cooldown_max 1"),
    ("life: no noise", "--g.noise_on 0"),
    ("life: leak shift 4-6", "--g.leak_shift_min 4 --g.leak_shift_max 6"),
    ("life: leak shift 4-6, slow bias step", "--g.leak_shift_min 4 --g.leak_shift_max 6 "
     "--g.sector_step_shift 3"),
    ("life: slow bias step", "--g.sector_step_shift 3"),
]
for _name, _flags in UNDER:
    tl.ARMS["u-" + label_key(_name)] = tl.ARMS["on"] + " " + _flags
DELAYS = [2, 3, 4, 8, 16]
# the ones scored on delayed echo (the trace table has them all). the leak lives are part
# B's own arms (loops.py CHANGES), so best reuses them
SCORE_AFTER = ["as lived", "sector bias held at 0", "sector bias at most 4", "thresholds frozen",
               "no tile winners", "inhibition 3 (was 5)", "cooldown 1", "leak shift 6 (was 1-4)",
               "loop inputs all strong", "no noise, no pacemakers"]
SCORE_UNDER = [("life: sector bias off", "u-life__sector_bias_off"),
               ("life: sector bias at most 4", "u-life__sector_bias_at_most_4"),
               ("life: slow bias step", "u-life__slow_bias_step"),
               ("life: leak shift 4-6", "leak-on"),
               ("life: leak shift 4-6, slow bias step", "leakstep-on")]


def reuse_timerule(folder, opt):
    """the plain arms' lives of the timerule job (the same flags, the same code: copied, not
    run again). out/from-agents/timerule/lives, if there"""
    src = os.path.join(tl.ROOT, "out", "from-agents", "timerule", "lives")
    if opt["quick"] or opt["table_only"] or not os.path.isdir(src):
        return
    os.makedirs(folder, exist_ok=True)
    for name in os.listdir(src):
        if name.endswith("-sum.csv") and not os.path.exists(os.path.join(folder, name)):
            with open(os.path.join(src, name)) as f, open(os.path.join(folder, name), "w") as g:
                g.write(f.read())


def life_text(task_args, st, arm, opt, steps=None):
    steps = steps or scaled(st["steps"], opt)
    return " ".join(task_args + tl.setting_flags(st, arm) +
                    ["--steps", str(steps), "--window", "200" if opt["quick"] else "1000"])


def trace_job(folder, name, st, arm, flags, seed, opt):
    base = os.path.join(folder, "%s-s%d" % (label_key(name), seed))
    q = opt["quick"]
    cmd = (["nice", "-n", "19", PROBE, "looptrace", "--seed", str(seed), "--life",
            life_text(delayed(2), st, arm, opt), "--read-delay", "8" if q else "16",
            "--calib", "300" if q else "600", "--trials", "600" if q else "1200",
            "--out", base + ".csv"] + flags + (READOUT + " " + TW).split() + opt["extra"])
    return (cmd, base + ".txt", base + ".txt"), base + ".txt"


def trace_parse(path):
    out = {"read": [], "vote": [], "lit": [], "rival": [], "bias": [], "trace": None}
    if not os.path.exists(path):
        return out
    for line in open(path):
        m = re.match(r"step \w+ \d+: read ([0-9.]+) vote ([0-9.]+); per 1000 cell-ticks lit "
                     r"([0-9.]+) rival ([0-9.]+) .* bias (-?[0-9.]+);", line)
        if m:
            for k, v in zip(("read", "vote", "lit", "rival", "bias"), m.groups()):
                out[k].append(float(v))
        m = re.match(r"looptrace .*loop cells (\d+) \(cue 0\) (\d+)", line)
        if m:
            out["loops"] = int(m.group(1)) + int(m.group(2))
        m = re.match(r"trace: (\d+)", line)
        if m:
            out["trace"] = int(m.group(1))
        m = re.match(r"state: hidden rate ([0-9.]+) .*thresholds mean ([0-9.]+), at cap ([0-9.]+)"
                     r".*bias mean (-?[0-9.]+), at \+max ([0-9.]+), at -max ([0-9.]+)", line)
        if m:
            out["state"] = [float(v) for v in m.groups()]
    return out


def trace_grid(folder, sheets, opt):
    """sheets: (name, setting, arm, probe flags); returns {name: [parsed per seed]}"""
    jobs, where = [], {}
    for name, st, arm, flags in sheets:
        for s in opt["seeds"]:
            job, path = trace_job(folder, name, st, arm, flags, s, opt)
            jobs.append(job)
            where.setdefault(name, []).append(path)
    run_all(jobs, opt)
    return {k: [trace_parse(p) for p in v] for k, v in where.items()}


def trace(opt):
    # the rule off: is a held trace the learned loop, or the cells alone?
    sheets = [("born", SUITE, "off", ["--born", "1"]), ("rule off", SUITE, "off", []),
              ("rule off, life: leak shift 4-6", SUITE, "leak-off", [])]
    sheets += [(n, SUITE, "on", f) for n, f in AFTER]
    sheets += [(n, SUITE, "u-" + label_key(n), []) for n, _ in UNDER]
    res = trace_grid(os.path.join(opt["out"], "trace"), sheets, opt)
    blanks = [1, 2, 3, 4, 8, 16] if not opt["quick"] else [1, 2, 3, 4, 8]
    lines = ["## A. The trace (scripts/loops.sh trace)",
             "seeds %s; the suite's sheet, a delayed-echo life of 2 steps with the rule; then one "
             "suspect switched (or the rule lived under it), every rule stopped, delayed echo "
             "read with %d blanks. trace: blanks a fresh ridge reader of all hidden cells reads "
             "at 0.75 or more, seed by seed then the mean. vote: the loop's own vote (cue-1 loop "
             "cells against cue-0). lit / rival: spikes per 1,000 cell-ticks of the loop of the "
             "cue shown and of the other. bias: the lit cells' sector bias. rate: hidden cells "
             "per 1,024 over the read." % (",".join(map(str, opt["seeds"])), blanks[-1]), "",
             "| sheet | trace | " + " | ".join("vote b%d" % b for b in blanks) +
             " | lit cue / b1 / b2 / b4 | rival b4 | bias cue / b1 / b2 | rate |",
             "|" + "---|" * (len(blanks) + 6)]
    for name, _, _, _ in sheets:
        p = [x for x in res[name] if x["vote"]]
        if not p:
            continue
        col = lambda k, i, d=3: fmt(mean([x[k][i] for x in p if len(x[k]) > i]), d)  # noqa: E731
        lines.append("| %s | %s (%s) | %s | %s | %s | %s | %s |" % (
            name, fmt(mean([x["trace"] for x in p]), 1), " ".join(str(x["trace"]) for x in p),
            " | ".join(col("vote", b) for b in blanks),
            " / ".join(col("lit", i, 0) for i in (0, 1, 2, 4)), col("rival", 4, 0),
            " / ".join(col("bias", i, 1) for i in (0, 1, 2)),
            fmt(mean([x["state"][0] for x in p if "state" in x]), 1)))
    lines += ["", tick_table(os.path.join(opt["out"], "trace"), opt,
                             ["as lived", "sector bias held at 0", "leak shift 6 (was 1-4)",
                              "life: leak shift 4-6"])]
    return "\n".join(lines)


TICKS = [0, 10, 19, 20, 25, 30, 35, 40, 50, 60, 80, 100, 160, 260]
TICK_COLS = [("lit", 1000, 0), ("rival", 1000, 0), ("inhib", 1000, 0), ("lit_thr", 1, 2),
             ("lit_bias", 1, 1), ("lit_cut", 1, 3), ("lit_full", 1, 3)]


def tick_table(folder, opt, names):
    """the per-tick csv of looptrace, mean of the seeds, at a few ticks of the trial"""
    import csv
    lines = ["Tick by tick through the trial (the cue is ticks 0-19, blank 1 20-39, ...), mean "
             "of the seeds: lit / rival / inhibitory spikes per 1,000 cell-ticks, the lit cells' "
             "threshold and sector bias, the share of lit cells over their bar but cut (rested, "
             "not fired: the tile's winners), the share whose tile spent all its winners.", ""]
    for name in names:
        rows = []
        for s in opt["seeds"]:
            path = os.path.join(folder, "%s-s%d.csv" % (label_key(name), s))
            if os.path.exists(path):
                rows.append({int(r["tick"]): r for r in csv.DictReader(open(path))})
        if not rows:
            continue
        lines += ["%s:" % name, "", "| tick | " + " | ".join(c for c, _, _ in TICK_COLS) + " |",
                  "|" + "---|" * (len(TICK_COLS) + 1)]
        for t in TICKS:
            got = [r[t] for r in rows if t in r]
            if got:
                lines.append("| %d | %s |" % (t, " | ".join(
                    fmt(mean([float(g[c]) * k for g in got]), d) for c, k, d in TICK_COLS)))
        lines.append("")
    return "\n".join(lines)


# ---- the task scores: after a normal life with the suspect switched (fresh readout, the
# inside frozen), and lives with the rule living under the change ----

def transfer_job(folder, name, delay, st, arm, flags, seed, opt, extra=()):
    half = scaled(st["steps"], opt) // 2
    window = "200" if opt["quick"] else "1000"
    log = os.path.join(folder, "%s-d%d-%s-s%d.txt" % (label_key(name), delay, arm, seed))
    then = life_text(delayed(delay), st, "off", opt, half)
    cmd = (["nice", "-n", "19", PROBE, "transfer", "--seed", str(seed), "--life",
            life_text(delayed(2), st, arm, opt, half), "--then-flags", then, "--then-steps",
            str(half), "--window", window] + list(flags) + list(extra) +
           (READOUT + " " + TW + (" " + CAP if "cap" in arm else "")).split() + opt["extra"])
    return (cmd, log, log), log


def transfer_late(path):
    if not os.path.exists(path):
        return None
    m = re.search(r"^transfer .* late ([0-9.]+)", open(path).read(), re.M)
    return float(m.group(1)) if m else None


def suspects(opt):
    folder = os.path.join(opt["out"], "suspects")
    jobs, where = [], {}
    for name, flags in [a for a in AFTER if a[0] in SCORE_AFTER]:
        for d in DELAYS:
            for s in opt["seeds"]:
                job, path = transfer_job(folder, name, d, SUITE, "on", flags, s, opt)
                jobs.append(job)
                where.setdefault((name, d), []).append(path)
    run_all(jobs, opt)
    reuse_timerule(os.path.join(opt["out"], "lives"), opt)
    rows = [("delay %d" % d, delayed(d), SUITE) for d in DELAYS]
    arms = ["on"] + [a for _, a in SCORE_UNDER]
    under = tl.grid(os.path.join(opt["out"], "lives"), rows, arms, opt, by_args=True)
    lines = ["## A. Delayed echo with each suspect switched (scripts/loops.sh suspects)",
             "seeds %s. After a normal life: a delayed-echo life of 2 steps with the rule (half "
             "the steps), then the suspect switched, the inside frozen, the motor cells as born "
             "and a fresh readout learning delayed echo at the delay of the column (the other "
             "half); its late score. Under the change: a whole life (blobrun) at that delay with "
             "the rule living under the change; late score and rate." %
             ",".join(map(str, opt["seeds"])), "",
             "| after a normal life | " + " | ".join("%d steps" % d for d in DELAYS) + " |",
             "|" + "---|" * (len(DELAYS) + 1)]
    for name in SCORE_AFTER:
        lines.append("| %s | %s |" % (name, " | ".join(
            fmt(mean([transfer_late(p) for p in where[name, d]])) for d in DELAYS)))
    lines += ["", "| rule living under it | " + " | ".join("%d steps" % d for d in DELAYS) + " |",
              "|" + "---|" * (len(DELAYS) + 1)]
    for name, arm in [("rule on, as it is", "on")] + SCORE_UNDER:
        cells = []
        for label, _, _ in rows:
            v = under[label, arm]
            cells.append("%s (%s)" % (fmt(mean([x[0] for x in v])), fmt(mean([x[1] for x in v]), 0)))
        lines.append("| %s | %s |" % (name, " | ".join(cells)))
    return "\n".join(lines)


# ---- the prediction sheet's rate (side 2 with the wide sensor lattice) ----

def pred(opt):
    p2 = dict(PRED, delay=2)
    sheets = []
    for label, st in (("side 2, wide lattice", p2), ("side 3, wide lattice", dict(p2, side=3)),
                      ("side 2, standard lattice", dict(p2, lattice="std")),
                      ("suite (side 3, standard)", SUITE)):
        for arm in ("off", "on", "oncap"):
            sheets.append(("%s, %s" % (label, arm), st, arm, []))
    res = trace_grid(os.path.join(opt["out"], "pred"), sheets, opt)
    lines = ["## The rate on the prediction sheet (scripts/loops.sh pred)",
             "seeds %s; delayed echo of 2 steps, then the read as in the trace table. rate: hidden "
             "cells per 1,024 cell-ticks over the read; thresholds: hidden cells' mean, and the "
             "share at their cap (threshold_cap); bias: sector bias mean, share of sector-ticks "
             "at +max and at -max." % ",".join(map(str, opt["seeds"])), "",
             "| sheet | rate | threshold mean | at cap | bias mean | at +max | at -max | loop "
             "cells | vote b1 / b2 |", "|---|---|---|---|---|---|---|---|---|"]
    for name, _, _, _ in sheets:
        p = [x for x in res[name] if "state" in x]
        if not p:
            continue
        st = [fmt(mean([x["state"][i] for x in p]), d) for i, d in
              ((0, 1), (1, 1), (2, 3), (3, 1), (4, 3), (5, 3))]
        loops = fmt(mean([x.get("loops", 0) for x in p]), 0)
        lines.append("| %s | %s | %s | %s / %s |" % (name, " | ".join(st), loops,
                                                     fmt(mean([x["vote"][1] for x in p])),
                                                     fmt(mean([x["vote"][2] for x in p]))))
    return "\n".join(lines)


# ---- part B's controls: a life with the change, then the inside frozen and a fresh readout
# (blobprobe transfer, as scripts/timerule.sh controls) ----

CONTROL_ROWS = [("delayed echo, 4 steps", 4), ("delayed echo, 8 steps", 8)]


def controls(opt):
    folder = os.path.join(opt["out"], "controls")
    c = opt["change"]
    kinds = [("born", "off", ["--born", "1"]), ("lived", "on", []),
             ("born, change", c + "-off", ["--born", "1"]), ("lived, change", c + "-on", []),
             ("shuffled, change", c + "-on", ["--life-scramble", "1"]),
             ("churn, change", c + "-churn", []), ("lived, change, cap", c + "-cap", [])]
    jobs, where = [], {}
    for label, d in CONTROL_ROWS:
        for kind, arm, flags in kinds:
            for s in opt["seeds"]:
                # the life at the row's own delay; read at the same delay
                job, path = _control_job(folder, d, arm, flags, s, opt)
                jobs.append(job)
                where.setdefault((label, kind), []).append(path)
    run_all(jobs, opt)
    names = [k for k, _, _ in kinds]
    lines = ["## B. Controls (scripts/loops.sh controls --change %s)" % c,
             "seeds %s. A life of half the steps at the row's delay (born: none), then the inside "
             "frozen and a fresh readout for the other half; late score. shuffled: each life step "
             "shows the lines of a step drawn from the last 256; churn: the rule's moves toward "
             "random sources; cap: the rate held at target in life and read." %
             ",".join(map(str, opt["seeds"])), "",
             "| row | " + " | ".join(names) + " |", "|" + "---|" * (len(names) + 1)]
    for label, _ in CONTROL_ROWS:
        lines.append("| %s | %s |" % (label, " | ".join(
            fmt(mean([transfer_late(p) for p in where[label, k]])) for k in names)))
    return "\n".join(lines)


def _control_job(folder, d, arm, flags, seed, opt):
    st = SUITE
    half = scaled(st["steps"], opt) // 2
    window = "200" if opt["quick"] else "1000"
    tag = arm + "".join(flags[::2]).replace("--", "_")
    log = os.path.join(folder, "d%d-%s-s%d.txt" % (d, tag, seed))
    # the life's genome numbers (the change among them) stay on the sheet for the read; the
    # probe stops the rule
    genome = READOUT + " " + TW + (" " + CAP if "cap" in arm else "")
    cmd = (["nice", "-n", "19", PROBE, "transfer", "--seed", str(seed), "--life",
            life_text(delayed(d), st, arm, opt, half), "--then-steps", str(half), "--window",
            window] + flags + genome.split() + opt["extra"])
    return (cmd, log, log), log
