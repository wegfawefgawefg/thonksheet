#!/usr/bin/env bash
# The inside's ladder (idea 165, docs/results-2026-11-25-inside-ladder.md): what can the
# sheet's cells represent under the best possible teacher? An offline model of the 3,072
# non-sensor cells of scripts/mnist.sh's sheet, trained by backprop (a measuring stick, not
# the method), takes on the cells' constraints one rung at a time: fan-in, top-counter wiring,
# strengths 1 and 4, cells hearing cells, a fixed sign per source, reach, the engine's rate.
# Each rung is read by a fresh linear reader of the cells, on MNIST and on the pair test
# (same class or not, interleaved), beside the same rung with a random (or the real born)
# wiring and nothing trained. Then the bottom rung's wiring is written into the real sheet
# (blobmnist ceiling --wiring-load) and read by the tree's own reader.
#
#   scripts/ladder2.sh NAME                       the whole ladder, test digits, seeds 5,6,7
#   scripts/ladder2.sh NAME --quick               one seed, one pass on 10,000 digits (a smoke run)
#   scripts/ladder2.sh NAME --seeds 5-12 --rungs net,7b      more seeds for chosen rungs
#   scripts/ladder2.sh NAME --kinds born,lin --parts bars,model   only the cheap arms
#   scripts/ladder2.sh NAME --val --seeds 1,2     on the validation slice (picking)
#   scripts/ladder2.sh NAME --table-only          print what is there
# Options: --tasks mnist,pair --parts bars,model,transplant --kinds lin,trained,born,auto
#   --epochs 12 --jobs 3 --settle 3000 (digits of regulation before a transplant is read).
# Results in out/ladder2/NAME/ (table.txt; one json per run; transplant-*.txt). A finished
# run is kept, so running a line again only runs what is missing. Needs torch: the first
# run makes a venv in out/ladder2/venv (CPU torch, nothing system-wide). One thread a job.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || [ -n "${THONK_MNIST:-}" ] || scripts/get_mnist.sh
venv=${LADDER_VENV:-out/ladder2/venv}
if [ ! -x "$venv/bin/python" ]; then
    python3 -m venv "$venv"
    "$venv/bin/pip" install --quiet --index-url https://download.pytorch.org/whl/cpu torch
    "$venv/bin/pip" install --quiet "numpy<2"
fi
scripts/build.sh --target blobmnist >/dev/null
export OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
exec nice -n 19 "$venv/bin/python" scripts/ladder2.py "$@"
