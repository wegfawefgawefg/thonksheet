"""The numbers behind scripts/figures.py, each block with the table it was copied from.
Pairs of numbers are (MNIST, pair test)."""

# docs/results-2026-11-25-rule-ladder.md, "The ladder (test digits, seeds 5, 6, 7)": the
# "reader" and "coin" columns of the MNIST and the pair test tables. A fresh linear reader in
# the offline model of the sheet's cells. The side arms (T0b, T1a, T1b, T6f, T6n, T7w, T7m,
# T7t) and the rungs under T7 (T8, T10, which change what the error is, not how it is cut
# down) are left out.
LADDER_BORN = (0.9604, 0.7643)  # "born arm"
LADDER = [
    {"name": "T0  backprop teacher: 12 passes, batches, Adam", "v": (0.9795, 0.8599)},
    {"name": "T2  one pass, one digit at a time, plain steps", "v": (0.9786, 0.8427)},
    {"name": "T2h  the error stops at the cell", "v": (0.9778, 0.8370)},
    {"name": "T3  fixed random feedback wires", "v": (0.9729, 0.8424),
     "coin": (0.9651, 0.7741)},
    {"name": "T4  a few +1 or -1 wires a cell", "v": (0.9723, 0.8371),
     "coin": (0.9648, 0.7726)},
    {"name": "T5  each line's error a sign", "v": (0.9720, 0.8275), "coin": (0.9523, 0.7758)},
    {"name": "T6  no derivative: near the bar or fired", "v": (0.9731, 0.8234),
     "coin": (0.9503, 0.7601)},
    {"name": "T6b  inputs as bits", "v": (0.9714, 0.8114), "coin": (0.9363, 0.7637)},
    {"name": "T6s  the cell told only more or less", "v": (0.9712, 0.8098),
     "coin": (0.9506, 0.7618)},
    {"name": "T7  one count with a small chance, 6-bit counter", "v": (0.9661, 0.7885),
     "coin": (0.9587, 0.7682)},
    {"name": "T7x  in its place: every marked input counts, 12 bits", "v": (0.9697, 0.8118),
     "coin": (0.9471, 0.7620)},
]

# The real sheet after a 60,000-digit life, fresh linear reader, test digits, seeds 5-12.
# born, the closed loop (arm intope), its coin and its shuffled-digit life: the "mean" column
# of out/from-agents/outputinside/report/table.txt (the first table of
# docs/results-2026-11-26-output-inside.md, "Short version").
# the second setting (intopeallw10) and its coin: out/from-agents/outputinside/report/
# table-report2.txt, the same doc table.
# competitive learning: docs/findings.md, "Where it stands", idea 167's block (0.952 / 0.726;
# MNIST 0.9523 in docs/results-2026-11-25-compete.md, row B).
# the old one-hop rule: docs/results-2026-11-25-hop-real.md, "Point 3" (MNIST 0.9377, 8
# seeds; pairs 0.698, 4 seeds).
# the backprop teacher's wiring written in: docs/results-2026-11-25-inside-ladder.md, "real
# sheet, 7b's wiring (8 seeds)" (0.9663 / 0.7853).
HEADLINE_BORN = (0.9251, 0.6774)
HEADLINE = [
    {"name": "competitive learning (no labels, no reward)", "v": (0.9523, 0.726)},
    {"name": "the old one-hop rule (error from the answer)", "v": (0.9377, 0.698)},
    {"name": "the closed loop inside the blob,\nsetting fixed before the test digits",
     "v": (0.9559, 0.7594), "coin": (0.9375, 0.6798), "shuffled": (0.9256, 0.6698)},
    {"name": "the closed loop inside the blob,\nsetting validation picked afterwards",
     "v": (0.9613, 0.7757), "coin": (0.9433, 0.6801)},
    {"name": "the backprop teacher's wiring, written in", "v": (0.9663, 0.7853)},
]

