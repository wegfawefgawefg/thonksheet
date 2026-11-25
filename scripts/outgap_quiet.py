#!/usr/bin/env python3
"""Buttons that went quiet in scripts/outgap.sh online runs: per run, at each check, how many
of the ten buttons were pressed on fewer than 1% of the test images, per seed.
  python3 scripts/outgap_quiet.py out/output-gap/online-gate [more run folders]"""
import os
import re
import sys

for d in sys.argv[1:]:
    rows = {}
    for f in sorted(os.listdir(d)):
        m = re.match(r"seed(\d+)\.txt$", f)
        if not m:
            continue
        for line in open(os.path.join(d, f)):
            p = re.match(r"\s+(\d+)\s.*\| test(.*)", line)
            if not p:
                continue
            presses = [int(x) for x in p.group(2).split()][:10]
            total = sum(presses) + 1
            quiet = sum(1 for x in presses if x * 100 < total)
            rows.setdefault(int(p.group(1)), []).append(quiet)
    print(d)
    for at in sorted(rows):
        print("  %7d  quiet buttons per seed: %s" % (at, " ".join(map(str, rows[at]))))
