"""Point 3 and 4 of idea 166 (scripts/timerule.py): lives through blobprobe transfer (a life,
then the inside frozen and a fresh readout) and blobprobe timewire (what the cells wired)."""

import csv
import os
import re

from timerule_lib import (CAP, PROBE, READOUT, SUITE, PRED, TW, fmt, mean, run_all, scaled,
                          setting_flags, setting_key, label_key, delayed)

# point 3: a life of half the steps, then the inside frozen and a fresh readout for the other
# half (blobprobe transfer). name: (the life's arm, transfer flags). Measurements, not a
# method: the method is always the one life with every rule on
TRANSFER = [
    ("born", "off", ["--born", "1"]),
    ("lived", "on", []),
    ("shuffled", "on", ["--life-scramble", "1"]),
    ("quiet", "on", ["--life-quiet", "1"]),
    ("churn", "churn", []),
    ("running", "on", ["--then-inside", "1"]),
    ("kept", "on", ["--then-keep", "1"]),
    ("born-cap", "offcap", ["--born", "1"]),
    ("lived-cap", "oncap", []),
    ("shuffled-cap", "oncap", ["--life-scramble", "1"]),
]
CONTROL_ROWS = [("delayed echo d2", delayed(2), SUITE), ("gap", ["--task", "gap"], SUITE),
                ("order", ["--task", "order"], SUITE)]


def transfer_job(folder, label, task_args, st, arm, flags, seed, opt):
    half = scaled(st["steps"], opt) // 2
    window = "200" if opt["quick"] else "1000"
    life = task_args + setting_flags(st, arm) + ["--steps", str(half), "--window", window]
    genome = (READOUT + " " + TW + (" " + CAP if "cap" in arm else "")).split()
    log = os.path.join(folder, "%s-%s-%s-s%d.txt" % (label, setting_key(st), arm +
                                                      "".join(flags[::2]).replace("--", "_"), seed))
    cmd = (["nice", "-n", "19", PROBE, "transfer", "--seed", str(seed), "--life", " ".join(life),
            "--then-steps", str(half), "--window", window] + flags + genome + opt["extra"])
    return (cmd, log, log), log


def transfer_late(path):
    if not os.path.exists(path):
        return None
    m = re.search(r"^transfer .* late ([0-9.]+)", open(path).read(), re.M)
    return float(m.group(1)) if m else None


def controls(opt, lives):
    """the freeze arms beside the one-life numbers (lives: {(label, arm): [(late, ..)]})"""
    folder = os.path.join(opt["out"], "controls")
    jobs, where = [], {}
    for label, task_args, st in CONTROL_ROWS:
        for name, arm, flags in TRANSFER:
            for s in opt["seeds"]:
                job, path = transfer_job(folder, label_key(label), task_args, st, arm, flags, s, opt)
                jobs.append(job)
                where.setdefault((label, name), []).append(path)
    run_all(jobs, opt)
    names = [n for n, _, _ in TRANSFER]
    lines = ["## 3. Is it time? Freeze, shuffle, churn, rate (scripts/timerule.sh controls)",
             "seeds %s. One life (blobrun, the whole life, the rule on beside the readout): off, "
             "on, churn, on at target rate. Transfer (blobprobe transfer): a life of half the "
             "steps, then the inside frozen and a fresh readout learns the task for the other "
             "half; late score of that readout. born: no life; lived: the rule on, the task in "
             "order, rewarded; shuffled: the same but each step shows the lines of a step "
             "drawn from the last 256 (time order gone); quiet: no good or bad in the life; "
             "churn: the moves toward random sources; running: the rule left on after the "
             "life; kept: the inside frozen but the life's readout kept and learning on (rule "
             "on for the first half of one life, then off); -cap: the rate held at target "
             "(--g.threshold_cap 1000) in life and read." % ",".join(map(str, opt["seeds"])), ""]
    one = ["off", "on", "churn", "oncap"]
    lines += ["| row | " + " | ".join("life " + a for a in one) + " | " + " | ".join(names) + " |",
              "|" + "---|" * (len(one) + len(names) + 1)]
    for label, _, _ in CONTROL_ROWS:
        cells = [fmt(mean([x[0] for x in lives.get((label, a), [])])) for a in one]
        cells += [fmt(mean([transfer_late(p) for p in where[label, n]])) for n in names]
        lines.append("| %s | %s |" % (label, " | ".join(cells)))
    lines += ["", "Paired against born (transfer), mean and seeds above:", "",
              "| row | " + " | ".join(n for n in names if n != "born") + " |",
              "|" + "---|" * len(names)]
    for label, _, _ in CONTROL_ROWS:
        cells = []
        for n in names:
            if n == "born":
                continue
            base = "born-cap" if n.endswith("-cap") else "born"
            d = [transfer_late(a) - transfer_late(b) for a, b in zip(where[label, n],
                                                                    where[label, base])
                 if transfer_late(a) is not None and transfer_late(b) is not None]
            cells.append("%+.3f (%d/%d)" % (sum(d) / len(d), sum(x > 0 for x in d), len(d))
                         if d else "-")
        lines.append("| %s | %s |" % (label, " | ".join(cells)))
    return "\n".join(lines)


