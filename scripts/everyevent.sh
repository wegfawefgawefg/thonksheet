#!/usr/bin/env bash
# Every-event counters in the engine's one-hop rule (idea 178;
# docs/results-2026-11-26-every-event.md). Every table of that doc from one line. Each NAME
# is a fixed recipe in scripts/everyevent.py:
#   pick      every arm on the validation slice, seeds 1 and 2, MNIST and the pair test
#   pick4     the model's other differences (re-pick every 64, one mark a digit, 9 bits)
#   report    the key arms on test digits, seeds 5-12, beside born, coin and shuffled lives
#   cap       the best arm with the rate held at target, seeds 5-8
#   speed     the rule off, the old rule and every-event in one judged MNIST life each:
#             the sheet's own readout and the life's ticks a second (run with --jobs 1)
#   toys      xor and parity of 3: the old rule against every-event counters, seeds 5-12
#   calc      the calc held-out columns (xor4, add4), seeds 5 and 6
#   smoke     one seed, every part (use with --quick)
# Arms (scripts/everyevent.py has the flags): born; off (the readout alone); old (the
# engine's rule from the answer, chance 2^-9, 6 bits); ev3 (every event, 12-bit planes, from
# the answer); ev3i, ev3all (inhibitory and pacemaker cells learn too; motor cells too);
# ev4, ev5 (the sheet's own signed error per line, by share, by margin); out, outi, outall
# (an outside softmax output's signed error per line); out6all (those errors on the old
# counters); ev3b6, ev3b8, ev3p9, outall9 (other counter widths); outall64, outallloose,
# outallboth (re-pick every 64 verdicts, one mark a digit).
# Examples:
#   scripts/everyevent.sh report                       run what is missing, print the tables
#   scripts/everyevent.sh smoke --quick                short lives: a check of the whole line
#   scripts/everyevent.sh report --seeds 9-12 --jobs 2
#   scripts/everyevent.sh pick --arms born,ev3,outall --coins outall --tests pair
#   scripts/everyevent.sh report --table-only
# Options: --seeds 5-8, --jobs 3, --arms a,b, --coins a,b|none, --shuffled a,b|none,
# --tests mnist,pair, --quick, --table-only.
# Results in out/everyevent/NAME/ (NAME-quick for --quick); finished runs are kept. A judged
# MNIST life with its read is about 6 minutes here, an outside-output life about 3.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || [ -n "${THONK_MNIST:-}" ] || scripts/get_mnist.sh
scripts/build.sh --target blobmnist --target blobprobe >/dev/null
exec python3 scripts/everyevent.py "$@"
