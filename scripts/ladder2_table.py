"""The inside's ladder: the table of a finished ladder (scripts/ladder2.sh NAME --table-only)."""

import glob
import json
import os
import re

NAMES = {
    "lin": "0  linear reader on the bits",
    "net": "0  unconstrained net: 3,072 relu, full fan-in",
    "1": "1  count units, full fan-in, float",
    "2": "2  born fan-in, learned top-K of pixels",
    "3a": "3a one-bit strengths, sign free",
    "3b": "3b strengths 1 and 4 at born counts",
    "4": "4  cells hear cells, 2 settle steps",
    "4s3": "4s3 the same, 3 settle steps",
    "5": "5  fixed sign per source",
    "6": "6  within reach (9 tiles, far tiles)",
    "7a": "7a threshold by regulation, engine's rate",
    "7b": "7b the engine's rate curve (bottom)",
}
ORDER = ["lin", "net", "1", "2", "3a", "3b", "4", "4s3", "5", "6", "7a", "7b"]


def load(outdir):
    runs = {}
    for path in glob.glob(os.path.join(outdir, "*.json")):
        r = json.load(open(path))
        runs.setdefault((r["task"], r["rung"], r["kind"]), []).append(r)
    return runs


def stat(rs, key):
    v = [r[key] for r in rs if key in r]
    if not v:
        return "  -  "
    m = sum(v) / len(v)
    if len(v) == 1:
        return "%.4f" % m
    return "%.4f (%.3f-%.3f, %d)" % (m, min(v), max(v), len(v))


def ceiling_score(path):
    if not os.path.exists(path):
        return None
    m = re.search(r"sensor cells left out\s+\d+\s+([0-9.]+)", open(path).read())
    return float(m.group(1)) if m else None


def ceiling_rate(path):
    if not os.path.exists(path):
        return None
    m = re.search(r"frozen, fit digits.*?: ([0-9.]+) spikes per 1000", open(path).read())
    return float(m.group(1)) if m else None


def table(outdir, opt):
    runs = load(outdir)
    tasks = opt["tasks"]
    lines = ["the inside's ladder: %s digits; fresh linear reader of the 3,072 cells (mean, "
             "lowest-highest, seeds); own: the net's own output layer; active: share of "
             "cell-digits with a spike; mean: spikes a digit a cell (born sheet 0.66)" %
             ("validation" if opt["val"] else "test")]
    for task in tasks:
        lines += ["", "%s" % ("MNIST" if task == "mnist" else "pair test (same class, interleaved)"),
                  "%-44s %-28s %-28s %-7s %-6s | born arm %s" % (
                      "rung", "reader", "own", "active", "mean", "reader")]
        for rung in ORDER:
            t = runs.get((task, rung, "trained"), [])
            b = runs.get((task, rung, "born"), [])
            if not t and not b:
                continue
            act = sum(r.get("active", 0) for r in t) / len(t) if t else 0
            mc = sum(r.get("mean_count", 0) for r in t) / len(t) if t else 0
            lines.append("%-44s %-28s %-28s %-7s %-6s | %s" % (
                NAMES[rung], stat(t, "reader"), stat(t, "own"),
                "%.3f" % act if t and rung != "lin" else "", "%.2f" % mc if t and rung != "lin"
                else "", stat(b, "reader") if b else ""))
        a = runs.get((task, "7b", "auto"), [])
        if a:
            lines.append("%-44s %-28s" % ("7b with no labels (autoencoder)", stat(a, "reader")))
        # the bottom rung's cells: what the teacher wired
        t = runs.get((task, "7b", "trained"), [])
        b = runs.get((task, "7b", "born"), [])
        for label, rs in (("bottom rung trained", t), ("bottom rung born", b)):
            if rs:
                keys = ["inputs", "pixels", "strong", "inh_inputs", "pixel_spread", "born_kept",
                        "active", "mean_count", "silent_cells"]
                lines.append("  %s: %s" % (label, ", ".join(
                    "%s %.3f" % (k, sum(r.get(k, 0) for r in rs) / len(rs)) for k in keys)))
        # the transplant: the real sheet with the bottom rung's wiring, the tree's reader
        tp, bp = [], []
        for s in opt["seeds"]:
            x = ceiling_score(os.path.join(outdir, "transplant-%s-%d.txt" % (task, s)))
            y = ceiling_score(os.path.join(outdir, "bornread-%s-%d.txt" % (task, s)))
            if x is not None and y is not None:
                tp.append((s, x, y,
                           ceiling_rate(os.path.join(outdir, "transplant-%s-%d.txt" % (task, s))),
                           ceiling_rate(os.path.join(outdir, "bornread-%s-%d.txt" % (task, s)))))
        if tp:
            lines.append("  transplant (blobmnist ceiling, real sheet, tree's reader), seed: "
                         "bottom rung's wiring / born wiring [hidden spikes per 1000 cell-ticks]")
            for s, x, y, rx, ry in tp:
                lines.append("    seed %d: %.4f / %.4f  [%s / %s]" % (s, x, y, rx, ry))
            lines.append("    mean: %.4f / %.4f" % (sum(v[1] for v in tp) / len(tp),
                                                    sum(v[2] for v in tp) / len(tp)))
        for kind in ("born", "transplant"):
            for path in sorted(glob.glob(os.path.join(outdir, "match-%s-%s-*.txt" % (kind, task)))):
                last = open(path).read().strip().splitlines()[-1]
                if last.startswith("{"):
                    m = json.loads(last)
                    lines.append("  cell by cell, %s wiring, seed %s: %s" % (
                        kind, path.split("-")[-1][:-4],
                        ", ".join("%s %.3f" % (k, v) for k, v in m.items())))
        for passes in (1, 5):
            p = os.path.join(outdir, "bars-%s-%d.txt" % (task, passes))
            if os.path.exists(p):
                for line in open(p).read().splitlines():
                    want = ("linear,", "two-layer, 100", "random") if passes == 1 else (
                        "two-layer, 100",)
                    if line.startswith(want):
                        lines.append("  tree baseline (net passes %d): %s" % (passes,
                                                                          line[:60].rstrip()))
    return "\n".join(lines)
