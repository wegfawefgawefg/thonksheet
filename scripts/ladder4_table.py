"""Label-free rule families (idea 173): the tables (docs/results-2026-11-26-label-free.md).

Model lives: per task, each arm and control at each checkpoint, the fresh reader (mean, lowest
to highest over seeds), on MNIST the reader on digits moved 1 and 2 pixels, the firing rate
(spikes per 1,000 cell-ticks, all 3,072 cells), share of cell-digits active, born inputs kept,
pixel inputs, pixel likeness and spread (learners). Then each arm against born, churn and its
shuffled life, seed by seed. Then the bounds (backprop on each family's objective), and the
transplants (the lived wiring in the real sheet, `blobmnist ceiling --wiring-load`).
"""

import glob
import json
import os
import re

BOUND_OF = {"f0": "auto (idea 165)", "f1": "real", "f1g": "real", "f2": "pixel", "f3": "looks",
            "f4": "apart", "f5": "bcm"}
# idea 165's autoencoder at the bottom rung, seeds 5-7 (out/from-agents/ladder2/full)
AUTO = {"mnist": [0.9608, 0.9583, 0.9615], "pair": [0.7448, 0.7409, 0.7487]}


def load(outdir, pattern):
    out = []
    for f in sorted(glob.glob(os.path.join(outdir, pattern))):
        try:
            out.append(json.loads(open(f).read()))
        except (ValueError, OSError):
            pass
    return out


def stat(v):
    if not v:
        return "-"
    m = sum(v) / len(v)
    if len(v) == 1:
        return "%.4f" % m
    return "%.4f (%.3f-%.3f)" % (m, min(v), max(v))


def mean(v):
    return sum(v) / len(v) if v else float("nan")


def diff(a, b):
    """paired by seed: mean difference, seeds above, of how many"""
    seeds = sorted(set(a) & set(b))
    if not seeds:
        return "-"
    d = [a[s] - b[s] for s in seeds]
    return "%+.4f %d/%d" % (mean(d), sum(x > 0 for x in d), len(d))


def model_tables(outdir, task, runs):
    lines = []
    born = {r["seed"]: r["reads"]["0"] for r in runs if r["family"] == "born"}
    arms = sorted(set(r["arm"] for r in runs if r["family"] != "born"))
    marks = sorted(set(int(k) for r in runs if r["family"] != "born" for k in r["reads"]))
    head = "%-22s %-8s %-6s %-24s %-8s %-8s %-6s %-6s %-6s %-6s %-6s %-6s"
    lines.append(head % ("arm", "ctrl", "digits", "reader", "moved1", "moved2", "rate", "active",
                         "kept", "pixels", "like", "spread"))

    def row(name, ctrl, n, reads):
        g = lambda k: [r.get(k) for r in reads if r.get(k) is not None]
        f = lambda k, fmt: (fmt % mean(g(k))) if g(k) else "-"
        lines.append(head % (name, ctrl, n, stat(g("reader")), f("moved1", "%.4f"),
                             f("moved2", "%.4f"), f("rate", "%.1f"), f("active", "%.3f"),
                             f("born_kept", "%.3f"), f("pixels", "%.1f"), f("likeness", "%.3f"),
                             f("spread", "%.2f")))
    row("born", "-", 0, list(born.values()))
    for a in arms:
        for ctrl in ("life", "churn", "shuffled"):
            for n in marks:
                reads = [r["reads"][str(n)] for r in runs
                         if r["arm"] == a and r["ctrl"] == ctrl and str(n) in r["reads"]]
                if reads:
                    row(a, ctrl, n, reads)
    lines.append("")
    lines.append("seed by seed (mean difference, seeds above): reader; moved 1 on MNIST")
    lines.append("%-22s %-6s %-14s %-14s %-14s %-14s" % ("arm", "digits", "- born", "- churn",
                                                       "- shuffled", "moved1 - born"))
    for a in arms:
        for n in marks:
            by = {}
            for ctrl in ("life", "churn", "shuffled"):
                by[ctrl] = {r["seed"]: r["reads"][str(n)] for r in runs if r["arm"] == a
                            and r["ctrl"] == ctrl and str(n) in r["reads"]}
            rd = lambda d: {s: v["reader"] for s, v in d.items()}
            mv = lambda d: {s: v["moved1"] for s, v in d.items() if "moved1" in v}
            if not by["life"]:
                continue
            lines.append("%-22s %-6d %-14s %-14s %-14s %-14s" % (
                a, n, diff(rd(by["life"]), rd(born)), diff(rd(by["life"]), rd(by["churn"])),
                diff(rd(by["life"]), rd(by["shuffled"])), diff(mv(by["life"]), mv(born))))
    return lines


