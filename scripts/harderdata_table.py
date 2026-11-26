"""A harder real-data test: the tables of what has been run (scripts/harderdata.sh NAME
--table-only). One block per set and test, then the table of the room."""

import glob
import json
import math
import os
import re

import everyevent as EE

TENTH = 0.1  # the difference the room table asks about: a tenth of the room


def mean(v):
    v = [x for x in v if x is not None]
    return sum(v) / len(v) if v else None


def spread(v):
    """standard deviation across seeds (n - 1)"""
    v = [x for x in v if x is not None]
    if len(v) < 2:
        return None
    m = sum(v) / len(v)
    return math.sqrt(sum((x - m) ** 2 for x in v) / (len(v) - 1))


def fmt(v, form="%.4f"):
    return "  -   " if v is None else form % v


def read(path):
    """one blobmnist ceiling run: everyevent's reading plus the bits' own linear reader and
    the share of the life's digits on which the rule was sent an error"""
    d = EE.sheet_read(path)
    if not d:
        return None
    text = open(path).read()
    bits = re.search(r"the bits alone \(same digits\)\s+\d+\s+([0-9.]+)", text)
    verdicts = re.findall(r"verdicts (\d+)", text)
    life = re.search(r"gap \d+\s+life (\d+)", text)
    d["bits"] = float(bits.group(1)) if bits else None
    d["teach"] = (int(verdicts[-1]) / int(life.group(1))
                  if verdicts and life and int(life.group(1)) else None)
    return d


def bars(path):
    """blobmnist baselines: learner -> test score"""
    out = {}
    if os.path.exists(path):
        for line in open(path).read().splitlines():
            m = re.match(r"(\S.*?)\s{2,}([0-9.]+)\s", line)
            if m and not line.startswith("learner"):
                out[m.group(1)] = float(m.group(2))
    return out


def model(path, key="reader"):
    if not os.path.exists(path):
        return None
    return json.load(open(path)).get(key)


def runs_there(outdir, test, seeds):
    """the sheet runs of a test in the order of the table: (name, [reading per seed])"""
    names = ["born", "bornread", "transplant", "bornread-quiet", "transplant-quiet"]
    lived = set()
    for p in glob.glob(os.path.join(outdir, "%s-*-k*-*.txt" % test)):
        m = re.match(r"%s-(.+-k\d+)-\d+\.txt$" % test, os.path.basename(p))
        if m:
            lived.add(m.group(1))
    order = ["compete", "out", "out-coin", "outf3", "outf3-coin", "outint2", "outint2-coin"]
    key = lambda n: (int(n.rsplit("-k", 1)[1]), order.index(n.rsplit("-k", 1)[0])
                     if n.rsplit("-k", 1)[0] in order else 99)
    out = []
    for n in names + sorted(lived, key=key):
        v = [read(os.path.join(outdir, "%s-%s-%d.txt" % (test, n, s))) for s in seeds]
        if any(v):
            out.append((n, v))
    return out


def block(opt, name, outdir, test):
    seeds = opt["seeds"]
    lines = ["", "%s, %s test: a fresh linear reader of the non-sensor cells, test digits" % (
        name, test)]
    b1, b5 = (bars(os.path.join(outdir, "%s-bars-%d.txt" % (test, p))) for p in (1, 5))
    for k, v in b1.items():
        lines.append("  plain learner (on the bits unless it says grey): %-38s %.4f" % (k, v))
    for k, v in b5.items():
        if k.startswith("two-layer") and "grey" not in k:
            lines.append("  plain learner (on the bits unless it says grey): %-38s %.4f" % (k, v))
    for tag, what in (("net", "the unconstrained net (3,072 relu), model"),
                      ("modelborn", "the cells as born, in the model"),
                      ("teacher", "the teacher's cells, in the model")):
        v = [model(os.path.join(outdir, "%s-%s-%d.json" % (test, tag, s))) for s in seeds]
        if any(x is not None for x in v):
            lines.append("  %-40s %s  mean %s" % (what, " ".join(fmt(x) for x in v), fmt(mean(v))))
    lines.append("%-22s %s  %-6s %-6s %-5s %-6s %-5s %-7s %-5s" % (
        "run", " ".join("%-6d" % s for s in seeds), "mean", "sd", "rate", "pixels", "kept",
        "outside", "teach"))
    got = dict(runs_there(outdir, test, seeds))
    for run, v in got.items():
        col = lambda k: mean([x[k] for x in v if x])
        score = [x["score"] if x else None for x in v]
        lines.append("%-22s %s  %s %s %s  %s  %s  %s  %s" % (
            run, " ".join(fmt(x) for x in score), fmt(mean(score)), fmt(spread(score)),
            fmt(col("rate"), "%4.1f"), fmt(col("pixels"), "%4.1f"), fmt(col("kept"), "%4.2f"),
            fmt(col("outside"), "%5.3f"), fmt(col("teach"), "%4.2f")))
    bits = mean([x["bits"] for x in got.get("born", []) if x])
    if bits is not None:
        lines.append("  the bits alone, the tree's linear reader on the same digits: %.4f" % bits)
    for run, v in got.items():
        base = run.split("-k")[0]
        if "-k" not in run or base.endswith("-coin"):
            continue
        tag = run.rsplit("-", 1)[1]
        for other in ("born", "%s-coin-%s" % (base, tag)):
            if other in got:
                d = [a["score"] - b["score"] for a, b in zip(v, got[other]) if a and b]
                if d:
                    lines.append("  %-14s - %-18s %+.4f  (%d of %d seeds above; %+.4f to %+.4f)" % (
                        run, other, sum(d) / len(d), sum(x > 0 for x in d), len(d), min(d), max(d)))
    return lines


