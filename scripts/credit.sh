#!/usr/bin/env bash
# The credit rules not yet tried (idea 177, docs/results-2026-11-26-credit-rules.md). On the
# rule's ladder's bench (idea 165's offline model of the sheet's 3,072 cells, a counter per
# candidate input, every marked input counted) each family supplies the cell's teaching bit
# another way than the one-hop random-wire rule (T7x) does: the difference of two phases
# (P2, P2s), credit by code (C2, C1, C2n192, C2n12), payment passed backward along real wires
# (pay1, pay, payc), a target for each cell (tgt, dtp), the one-hop bit passed back once more
# (hopb, dtpb), and for time an error that arrives late with marks that last (d1n, d1t2,
# d1t4, d3t4, d0t4). Each beside born, its coin control (the teaching signal from another
# digit) and T7x on the same seeds. Every wiring is written into the real sheet
# (blobmnist ceiling --wiring-load) on MNIST and the pair test; that number ranks the rules,
# the model's own reader only stands beside it.
#
#   scripts/credit.sh NAME                         every arm with coins, seeds 5,6,7, both tests
#   scripts/credit.sh smoke --quick                one seed, 10,000 digits, every family, end to end
#   scripts/credit.sh NAME --seeds 5-12 --arms T7x,pay,dtp      chosen arms, more seeds
#   scripts/credit.sh pick --val --seeds 1 --arms P2@1,P2@4,P2@16 --coins 0    picking a gain
#   scripts/credit.sh NAME --parts model           the lives only (no real sheet)
#   scripts/credit.sh NAME --parts transplant      write in what has been lived
#   scripts/credit.sh NAME --table-only            print what is there
# Options: --tasks mnist,pair --jobs 3 --coins 0|1 --digits N (life length) --limit N
#   --settle 3000 --val.
# Results in out/credit/NAME/ (table.txt; one json per life; transplant-*.txt; wirings
# gzipped). A finished run is kept, so a line run again only runs what is missing. Needs
# torch: uses LADDER_VENV or out/ladder2/venv (made by scripts/ladder2.sh). One thread a job.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || [ -n "${THONK_MNIST:-}" ] || scripts/get_mnist.sh
venv=${LADDER_VENV:-out/ladder2/venv}
if [ ! -x "$venv/bin/python" ]; then
    echo "no torch venv at $venv: run scripts/ladder2.sh smoke --quick once, or set LADDER_VENV" >&2
    exit 1
fi
scripts/build.sh --target blobmnist >/dev/null
export OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
exec nice -n 19 "$venv/bin/python" scripts/credit.py "$@"
