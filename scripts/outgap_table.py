#!/usr/bin/env python3
"""Tables for scripts/outgap.sh: every finished run under out/output-gap, the mean over seeds
at each check (test accuracy), with the lowest and highest seed at 20,000 digits.
  python3 scripts/outgap_table.py out/output-gap [--seeds]   (--seeds: one line per seed too)"""
import os
import re
import sys

CHECKS = [1000, 5000, 20000, 60000, 120000]


def parse_online(path):
    acc = {}
    for line in open(path):
        m = re.match(r"\s+(\d+)\s+[\d.]+\s+([\d.]+)\s", line)
        if m and int(m.group(1)) in CHECKS:
            acc[int(m.group(1))] = float(m.group(2))
    return {"7  engine": [acc.get(c) for c in CHECKS]} if acc else {}


def parse_ladder(path):
    rows = {}
    for line in open(path):
        # a reader's line: its label in the first 44 columns, then the accuracies
        if not re.match(r"\d[a-z]{0,3} ", line) or len(line) < 50:
            continue
        m = re.search(r"((?:\s+\d\.\d{4}){5})", line) or re.search(r"(\s+\d\.\d{4})\s+\(offline", line)
        if not m:
            continue
        key, rest = line[:m.start()].strip(), line[m.start():]
        if "offline" in rest:
            rows[key] = [None, None, None, float(rest.split()[0]), None]
            continue
        vals = [float(v) for v in m.group(1).split()]
        quiet = rest.split("quiet")[1].split("ties")[0].split() if "quiet" in rest else []
        rows[key] = vals[:5]
        if quiet:
            rows[key + "  (quiet buttons)"] = [float(q) for q in quiet[:5]]
        if "close goods" in rest:
            rows[key + "  (close goods)"] = [None] * 4 + [float(rest.split("close goods")[1])]
    return rows


def fmt(v):
    return "   -   " if v is None else "%7.4f" % v


TOY_ROWS = ["k2", "k4", "k8", "k16", "xor", "order"]


def toys(root):
    """the toy rows: late score (blob, and its learning-off twin) per row, mean over seeds"""
    names = sorted(n for n in os.listdir(root) if n.startswith("toys-"))
    if not names:
        return
    print("toy rows: late score, mean over seeds (learning-off twin in brackets)")
    print("%-24s %s" % ("", " ".join("%15s" % r for r in TOY_ROWS)))
    for name in names:
        d = os.path.join(root, name)
        cols, seeds = [], set()
        for row in TOY_ROWS:
            blob, off = [], []
            for f in os.listdir(d):
                m = re.match(r"%s-seed(\d+)-sum\.csv$" % row, f)
                if not m:
                    continue
                seeds.add(int(m.group(1)))
                for line in open(os.path.join(d, f)):
                    p = line.strip().split(",")
                    if p[1:2] == ["late"]:
                        (blob if p[0] == "blob" else off if p[0] == "learn_off" else []).append(
                            float(p[2]))
            cols.append("%7.3f (%5.3f)" % (sum(blob) / len(blob), sum(off) / len(off))
                        if blob else "%15s" % "-")
        print("%-24s %s   seeds %s" % (name[5:], " ".join(cols), ",".join(map(str, sorted(seeds)))))
    print()


def load(d, parse):
    runs = {}
    if not os.path.isdir(d):
        return runs
    for f in sorted(os.listdir(d)):
        m = re.match(r"seed(\d+)\.txt$", f)
        if m and "total wall" in open(os.path.join(d, f)).read():
            runs[int(m.group(1))] = parse(os.path.join(d, f))
    return runs


def mean_at(runs, key, i):
    vals = [r[key][i] for r in runs.values() if key in r and r[key][i] is not None]
    return (sum(vals) / len(vals), min(vals), max(vals), len(vals)) if vals else None


def doc(root):
    """the ladder as a markdown table: born and lived side by side, means over seeds at
    20,000, 60,000 and 120,000 (rung 1: offline, one number); the engine rows after"""
    born = load(os.path.join(root, "ladder-born"), parse_ladder)
    lived = load(os.path.join(root, "ladder-lived"), parse_ladder)
    engine = [("7  engine, plain", "online-born", "online-lived"),
              ("7  engine, gate", "online-gate", None),
              ("7  engine, runner-up + gate", "online-rival", "online-livedrival")]
    print("born seeds %s; lived seeds %s" % (sorted(born), sorted(lived)))
    print("| rung | born 20k | 60k | 120k | lived 20k | 60k | 120k |")
    print("|---|---|---|---|---|---|---|")
    keys = []
    for r in list(born.values()) + list(lived.values()):
        keys += [k for k in r if k not in keys and "  (" not in k]

    def cells(runs, key):
        out = []
        for i in (2, 3, 4):
            m = mean_at(runs, key, i)
            out.append("%.3f" % m[0] if m else "")
        if key.startswith("1 "):
            m = mean_at(runs, key, 3)
            out = ["%.3f (offline)" % m[0] if m else "", "", ""]
        return out

    for k in keys:
        print("| %s | %s | %s |" % (k, " | ".join(cells(born, k)), " | ".join(cells(lived, k))))
    for label, b, l in engine:
        rb = load(os.path.join(root, b), parse_online)
        rl = load(os.path.join(root, l), parse_online) if l else {}
        key = "7  engine"
        print("| %s | %s | %s |" % (label, " | ".join(cells(rb, key)), " | ".join(cells(rl, key))))
    print()
    print("seed spread (lowest-highest) at 20,000, born | lived:")
    for k in keys:
        sb, sl = mean_at(born, k, 2), mean_at(lived, k, 2)
        print("  %-48s %s | %s" % (k, "%.3f-%.3f" % sb[1:3] if sb else "-",
                                    "%.3f-%.3f" % sl[1:3] if sl else "-"))


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "out/output-gap"
    if "--doc" in sys.argv:
        doc(root)
        return
    per_seed = "--seeds" in sys.argv
    toys(root)
    for name in sorted(os.listdir(root)):
        d = os.path.join(root, name)
        if not os.path.isdir(d) or not (name.startswith("online-") or name.startswith("ladder-")):
            continue
        runs = {}
        for f in sorted(os.listdir(d)):
            m = re.match(r"seed(\d+)\.txt$", f)
            if not m:
                continue
            path = os.path.join(d, f)
            if "total wall" not in open(path).read():
                continue
            parse = parse_online if name.startswith("online-") else parse_ladder
            runs[int(m.group(1))] = parse(path)
        if not runs:
            continue
        print("%s  (seeds %s)" % (name, ",".join(str(s) for s in sorted(runs))))
        print("%-52s %7s %7s %7s %7s %7s   %s" % ("rung", *CHECKS, "20k low-high"))
        keys = []
        for r in runs.values():
            keys += [k for k in r if k not in keys]
        for k in keys:
            cols = []
            for i in range(5):
                vals = [r[k][i] for r in runs.values() if k in r and r[k][i] is not None]
                cols.append(sum(vals) / len(vals) if vals else None)
            v20 = [r[k][2] for r in runs.values() if k in r and r[k][2] is not None]
            span = "%.3f-%.3f" % (min(v20), max(v20)) if v20 and "quiet" not in k else ""
            print("%-52s %s   %s" % (k, " ".join(fmt(c) for c in cols), span))
            if per_seed and "quiet" not in k:
                for s in sorted(runs):
                    if k in runs[s]:
                        print("%-52s %s" % ("    seed %d" % s, " ".join(fmt(c) for c in runs[s][k])))
        print()


if __name__ == "__main__":
    main()