def room_row(opt, name, outdir, test):
    """the room between born and the teacher's wiring, the spread across seeds, and how many
    seeds a difference of a tenth of the room needs (twice its standard error)"""
    seeds = opt["seeds"]
    got = dict(runs_there(outdir, test, seeds))
    score = lambda n: [x["score"] if x else None for x in got.get(n, [None] * len(seeds))]
    born, read0, top = score("born"), score("bornread"), score("transplant")
    base = read0 if any(x is not None for x in read0) else born
    gaps = [t - b for t, b in zip(top, base) if t is not None and b is not None]
    if not gaps:
        return None
    room = sum(gaps) / len(gaps)
    # the runs a comparison of rules is between: born and the lived arms of the longest life
    lives = sorted({int(n.rsplit("-k", 1)[1]) for n in got if "-k" in n})
    arms = ["born"] + [n for n in got if lives and n.endswith("-k%d" % lives[-1])
                       and "-coin" not in n]
    var = [spread(score(a)) ** 2 for a in arms if spread(score(a)) is not None]
    one = math.sqrt(sum(var) / len(var)) if var else None
    dvar = []
    for i, a in enumerate(arms):
        for c in arms[i + 1:]:
            d = [x - y for x, y in zip(score(a), score(c)) if x is not None and y is not None]
            if spread(d) is not None:
                dvar.append(spread(d) ** 2)
    pair = math.sqrt(sum(dvar) / len(dvar)) if dvar else None
    need = None
    if pair is not None and room > 0:
        need = max(2, math.ceil(4 * pair * pair / (TENTH * room) ** 2))
    return {"set": name, "test": test, "born": mean(base), "top": mean(top), "room": room,
            "room_lo": min(gaps), "room_hi": max(gaps), "one": one, "pair": pair, "need": need,
            "arms": len(arms), "life": lives[-1] if lives else 0}


def tables(opt, sets):
    lines = []
    for name, outdir in sets:
        for test in opt["tests"]:
            lines += block(opt, name, outdir, test)
    lines += ["", "the room: the teacher's wiring in the real sheet less the born sheet read "
              "the same way (after 3,000 digits of regulation)",
              "%-8s %-6s %-7s %-7s %-7s %-17s %-7s %-7s %-5s %s" % (
                  "set", "test", "born", "teacher", "room", "(lowest-highest)", "sd arm",
                  "sd diff", "seeds", "for a tenth of the room")]
    for name, outdir in sets:
        for test in opt["tests"]:
            r = room_row(opt, name, outdir, test)
            if r:
                lines.append("%-8s %-6s %s  %s  %+.4f (%+.4f to %+.4f) %s  %s  %-5s %s" % (
                    r["set"], r["test"], fmt(r["born"]), fmt(r["top"]), r["room"], r["room_lo"],
                    r["room_hi"], fmt(r["one"]), fmt(r["pair"]),
                    "-" if r["need"] is None else str(r["need"]),
                    "(%d runs a seed, lives of %d,000)" % (r["arms"], r["life"])))
    lines.append("(sd arm: a run's spread across seeds, pooled over born and the lived arms of "
                 "the longest life; sd diff: the spread across seeds of the difference between "
                 "two of those runs on the same seed, pooled over every two; seeds: how many "
                 "make twice the standard error of such a difference a tenth of the room. "
                 "rate: hidden spikes per 1,000 cell-ticks while read; pixels: pixel inputs a "
                 "hidden excitatory cell; kept: born inputs kept; outside: the outside output's "
                 "share right in the last tenth of the life; teach: the share of the life's "
                 "digits that sent an error)")
    return "\n".join(lines)
