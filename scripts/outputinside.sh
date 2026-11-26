#!/usr/bin/env bash
# The blob's own output for the one-hop rule (idea 182;
# docs/results-2026-11-26-output-inside.md). Every table of that doc from one line. Each NAME
# is a fixed recipe in scripts/outputinside.py:
#   ladder        from the outside whole-number output down toward the engine's readout, one
#                 difference a rung, each as the outside output of a live life; validation
#                 slice, seeds 1 and 2, MNIST and the pair test
#   ladderengine  the last rungs: the engine's own readout under each of its rules
#   smoke         one seed, every part (use with --quick)
# Examples:
#   scripts/outputinside.sh ladder                   run what is missing, print the table
#   scripts/outputinside.sh smoke --quick            short lives: a check of the whole line
#   scripts/outputinside.sh ladder --arms fld,com --tests mnist
#   scripts/outputinside.sh ladder --table-only
# Options: --seeds 5-8, --jobs 3, --arms a,b, --coins a,b|none, --shuffled a,b|none,
# --tests mnist,pair, --quick, --table-only, --reverse.
# Results in out/outputinside/NAME/ (NAME-quick for --quick); finished runs are kept.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || [ -n "${THONK_MNIST:-}" ] || scripts/get_mnist.sh
scripts/build.sh --target blobmnist --target blobprobe --target blobrun >/dev/null
exec python3 scripts/outputinside.py "$@"
