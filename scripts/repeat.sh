#!/usr/bin/env bash
# Why the sheet does not answer the same digit the same way twice (idea 168,
# docs/results-2026-11-26-repeatable.md). Each arm changes one thing in the sheet; each run
# is `blobmnist ceiling --twice 1` on the born sheet and on idea 165's transplanted wiring,
# MNIST and the pair test: a cell's counts against themselves from another starting state,
# the inside-learning test's reader, the firing rate and share of cell-digits with a spike.
# Results in out/repeat/NAME/ (one file a run, kept: a line run again only runs what is
# missing), the table in out/repeat/NAME/table.txt.
#   scripts/repeat.sh arms                          every arm, seed 5 (about 150 runs)
#   scripts/repeat.sh arms --arms base,freeze --seeds 5-12      some arms, more seeds
#   scripts/repeat.sh arms --tasks pair --kinds born            one test, the born sheet only
#   scripts/repeat.sh arms --table-only             the table from what has run
#   scripts/repeat.sh smoke --quick                 two arms on few digits, end to end
#   LADDER_VENV=... scripts/repeat.sh wide arms     the wide cells' transplant wiring (torch,
#                                                   idea 165's model at its bottom rung)
#   LADDER_VENV=... scripts/repeat.sh wiring arms --seeds 8-12   the standard transplant
#                                                   wiring, for seeds whose file was not kept
#   scripts/repeat.sh check arms --arms base --kinds born   idea 165's own measure of an arm:
#                                                   two whole runs, rows written and compared
#   scripts/repeat.sh time                          the time rows with each change on the best
#                                                   setting (run first with scripts/oneset.sh,
#                                                   lines in the results doc)
#   JOBS=2 scripts/repeat.sh arms                   runs at once (default 3)
# The transplant wirings are read from out/repeat/NAME/, out/repeat/wiring/ or REPEAT_WIRING
# (default: the main checkout's out/from-agents/ladder2/full); MNIST from data/mnist or
# THONK_MNIST.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -x build/blobmnist ] || scripts/build.sh --target blobmnist
[ -d data/mnist ] || [ -n "${THONK_MNIST:-}" ] || echo "no data/mnist: scripts/get_mnist.sh" >&2
[ $# -ge 1 ] || { sed -n '2,20p' "$0"; exit 1; }
exec python3 scripts/repeat.py "$@" --jobs "${JOBS:-3}"
