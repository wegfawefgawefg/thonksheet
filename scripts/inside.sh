#!/usr/bin/env bash
# The inside-learning test: does a life (no reward) under some rule leave the inside of the
# sheet better to read than it was born? Freeze the sheet after the life, fit a fresh linear
# reader, and set it beside the sheet as born and a control. The life-then-freeze is only how
# it is measured. The machine has no pretraining stage: a rule that passes here is meant to
# be always on, in the one life, beside everything else that learns (scripts/mnist.sh
# online with the rule's flags is that life).
#   MNIST: a reader of the non-sensor cells (scripts/mnist.sh's sheet; born 0.924 there) on
#     (a) the sheet as born, (b) after a life of --life training digits, (c) the control: the
#     same life on digits whose pixels are shuffled by a permutation drawn fresh for every
#     digit, read on real digits. Beside them the baselines (linear on pixels 0.922, random
#     projection 0.943, two-layer net 0.955), and lines on how alive the hidden cells are and
#     where their inputs come from (out/inside/NAME/mnist-*.txt).
#   calc: the calculation battery's sheet reader (blobprobe calc) on xor4, parity4, add4,
#     born against after a life of --calc-life ticks, seen and held-out cases.
# Every flag it does not know goes to every run: that is the rule under test.
#   scripts/inside.sh cowire                                  the default rule (co-activity)
#   scripts/inside.sh compete --g.learn_mode 5 --g.compete_shift 4
#   scripts/inside.sh NAME --seeds 5-12 --only mnist ...      more seeds, one part
#   scripts/inside.sh NAME --table-only                       print what is there
# Options: --seeds 1,2 (MNIST; default 1,2), --calc-seeds 5,6, --life 60000 (digits),
#   --fit 60000, --calc-life 200000 (ticks), --trials 3000, --members xor4,parity4,add4,
#   --only mnist|calc, --kinds born,lived,shuffled (MNIST runs to make), --jobs 3 (runs at
#   once). Results in out/inside/NAME/table.txt; a
#   finished run is kept, so running it again only runs what is missing. One MNIST run is
#   about 2 to 4 minutes, one calc run under a minute. Docs: docs/results-2026-11-25-compete.md
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || scripts/get_mnist.sh
scripts/build.sh --target blobmnist --target blobprobe >/dev/null
exec python3 scripts/inside.py "$@"
