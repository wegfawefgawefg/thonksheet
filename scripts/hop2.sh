#!/usr/bin/env bash
# What stops the one-hop error on real data (idea 163; docs/results-2026-11-25-hop-real.md).
# Every table of that doc from one line. Each NAME is a fixed recipe in scripts/hop2.py:
#   bar        the bar and the room for the pair tests: linear on the bits, the born sheet,
#              a random projection, a two-layer net (1 and 5 passes); seed 1
#   pick       the step chance on the same-class test: validation slice, seeds 1 and 2
#   same       the report on "are two interleaved digits the same class", seeds 5-12: born,
#              the hop rule told the answer, its coin (and on distinct lines), its
#              shuffled-digit life, competitive learning, both, both with the coin
#   sum        the second test, "is the sum of two side-by-side digits odd", seeds 5-8
#   wide       MNIST with wider cells (more pixel inputs a cell), seeds 5-8
#   widesame   the same-class test with wider cells, seeds 5-8
#   deepsame   the same-class test with half the cells hearing no pixels, seeds 5-12
#   targetpick, target   MNIST with errors from the answer alone (hop_teach 3; target
#              seeds 5-12); targetcap the same at target rate (5-8); slow the old errors
#              at the same step (5-8); targetsame, slowsame on the pair test (5-8)
#   toys       xor and parity of 3 with the shuffled-input control, seeds 5-12
#   parity4    parity of 4 under every arm, seeds 5-8
#   calc       the calc held-out columns: shuffled input, depth, width; seeds 5 and 6
# Examples:
#   scripts/hop2.sh same                      run what is missing, print the table
#   scripts/hop2.sh same --quick              one seed, short lives: a smoke check
#   scripts/hop2.sh same --seeds 9-12 --jobs 2
#   scripts/hop2.sh same --arms born,hop,coin
#   scripts/hop2.sh same --table-only
# Results in out/hop2/NAME/ (NAME-quick for --quick); finished runs are kept. An MNIST-sized
# run is 3 to 6 minutes here, a toy run under a minute, a calc run one to three minutes.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || scripts/get_mnist.sh
scripts/build.sh --target blobmnist --target blobprobe >/dev/null
exec python3 scripts/hop2.py "$@"