# ---- point 4: what the cells wired (blobprobe timewire) ----

# sheets: name, task args, setting, the life's arm, transfer-style flags
WIRE_SHEETS = [
    ("suite d2 born", delayed(2), SUITE, "off", ["--born", "1"]),
    ("suite d2 on", delayed(2), SUITE, "on", []),
    ("suite d2 churn", delayed(2), SUITE, "churn", []),
    ("suite d2 shuffled", delayed(2), SUITE, "on", ["--life-scramble", "1"]),
    ("suite d2 quiet", delayed(2), SUITE, "on", ["--life-quiet", "1"]),
    ("suite d4 born", delayed(4), dict(SUITE, delay=4), "off", ["--born", "1"]),
    ("suite d4 on", delayed(4), dict(SUITE, delay=4), "on", []),
    ("pred d4 born", delayed(4), PRED, "off", ["--born", "1"]),
    ("pred d4 on", delayed(4), PRED, "on", []),
    ("pred d2 born", delayed(2), dict(PRED, delay=2), "off", ["--born", "1"]),
    ("pred d2 on", delayed(2), dict(PRED, delay=2), "on", []),
]


def wire_job(folder, sheet, seed, opt):
    name, task_args, st, arm, flags = sheet
    steps = scaled(st["steps"], opt)
    life = task_args + setting_flags(st, arm) + ["--steps", str(steps), "--window",
                                                 "200" if opt["quick"] else "1000"]
    base = os.path.join(folder, "%s-s%d" % (label_key(name), seed))
    cmd = (["nice", "-n", "19", PROBE, "timewire", "--seed", str(seed), "--life", " ".join(life),
            "--trials", "400" if opt["quick"] else "2000", "--out", base + ".csv"] + flags +
           (READOUT + " " + TW).split() + opt["extra"])
    return (cmd, base + ".txt", base + ".txt"), base


def wire_parse(base):
    """read shares by step, carriers by step, and the wiring lines of the probe's report"""
    out = {"read": [], "near": [], "carriers": [], "wiring": {}}
    if not os.path.exists(base + ".txt"):
        return out
    for line in open(base + ".txt"):
        m = re.match(r"read (\w+) (\d+): all ([0-9.]+) near ([0-9.]+), (\d+) cells", line)
        if m:
            out["read"].append(float(m.group(3)))
            out["near"].append(float(m.group(4)))
            out["carriers"].append(int(m.group(5)))
        m = re.match(r"(top|carriers|all hidden) +connections per cell: (.*)", line)
        if m:
            pairs = re.findall(r"(\S+) ([0-9.]+)", m.group(2))
            out["wiring"][m.group(1)] = {k: float(v) for k, v in pairs}
    return out


