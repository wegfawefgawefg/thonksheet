#!/usr/bin/env python3
"""Idea 183, part 4: MNIST as a world with something to predict in time. See
scripts/predevent.sh (the `moved` line). Each digit of the life is shown for 3 steps, a
pixel further each step; a predictor outside the sheet sends its next-frame errors to the
every-event one-hop rule; then the usual frozen read on the usual labels."""

import os
import re
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import inside  # noqa: E402  the MNIST sheet and its reader
import hop2  # noqa: E402  one run, reading a run
import predhop  # noqa: E402  seed lists

MOVED = "--g.learn_mode 5 --life-moved 3"
EV = " --g.hop_on 1 --g.hop_bits 12 --g.hop_shift 0"
# arm -> life flags (None: the sheet as born)
ARMS = {
    "born": None,
    "off": MOVED,                                   # a life on moved digits, the rule off
    "moved": MOVED + EV,                            # next-frame errors teach the cells
    "movedc": MOVED + EV + " --g.hop_coin 3",       # its coin: another step's errors
    "moveds": MOVED + EV + " --life-shuffle 1",     # pixels shuffled afresh every step
    "movedm1": MOVED + EV + " --moved-margin 1",
    "movedm4": MOVED + EV + " --moved-margin 4",
}


def main():
    argv = sys.argv[1:]
    opt = {"name": argv[0], "seeds": [1, 2], "jobs": 3, "life": 20000, "fit": 60000,
           "arms": ["born", "off", "moved", "movedc"], "val": True, "quick": False,
           "table_only": False}
    i = 1
    while i < len(argv):
        a, val = argv[i], argv[i + 1] if i + 1 < len(argv) else None
        if a == "--quick":
            opt.update(seeds=[1], life=600, fit=3000, quick=True)
        elif a == "--table-only":
            opt["table_only"] = True
        elif a == "--test":
            opt["val"] = False
        elif a == "--seeds":
            opt["seeds"] = predhop.seeds_of(val)
            i += 1
        elif a in ("--jobs", "--life", "--fit"):
            opt[a[2:]] = int(val)
            i += 1
        elif a == "--arms":
            opt["arms"] = val.split(",")
            i += 1
        i += 1
    folder = os.path.join(ROOT, "out", "predevent", opt["name"])
    os.makedirs(folder, exist_ok=True)
    jobs = []
    for s in opt["seeds"]:
        for arm in opt["arms"]:
            cmd = ["nice", "-n", "19", inside.MNIST, "ceiling"] + inside.SHEET + inside.READER + [
                "--fit", str(opt["fit"]), "--seed", str(s)]
            if ARMS[arm] is not None:
                cmd += ["--life", str(opt["life"])] + ARMS[arm].split()
            cmd += ["--val", "1"] if opt["val"] else []
            cmd += ["--test-count", "2000"] if opt["quick"] else []
            jobs.append((cmd, os.path.join(folder, "%s-%d.txt" % (arm, s))))
    if not opt["table_only"]:
        with ThreadPoolExecutor(max_workers=opt["jobs"]) as pool:
            list(pool.map(hop2.run, jobs))
    lines = ["predmoved %s: a fresh linear reader of the non-sensor cells on the usual labels "
             "(%s), after a life of %d digits shown 3 steps each" % (
                 opt["name"], "validation" if opt["val"] else "test digits", opt["life"]),
             "%-9s %s  %-7s %-5s %-6s %-5s %-9s %-7s" % (
                 "arm", " ".join("%-6d" % s for s in opt["seeds"]), "mean", "rate", "pixels",
                 "kept", "predictor", "taught")]
    for arm in opt["arms"]:
        rows, pred, sent = [], [], []
        for s in opt["seeds"]:
            path = os.path.join(folder, "%s-%d.txt" % (arm, s))
            rows.append(hop2.mnist_read(path))
            text = open(path).read() if os.path.exists(path) else ""
            m = re.findall(r"next-frame predictor: right [0-9.]+ balanced ([0-9.]+), pixels "
                           r"that taught ([0-9.]+)", text)
            pred.append(float(m[-1][0]) if m else None)
            sent.append(float(m[-1][1]) if m else None)
        avg = lambda k: hop2.mean([r[k] for r in rows if r])  # noqa: E731
        lines.append("%-9s %s  %s  %s  %s  %s  %s      %s" % (
            arm, " ".join(hop2.fmt(r["score"]) if r else "  -   " for r in rows),
            hop2.fmt(avg("score")), hop2.fmt(avg("rate"), "%4.1f"),
            hop2.fmt(avg("pixels"), "%4.1f"), hop2.fmt(avg("kept"), "%4.2f"),
            hop2.fmt(hop2.mean(pred), "%5.3f"), hop2.fmt(hop2.mean(sent), "%5.3f")))
    lines.append("(rate: hidden spikes per 1,000 cell-ticks while read; pixels: pixel inputs a "
                 "hidden excitatory cell; kept: born inputs kept; predictor: the outside "
                 "next-frame predictor's balanced share right in the last tenth of the life; "
                 "taught: the share of pixel-steps that sent an error)")
    text = "\n".join(lines)
    print(text)
    open(os.path.join(folder, "table.txt"), "w").write(text + "\n")


if __name__ == "__main__":
    main()
