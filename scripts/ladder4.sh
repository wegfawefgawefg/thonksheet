#!/usr/bin/env bash
# Label-free rule families, tried in the model before the engine (idea 173,
# docs/results-2026-11-26-label-free.md). Idea 165's model of the sheet's 3,072 non-sensor
# cells at its bottom rung (scripts/ladder2*.py: the engine's cell, reach, fixed signs, 1 and 4
# at born counts, the engine's regulation) lives one pass of digits under a local rule with no
# labels: a counter per candidate input per cell, integer steps, the cell wired to its top
# counters at its born counts. Families: f0 competitive learning (the engine's, calibration),
# f1 "is this real" against made-up digits (f1g gated by the tile), f2 predict a pixel just
# outside the cell's inputs, f3 two looks at a digit agree, f4 push apart in the tile, f5 a
# sliding threshold (BCM). Each with its controls: churn (as many steps on random inputs) and
# a life on digits shuffled afresh per digit. Then a fresh linear reader on MNIST (and moved 1
# and 2 pixels) and the pair test, beside the born sheet; each family's objective trained by
# backprop as its upper bound (a measuring stick only); and lived wirings written into the
# real sheet (blobmnist ceiling --wiring-load).
#
#   scripts/ladder4.sh NAME                        every family, seeds 5,6,7, both tasks, the model
#   scripts/ladder4.sh NAME --quick                one seed, 2,000-digit lives, a small fit (smoke)
#   scripts/ladder4.sh NAME --parts bound --jobs 2 the upper bounds
#   scripts/ladder4.sh NAME --parts transplant --transplant-arms f0,f3 --seeds 5-12
#   scripts/ladder4.sh NAME --arms f0.cap4,f2.rate0.25 --ctrls life   settings after dots
#   scripts/ladder4.sh NAME --table-only           print what is there
# Options: --seeds --tasks mnist,pair --arms --ctrls life,churn,shuffled --parts
#   model,bound,transplant --jobs 3 --life 60000 --checkpoints 20000,60000 --epochs 6
#   --objectives real,pixel,looks,apart,bcm --transplant-arms --settle 3000.
# Results in out/ladder4/NAME/ (table.txt; one json per run; wirings gzipped). A finished run
# is kept, so running a line again only runs what is missing. Needs torch: set LADDER_VENV to
# a venv that has it (default out/ladder2/venv, made by scripts/ladder2.sh). One thread a job.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || [ -n "${THONK_MNIST:-}" ] || scripts/get_mnist.sh
venv=${LADDER_VENV:-out/ladder2/venv}
if [ ! -x "$venv/bin/python" ]; then
    echo "no torch venv at $venv: run scripts/ladder2.sh once, or set LADDER_VENV" >&2
    exit 1
fi
scripts/build.sh --target blobmnist >/dev/null
export OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
exec nice -n 19 "$venv/bin/python" scripts/ladder4.py "$@"