def bound_table(task, bounds):
    lines = ["%-18s %-24s %-6s %-6s %-6s %-6s" % ("objective", "reader", "rate", "active",
                                                  "pixels", "spread")]
    lines.append("%-18s %-24s" % ("auto (idea 165)", stat(AUTO[task])))
    for obj in sorted(set(b["objective"] for b in bounds)):
        bs = [b for b in bounds if b["objective"] == obj]
        g = lambda k: mean([b.get(k, float("nan")) for b in bs])
        lines.append("%-18s %-24s %-6.1f %-6.3f %-6.1f %-6.2f" % (
            obj, stat([b["reader"] for b in bs]), g("rate"), g("active"), g("pixels"),
            g("pixel_spread")))
    return lines


def parse_ceiling(path):
    """blobmnist ceiling's output: the reader of the non-sensor cells, moved 1 and 2, rate"""
    out = {}
    for line in open(path):
        if line.startswith("sheet, sensor cells left out"):
            v = line.split()
            out["reader"], out["moved1"], out["moved2"] = float(v[6]), float(v[7]), float(v[8])
        m = re.search(r"([\d.]+) spikes per 1000 cell-ticks", line)
        if m and line.startswith("frozen"):
            out["rate"] = float(m.group(1))
    return out


def transplant_table(outdir, task):
    lines = []
    borns = {}  # by read tag ("" or "@tag")
    for f in glob.glob(os.path.join(outdir, "bornread-%s*-*.txt" % task)):
        name, seed = os.path.basename(f)[len("bornread-%s" % task):-4].rsplit("-", 1)
        r = parse_ceiling(f)
        if "reader" in r:
            borns.setdefault(name, {})[int(seed)] = r
    born = borns.get("", {})
    keys = {}
    files = [(f, "transplant") for f in glob.glob(os.path.join(outdir, "transplant-%s-*.txt" % task))]
    files += [(f, "engine") for f in glob.glob(os.path.join(outdir, "engine-%s-*.txt" % task))]
    for f, kind in files:
        name = os.path.basename(f)[len("%s-%s-" % (kind, task)):-4]
        name = name if kind == "transplant" else "engine " + name
        arm_ctrl, seed = name.rsplit("-", 1)
        r = parse_ceiling(f)
        if "reader" in r:
            keys.setdefault(arm_ctrl, {})[int(seed)] = r
    if not born and not keys:
        return lines
    lines.append("%-26s %-24s %-8s %-8s %-6s %-16s" % ("wiring", "reader", "moved1", "moved2",
                                                     "rate", "- born (seeds)"))

    def row(name, d, ref):
        rs = [d[s]["reader"] for s in sorted(d)]
        g = lambda k: ("%.4f" % mean([d[s][k] for s in d])) if all(k in d[s] for s in d) else "-"
        lines.append("%-26s %-24s %-8s %-8s %-6s %-16s" % (
            name, stat(rs), g("moved1") if task == "mnist" else "-",
            g("moved2") if task == "mnist" else "-", g("rate")[:-2] if g("rate") != "-" else "-",
            diff({s: d[s]["reader"] for s in d}, {s: ref[s]["reader"] for s in ref})
            if ref is not None else ""))
    for tag in sorted(borns):
        row("born" + tag, borns[tag], None)
    for k in sorted(keys):
        tag = "@" + k.split("@", 1)[1] if "@" in k else ""
        row(k, keys[k], borns.get(tag, {}))
    by_arm = {}
    for k in keys:
        arm, ctrl = k.rsplit("-", 1)
        by_arm.setdefault(arm, {})[ctrl] = keys[k]
    for arm, d in sorted(by_arm.items()):
        for ctrl in ("churn", "shuffled"):
            if "life" in d and ctrl in d:
                lines.append("%-26s %s" % (arm + " life - " + ctrl, diff(
                    {s: v["reader"] for s, v in d["life"].items()},
                    {s: v["reader"] for s, v in d[ctrl].items()})))
    return lines


def table(outdir, opt):
    out = ["Label-free rule families in the model (idea 173): fresh linear reader of the 3,072 "
           "cells, test digits; mean (lowest-highest) over seeds"]
    runs = load(outdir, "life-*.json")
    for r in runs:
        r["arm"] = r.get("arm", r["family"])
    bounds = load(outdir, "bound-*.json")
    for task in opt["tasks"]:
        out += ["", task.upper() if task == "mnist" else "PAIR TEST", ""]
        tr = [r for r in runs if r["task"] == task]
        if tr:
            out += model_tables(outdir, task, tr)
        tb = [b for b in bounds if b["task"] == task]
        out += ["", "upper bounds (backprop on the family's objective, no labels, rung 7b)"]
        out += bound_table(task, tb)
        tt = transplant_table(outdir, task)
        if tt:
            out += ["", "written into the real sheet (blobmnist ceiling --wiring-load, settle "
                    "%d); rate: hidden spikes per 1,000 cell-ticks" % opt["settle"]]
            out += tt
    return "\n".join(out)