# The blob's answer's share right in each 6,000 digits of a 60,000-digit MNIST life: the
# "readout right" number of every "life N" line of the archived life logs, as mean, lowest
# seed, highest seed per window.
# closed loop and output alone: out/from-agents/outputinside/report/mnist-intope-{5..12}.txt
# and mnist-alonee-{5..12}.txt (the last window is the "own" column of that report's table:
# 0.925 and 0.906).
# old readout: out/from-agents/everyevent/report/mnist-old-{5,6,7}.txt, the old one-hop rule's
# life under the engine's readout (the "life" column of table-online.txt: 0.680).
LIFE = [
    ("the closed loop:\nthe blob's own output",
     [0.806, 0.883, 0.896, 0.906, 0.913, 0.915, 0.919, 0.921, 0.923, 0.925],
     [0.800, 0.877, 0.891, 0.901, 0.907, 0.907, 0.913, 0.917, 0.917, 0.923],
     [0.814, 0.888, 0.902, 0.913, 0.921, 0.920, 0.923, 0.928, 0.928, 0.929]),
    ("the same output on a\nsheet that does not learn",
     [0.796, 0.874, 0.883, 0.892, 0.896, 0.897, 0.899, 0.903, 0.904, 0.906],
     [0.787, 0.865, 0.875, 0.888, 0.892, 0.888, 0.894, 0.899, 0.898, 0.897],
     [0.801, 0.881, 0.889, 0.895, 0.901, 0.904, 0.902, 0.907, 0.909, 0.913]),
    ("the old readout\n(under the old one-hop rule)",
     [0.615, 0.694, 0.684, 0.697, 0.689, 0.687, 0.690, 0.695, 0.681, 0.680],
     [0.588, 0.651, 0.641, 0.667, 0.657, 0.663, 0.663, 0.676, 0.662, 0.660],
     [0.632, 0.731, 0.730, 0.733, 0.731, 0.725, 0.722, 0.726, 0.714, 0.701]),
]

# The quick map, seeds 5-12, late score, * passes. Columns base, best, gru, memory, vm:
# docs/results-2026-11-26-one-setting.md, the table under `scripts/oneset.sh best`. Columns
# a2rucomp and a2bestru: docs/results-2026-11-26-one-setting-2.md, "The table". Row names:
# the first doc's "The quick map" table. Not drawn: the critic (6 rows), the runner-up rule
# alone (9), competitive learning alone (11), the one setting with competitive learning (15).
MAP_ROWS = [
    "echo, 2 cues", "echo, 8 cues", "echo, 16 cues", "shuffled echo (remapped)",
    "reach: cue 3 tiles away", "reach: cue 5 tiles away", "exclusive-or", "parity of 3",
    "order", "gap", "delayed echo, 2 steps", "hold about 1,000 ticks", "bandit",
    "two-step chain", "cartpole (steps per fall)", "catch", "noisy lines", "copy of 2",
    "held-out scale", "held-out place", "keeping pairs", "MNIST online, 20,000 digits",
]
# (name, is it the sheet, the scores in row order, rows passed as the doc counts them)
MAP_COLUMNS = [
    ("the output\nrule\nalone", True,
     "1.000* 0.801* 0.266 1.000* 0.847* 0.576 0.882* 0.561 0.676 0.509 0.544 0.506 0.773* "
     "0.306 17 0.298 0.268 0.638 0.699 0.487 0.273 0.214".split(), 6),
    ("runner-up rule\n+ competitive\nlearning", True,
     "1.000* 0.925* 0.620* 1.000* 0.925* 0.841* 0.997* 0.831* 0.733 0.938* 0.919* 0.508 "
     "1.000* 0.000 13 0.366 0.218 0.927* 0.719 0.536 0.358 0.838*".split(), 13),
    ("the\none\nsetting", True,
     "1.000* 0.976* 0.955* 1.000* 0.975* 0.933* 0.936* 0.642 0.994* 0.897* 1.000* 0.998* "
     "0.995* 0.150 17 0.671* 0.307 0.897* 0.602 0.471 0.525 0.714*".split(), 15),
    ("one setting\nwith the\nrunner-up rule", True,
     "1.000* 0.992* 0.952* 1.000* 0.975* 0.933* 0.936* 0.642 0.994* 0.897* 1.000* 0.998* "
     "0.995* 0.150 17 0.664* 0.215 0.897* 0.602 0.471 0.460 0.841*".split(), 15),
    ("GRU", False,
     "1.000* 0.998* 0.992* 1.000* 1.000* 1.000* 0.988* 0.518 0.998* 0.994* 0.999* 0.495 "
     "1.000* 0.010 703* 0.723* 0.666* 0.998* 0.569 0.388 0.338 0.733*".split(), 16),
    ("memory\nengine", False,
     "0.993* 0.984* 0.982* 0.993* 0.993* 0.993* 0.993* 0.994* 0.993* 0.995* 0.993* 0.998* "
     "0.988* 0.000 298 0.991* 0.598 0.837* 0.470 0.347 0.977* 0.158".split(), 16),
    ("vm\nengine", False,
     "1.000* 1.000* 1.000* 1.000* 1.000* 1.000* 0.951* 0.829* 0.742* 0.476 1.000* 1.000* "
     "0.767* 0.000 32 0.523 0.394 0.964* 0.514 0.256 0.746* 0.118".split(), 14),
]
