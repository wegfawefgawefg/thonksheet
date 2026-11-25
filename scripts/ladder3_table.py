"""The rule's ladder: the tables of a finished run (scripts/ladder3.sh NAME --table-only)."""

import glob
import json
import os

import ladder2_table as L2T

NAMES = {
    "born": "born arm (7b, the real born wiring)",
    "T0": "T0  idea 165: 12 passes, Adam, batches",
    "T0b": "T0b the same from the born wiring",
    "T1a": "T1a one pass of 60,000",
    "T1b": "T1b 20,000 digits",
    "T2": "T2  one digit at a time, plain steps",
    "T2h": "T2h the error stops at the cell",
    "T3": "T3  fixed random feedback, dense",
    "T4": "T4  a few +-1 wires a cell",
    "T5": "T5  each line's error a sign",
    "T6f": "T6f no derivative: fired",
    "T6n": "T6n no derivative: near, not fired",
    "T6": "T6  no derivative: near or fired",
    "T6b": "T6b inputs as bits (on or not)",
    "T6s": "T6s the cell told only more or less",
    "T7": "T7  counter steps on inputs that were on",
    "T7w": "T7w counters of -127..127",
    "T7m": "T7m counters of 9 bits (-255..255)",
    "T7k": "T7k counters of 12 bits",
    "T7x": "T7x counters of 16 bits",
    "T7t": "T7t counter steps, a chance per tick on",
    "T8t": "T8t T7t from the answer, every step",
    "T10t": "T10t T8t, only the engine's learners",
    "T8x": "T8x T7x from the answer, every step",
    "T10x": "T10x T8x, only the engine's learners",
    "T8v": "T8v told good or bad (pressed)",
    "T8r": "T8r told the right button on bad",
    "T8": "T8  from the answer, every step",
    "T10": "T10 only the engine's learners",
    "S5": "S5  T2, each line's error a sign",
    "S6": "S6  T2, no derivative: near or fired",
    "S6f": "S6f T2, no derivative: fired",
    "S7": "S7  T2, counter steps",
    "S8": "S8  T2, from the answer",
    "S10": "S10 T2, only the engine's learners",
    "A0": "A0  T0 written out by hand",
    "A2h": "A2h T0, the error stops at the cell",
    "A3": "A3  T0, random feedback (one hop)",
    "A5": "A5  T0, each line's error a sign",
    "A6": "A6  T0, no derivative: fired",
    "A8": "A8  T0, from the answer",
}
ORDER = ["born", "T0", "T0b", "T1a", "T1b", "T2", "T2h", "T3", "T4", "T5", "T6f", "T6n", "T6",
         "T6b", "T6s", "T7", "T7w", "T7m", "T7k", "T7x", "T7t", "T8v", "T8r", "T8", "T8t",
         "T8x", "T10", "T10t", "T10x", "S5", "S6", "S6f", "S7", "S8", "S10", "A0", "A2h", "A3",
         "A5", "A6", "A8"]


def load(outdir):
    runs = {}
    for path in glob.glob(os.path.join(outdir, "*.json")):
        task, arm, seed = os.path.basename(path)[:-5].rsplit("-", 2)[-3:]
        r = json.load(open(path))
        runs.setdefault((task, arm), {})[int(seed)] = r
    return runs


def mean(v):
    return sum(v) / len(v) if v else None


def stat(rs, key, form="%.4f"):
    v = [r[key] for r in rs.values() if r.get(key) is not None]
    if not v:
        return "-"
    if len(v) == 1:
        return form % v[0]
    return (form + " (%s-%s, %d)") % (mean(v), form % min(v), form % max(v), len(v))


def diff(a, b):
    """seed by seed a - b on the reader: mean, seeds above"""
    s = sorted(set(a) & set(b))
    if not s:
        return "-"
    d = [a[k]["reader"] - b[k]["reader"] for k in s]
    return "%+.4f (%d of %d)" % (mean(d), sum(x > 0 for x in d), len(d))


def sort_key(arm):
    name, _, v = arm.partition("@")
    base = name[:-4] if name.endswith("coin") else name
    return (ORDER.index(base) if base in ORDER else 99, float(v) if v else 0.0)


