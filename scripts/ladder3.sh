#!/usr/bin/env bash
# The rule's ladder (idea 167, docs/results-2026-11-25-rule-ladder.md): idea 165's offline
# model of the sheet's 3,072 cells at its bottom rung (7b, every constraint of the engine),
# taught first by backprop (a measuring stick, not the method) and then by rules that step one
# at a time toward the engine's one-hop rule: less data, one digit at a time, the error
# stopping at the cell, fixed random feedback wires, a few +-1 wires a cell, a sign per line,
# no derivative (fired / near), counter steps on inputs that were on, one verdict / the right
# button / the answer, only the engine's learners. Each rung is read by a fresh linear reader
# of the cells (MNIST and the pair test), beside its coin control (the cells given another
# digit's errors) and the born arm; chosen rungs' wirings are written into the real sheet
# (blobmnist ceiling --wiring-load).
#
#   scripts/ladder3.sh NAME                        the ladder T0b..T10 with coins, seeds 5,6,7
#   scripts/ladder3.sh NAME --quick                one seed, 10,000 digits, a smoke run of every part
#   scripts/ladder3.sh NAME --seeds 5-12 --arms T5,T6 --transplant T5,T6 --parts model,transplant
#   scripts/ladder3.sh NAME --arms A0,A3,A5 --coins 0          single steps alone from the top
#   scripts/ladder3.sh pick --val --seeds 1 --arms T2@3,T2@10,T2@30 --coins 0   picking a step
#   scripts/ladder3.sh NAME --parts engine --seeds 5           the engine's own rule, real sheet
#   scripts/ladder3.sh NAME --table-only           print what is there
# Options: --tasks mnist,pair --jobs 3 --digits N (life length) --settle 3000.
# Results in out/ladder3/NAME/ (table.txt; one json per run; transplant-*.txt). A finished
# run is kept, so a line run again only runs what is missing. Needs torch: uses
# LADDER_VENV or out/ladder2/venv (made by scripts/ladder2.sh). One thread a job.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || [ -n "${THONK_MNIST:-}" ] || scripts/get_mnist.sh
venv=${LADDER_VENV:-out/ladder2/venv}
if [ ! -x "$venv/bin/python" ]; then
    echo "no torch venv at $venv: run scripts/ladder2.sh smoke --quick once, or set LADDER_VENV"
    exit 1
fi
scripts/build.sh --target blobmnist >/dev/null
export OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
exec nice -n 19 "$venv/bin/python" scripts/ladder3.py "$@"
