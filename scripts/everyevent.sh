#!/usr/bin/env bash
# Every-event counters in the engine's one-hop rule (idea 178;
# docs/results-2026-11-26-every-event.md). Every table of that doc from one line. Each NAME
# is a fixed recipe in scripts/everyevent.py:
#   pick      every arm on the validation slice, seeds 1 and 2, MNIST and the pair test
#   pick4     the model's other differences (re-pick every 64, one mark a verdict, 9 bits)
#   pick2     after pick: each cell taught "more" and "less" equally often, other margins,
#             told the right button, and which part of the outside output's error matters
#   pick3, pick5, pick6   the outside output in whole numbers and its margin; the softmax
#             output's fine zone; 16-bit counters
#   report    the key arms (out, outf3, outint2) on test digits, seeds 5-12, beside born,
#             their coin and their shuffled-digit lives
#   pieces    the other arms beside them, seeds 5-7 (same folder as report)
#   cap       out with the rate held at target, MNIST, seeds 5-8
#   online    the sheet's own readout during a judged life, seeds 5-7 (report's folder)
#   speed     one MNIST life each, alone (--jobs 1): the life's ticks a second
#   toys      xor and parity of 3: the old rule against every-event counters, seeds 5-12
#   calc      the calc held-out columns (xor4, add4), seeds 5 and 6
#   smoke     one seed, every part (use with --quick)
# Arms (scripts/everyevent.py has the flags): born; off (the readout alone); old (the
# engine's rule from the answer, chance 2^-9, 6 bits); ev3 (every event, 12-bit planes, from
# the answer); ev3i, ev3all (inhibitory and pacemaker cells learn too; motor cells too);
# ev2 (told the right button on bad steps); ev4, ev5 (the sheet's own signed error per
# line, by share, by margin; ev5f4, ev5f16 other margins); ev3bal, ev5bal (each cell taught
# more and less equally often); out, outi, outall (an outside softmax output's signed error
# per line, fine under 0.1); outf05, outf2, outf3, outf5 (other fine zones); outint1,
# outint2, outint, outint32 (the outside output in whole numbers, a perceptron with a margin
# of 1, 2, 8, 32 steps); outwhen, outsigns (which part of the outside error matters); outj,
# outintj (out and outint2 in a judged life); out6s3, out6s5, outf36s3, outf36s5, out6all
# (those errors on the old 6-bit chance counters); ev3b6, ev3b8, ev3p9, out9, out16 (other
# counter widths); out64, outloose, outboth, outallboth (re-pick every 64 verdicts, one
# mark a verdict).
# Examples:
#   scripts/everyevent.sh report                       run what is missing, print the tables
#   scripts/everyevent.sh smoke --quick                short lives: a check of the whole line
#   scripts/everyevent.sh report --seeds 9-12 --jobs 2
#   scripts/everyevent.sh pick --arms born,ev3,outall --coins outall --tests pair
#   scripts/everyevent.sh report --table-only
# Options: --seeds 5-8, --jobs 3, --arms a,b, --coins a,b|none, --shuffled a,b|none,
# --tests mnist,pair, --quick, --table-only, --reverse (the job list from its end, for a
# second machine working the same list).
# Results in out/everyevent/NAME/ (NAME-quick for --quick); finished runs are kept. A judged
# MNIST life with its read is about 6 minutes here, an outside-output life about 3.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || [ -n "${THONK_MNIST:-}" ] || scripts/get_mnist.sh
scripts/build.sh --target blobmnist --target blobprobe >/dev/null
exec python3 scripts/everyevent.py "$@"