def table(outdir, opt):
    runs = load(outdir)
    lines = ["the rule's ladder: %s digits; fresh linear reader of the 3,072 cells, mean "
             "(lowest-highest, seeds); coin: the same rung, the cells given another digit's "
             "errors; changed: born inputs a cell no longer wires (of about 33); pixels: pixel "
             "inputs a cell; active: share of cell-digits with a spike" %
             ("validation" if opt["val"] else "test")]
    for task in opt["tasks"]:
        arms = sorted({a for t, a in runs if t == task and not a.endswith("coin") and
                       "coin@" not in a}, key=sort_key)
        born = runs.get((task, "born"), {})
        lines += ["", "MNIST" if task == "mnist" else "pair test (same class, interleaved)",
                  "%-40s %-26s %-26s %-17s %-17s %-6s %-6s %-6s %-6s %s" % (
                      "arm", "reader", "coin", "minus coin", "minus born", "own", "changed",
                      "pixels", "active", "step")]
        for arm in arms:
            rs = runs[(task, arm)]
            name, _, v = arm.partition("@")
            coin = runs.get((task, name + "coin" + ("@" + v if v else "")), {})
            r0 = next(iter(rs.values()))
            step = r0.get("lr") if r0.get("lr") is not None else (
                "2^-%d" % r0["shift"] if r0.get("shift") is not None else "")
            lines.append("%-40s %-26s %-26s %-17s %-17s %-6s %-6s %-6s %-6s %s" % (
                NAMES.get(name, name) + ("@" + v if v else ""), stat(rs, "reader"),
                stat(coin, "reader") if coin else "", diff(rs, coin) if coin else "",
                diff(rs, born) if born and arm != "born" else "",
                stat(rs, "own", "%.3f").split(" ")[0],
                stat(rs, "born_changed", "%.1f").split(" ")[0],
                stat(rs, "pixels", "%.1f").split(" ")[0], stat(rs, "active", "%.3f").split(" ")[0],
                step))
        # the real sheet: transplants and the engine's own rule
        tps = sorted(glob.glob(os.path.join(outdir, "transplant-%s-*.txt" % task)))
        by_arm = {}
        for p in tps:
            arm, seed = os.path.basename(p)[:-4].split("-")[2:4]
            by_arm.setdefault(arm, []).append(int(seed))
        for arm, seeds in sorted(by_arm.items(), key=lambda kv: sort_key(kv[0])):
            lines.append("  real sheet, %s's wiring (blobmnist ceiling --wiring-load), seed: "
                         "transplant / born [hidden spikes per 1000 cell-ticks]" % arm)
            got = []
            for s in sorted(seeds):
                a = os.path.join(outdir, "transplant-%s-%s-%d.txt" % (task, arm, s))
                b = os.path.join(outdir, "bornread-%s-%d.txt" % (task, s))
                x, y = L2T.ceiling_score(a), L2T.ceiling_score(b)
                if x is not None and y is not None:
                    got.append((x, y))
                    lines.append("    seed %d: %.4f / %.4f  [%s / %s]" % (
                        s, x, y, L2T.ceiling_rate(a), L2T.ceiling_rate(b)))
            if got:
                d = [x - y for x, y in got]
                lines.append("    mean: %.4f / %.4f  (%+.4f, %d of %d above born)" % (
                    mean([g[0] for g in got]), mean([g[1] for g in got]), mean(d),
                    sum(v > 0 for v in d), len(d)))
        import hop2
        for arm in ("engine", "enginecoin"):
            for p in sorted(glob.glob(os.path.join(outdir, "%s-%s-*.txt" % (arm, task)))):
                r = hop2.mnist_read(p)
                if r:
                    lines.append("  the engine's one-hop rule from the answer, real sheet, %s "
                                 "seed %s: %.4f [rate %s, pixels %s, kept %s]" % (
                                     "coin" if "coin" in arm else "rule", p[:-4].split("-")[-1],
                                     r["score"], r["rate"], r["pixels"], r["kept"]))
    return "\n".join(lines)
