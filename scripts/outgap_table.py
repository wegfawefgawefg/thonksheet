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
        m = re.match(r"(\d[a-z]?)\s+(.{40})\s*([\d.]+)(.*)", line)
        if not m:
            continue
        key = m.group(1) + " " * (3 - len(m.group(1))) + m.group(2).strip()
        rest = m.group(4)
        if "offline" in rest:
            rows[key] = [None, None, None, float(m.group(3)), None]
            continue
        vals = [float(m.group(3))] + [float(v) for v in rest.split("quiet")[0].split()]
        quiet = rest.split("quiet")[1].split("ties")[0].split() if "quiet" in rest else []
        rows[key] = vals[:5]
        if quiet:
            rows[key + "  (quiet buttons)"] = [float(q) for q in quiet[:5]]
    return rows


def fmt(v):
    return "   -   " if v is None else "%7.4f" % v


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "out/output-gap"
    per_seed = "--seeds" in sys.argv
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