def follow(lived_base, other_bases, top=64):
    """the lived sheet's top cells (by the reader's weight), and the same cells elsewhere"""
    def rows(path):
        return {int(r["neuron"]): r for r in csv.DictReader(open(path))} if os.path.exists(path) else {}
    lived = rows(lived_base + ".csv")
    ids = [n for n, r in lived.items() if int(r["rank"]) < top]
    res = {}
    for name, base in [("lived", lived_base)] + other_bases:
        r = rows(base + ".csv")
        if not r or not ids:
            continue
        res[name] = {k: mean([float(r[n][k]) for n in ids if n in r])
                     for k in ("n_cue+", "n_carry+", "n_carry-", "n_hidden", "n_motor", "n_inhib")}
        res[name]["|carry|"] = mean([abs(float(r[n]["carry"])) for n in ids if n in r])
    return res


def wiring(opt):
    folder = os.path.join(opt["out"], "wiring")
    jobs, bases = [], {}
    for sheet in WIRE_SHEETS:
        for s in opt["seeds"]:
            job, base = wire_job(folder, sheet, s, opt)
            jobs.append(job)
            bases.setdefault(sheet[0], []).append(base)
    run_all(jobs, opt)
    lines = ["## 4. What the cells wired (scripts/timerule.sh wiring)",
             "seeds %s; after the life every rule stopped and delayed echo shown again (fresh "
             "task seed, 2,000 trials). read: a fresh ridge reader of the cue from every hidden "
             "cell (near: only those in the 3x3 tiles round a button's cell, what the readout "
             "can hear), test trials, at each step of the trial (cue, blanks, ask; the ask step "
             "also hears the press). carriers: hidden cells with |d'| >= 0.5 for the cue at "
             "that step." % ",".join(map(str, opt["seeds"])), "",
             "| sheet | read by step (all) | read by step (near) | carriers by step |",
             "|---|---|---|---|"]
    parsed = {k: [wire_parse(b) for b in v] for k, v in bases.items()}
    for name, _, _, _, _ in WIRE_SHEETS:
        p = [x for x in parsed[name] if x["read"]]
        if not p:
            continue
        steps = len(p[0]["read"])
        col = lambda key, d: " / ".join(fmt(mean([x[key][i] for x in p]), d) for i in range(steps))
        lines.append("| %s | %s | %s | %s |" % (name, col("read", 3), col("near", 3), col("carriers", 0)))
    lines += ["", "Connections per cell by source (cue+: the cue line the cell prefers; carry+ / "
              "carry-: hidden cells that hold the same / the other cue through the blanks; "
              "hidden: other hidden cells; inhib: inhibitory). carriers: every carrier cell, its "
              "preference by its own carry; top: the 64 cells the last-blank reader leans on.", "",
              "| sheet | cells | cue+ | cue- | ask | line | motor | carry+ | carry- | hidden | inhib |",
              "|---|---|---|---|---|---|---|---|---|---|---|"]
    keys = ["cue+", "cue-", "ask", "line", "motor", "carry+", "carry-", "hidden", "inhib"]
    for name, _, _, _, _ in WIRE_SHEETS:
        p = [x for x in parsed[name] if x["wiring"]]
        for who in ("carriers", "top"):
            if p and all(who in x["wiring"] for x in p):
                lines.append("| %s | %s | %s |" % (name, who, " | ".join(
                    fmt(mean([x["wiring"][who][k] for x in p]), 2) for k in keys)))
    lines += ["", "The lived sheet's top 64 cells (suite d2 on), the same cells in the other "
              "sheets of the same seed (each sheet's carriers by its own activity):", "",
              "| sheet | cue+ | carry+ | carry- | hidden | motor | inhib | abs carry d' |",
              "|---|---|---|---|---|---|---|---|"]
    acc = {}
    for i, _ in enumerate(opt["seeds"]):
        others = [(n, bases[n][i]) for n in ("suite d2 born", "suite d2 churn", "suite d2 shuffled")]
        for n, v in follow(bases["suite d2 on"][i], others).items():
            acc.setdefault(n, []).append(v)
    for n, vs in acc.items():
        lines.append("| %s | %s |" % (n, " | ".join(fmt(mean([v[k] for v in vs]), 2) for k in (
            "n_cue+", "n_carry+", "n_carry-", "n_hidden", "n_motor", "n_inhib", "|carry|"))))
    return "\n".join(lines)
