"""The credit rules not yet tried: the table of a finished run (scripts/credit.sh NAME
--table-only). The real sheet's number (the wiring written in) is the one to rank by; the
model's own reader stands beside it."""

import glob
import json
import os

import ladder2_table as L2T

NAMES = {
    "born": "born (nothing learned)",
    "T7x": "T7x one hop, random wires (the ladder's code)",
    "hop": "hop  T7x through this bench's loop (check)",
    "hopb": "hopb T7x, then passed back along real wires",
    "P2": "P2   two phases, every spike an event",
    "P2s": "P2s  two phases, the sign of the difference",
    "C2": "C2   code, two runs, the answer's line rose or fell",
    "C2n192": "C2n192 code, 192 cells perturbed a digit",
    "C2n12": "C2n12 code, 12 cells perturbed a digit",
    "C1": "C1   code, one run, good or bad only",
    "pay1": "pay1 the output pays the cells that fired",
    "pay": "pay  payment, passed back once more",
    "payc": "payc payment, passed back to one source (conserved)",
    "tgt": "tgt  a target for each cell from the answer",
    "dtp": "dtp  targets on wires the cell tuned itself",
    "dtpb": "dtpb dtp, then passed back along real wires",
    "d1n": "d1n  error 1 showing late, no trace",
    "d1t2": "d1t2 error 1 late, marks last 2 showings",
    "d1t4": "d1t4 error 1 late, marks last 4 showings",
    "d3t4": "d3t4 error 3 late, marks last 4 showings",
    "d0t4": "d0t4 error on time, marks last 4 showings",
}
ORDER = list(NAMES)


def sort_key(arm):
    name, _, v = arm.partition("@")
    return (ORDER.index(name) if name in ORDER else 99, name, float(v) if v else 0.0)


def load(outdir):
    """model runs and real-sheet reads by (task, arm) and seed"""
    runs, sheet, rate = {}, {}, {}
    for path in glob.glob(os.path.join(outdir, "*.json")):
        task, arm, seed = os.path.basename(path)[:-5].rsplit("-", 2)
        runs.setdefault((task, arm), {})[int(seed)] = json.load(open(path))
    for path in glob.glob(os.path.join(outdir, "transplant-*.txt")):
        task, arm, seed = os.path.basename(path)[11:-4].rsplit("-", 2)
        score = L2T.ceiling_score(path)
        if score is not None:
            sheet.setdefault((task, arm), {})[int(seed)] = score
            rate.setdefault((task, arm), {})[int(seed)] = L2T.ceiling_rate(path)
    for path in glob.glob(os.path.join(outdir, "bornread-*.txt")):
        task, seed = os.path.basename(path)[9:-4].rsplit("-", 1)
        score = L2T.ceiling_score(path)
        if score is not None:
            sheet.setdefault((task, "born"), {})[int(seed)] = score
            rate.setdefault((task, "born"), {})[int(seed)] = L2T.ceiling_rate(path)
    return runs, sheet, rate


def mean(v):
    v = [x for x in v if x is not None]
    return sum(v) / len(v) if v else None


def spread(d, form="%.4f"):
    v = list(d.values())
    if not v:
        return "-"
    if len(v) == 1:
        return form % v[0]
    return (form + " (%s-%s, %d)") % (mean(v), form % min(v), form % max(v), len(v))


def diff(a, b):
    """seed by seed a - b: mean, seeds above"""
    s = sorted(set(a) & set(b))
    if not s:
        return "-"
    d = [a[k] - b[k] for k in s]
    return "%+.4f (%d/%d)" % (mean(d), sum(x > 0 for x in d), len(d))


def field(rs, key, form="%.2f"):
    m = mean([r.get(key) for r in rs.values()])
    return form % m if m is not None else "-"


def table(outdir, opt):
    runs, sheet, rate = load(outdir)
    lines = ["the credit rules: %s digits. sheet: the wiring written into the real sheet, the "
             "tree's reader, mean (lowest-highest, seeds); rate: its hidden spikes per 1,000 "
             "cell-ticks; coin: the same rule taught by another digit's signal, in the sheet; "
             "- coin, - born, - T7x: seed by seed in the sheet (seeds above); model: the "
             "model's own reader (not to rank by); agree: share of teaching bits with "
             "backprop's sign (life / first 4,096 digits); taught: share of cell-digits "
             "given a bit; changed: born inputs a cell no longer wires; px: pixel inputs a "
             "cell" % ("validation" if opt["val"] else "test")]
    for task in opt["tasks"]:
        arms = sorted({a for t, a in list(runs) + list(sheet) if t == task and
                       not a.partition("@")[0].endswith("coin")}, key=sort_key)
        born, hop = sheet.get((task, "born"), {}), sheet.get((task, "T7x"), {})
        lines += ["", "MNIST" if task == "mnist" else "pair test (same class, interleaved)",
                  "%-50s %-28s %-5s %-7s %-15s %-15s %-15s %-7s %-7s %-11s %-6s %-7s %s" % (
                      "arm", "sheet", "rate", "coin", "- coin", "- born", "- T7x", "model",
                      "m.coin", "agree", "taught", "changed", "px")]
        for arm in arms:
            name, _, v = arm.partition("@")
            coin = name + "coin" + ("@" + v if v else "")
            rs, cs = runs.get((task, arm), {}), runs.get((task, coin), {})
            sh, csh = sheet.get((task, arm), {}), sheet.get((task, coin), {})
            lines.append(
                "%-50s %-28s %-5s %-7s %-15s %-15s %-15s %-7s %-7s %-11s %-6s %-7s %s" % (
                    NAMES.get(name, name) + ("@" + v if v else ""), spread(sh),
                    "%.1f" % mean(rate[(task, arm)].values()) if (task, arm) in rate else "-",
                    "%.4f" % mean(csh.values()) if csh else "-", diff(sh, csh),
                    diff(sh, born) if arm != "born" else "-",
                    diff(sh, hop) if arm != "T7x" else "-",
                    field(rs, "reader", "%.4f"), field(cs, "reader", "%.4f"),
                    field(rs, "agree") + "/" + field(rs, "agree_first"),
                    field(rs, "taught", "%.3f"), field(rs, "born_changed", "%.1f"),
                    field(rs, "pixels", "%.1f")))
    return "\n".join(lines)
