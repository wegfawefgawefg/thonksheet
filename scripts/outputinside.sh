#!/usr/bin/env bash
# The blob's own output for the one-hop rule (idea 182;
# docs/results-2026-11-26-output-inside.md). Every table of that doc from one line. Each NAME
# is a fixed recipe in scripts/outputinside.py:
#   ladder, ladder2, ladder3   from the outside whole-number output down toward the engine's
#                 readout, one difference a rung, each as the outside output of a live life;
#                 validation slice, seeds 1 and 2, MNIST and the pair test
#   ladderengine, ladderengine2, ladderengine3   the last rungs: the engine's own readout
#                 under each of its rules, and told the right line (readout_good 3, 4)
#   loop          the blob's own output (out_on): alone, the closed loop, which lines step
#   looppair      the pair test with the buttons' cells spread evenly, or all of them
#   looppay       the errors back along the output's own sources (out_pay)
#   loopknobs     margin, the cells' own zone, window, width, re-pick every 64, who learns
#   loopcombo     the knobs that looked better, together
#   report        the closed loop on test digits, seeds 5-12, beside born, its coin, a
#                 shuffled-digit life, payment, the output alone, the same rule outside
#   report2       the setting validation picked afterwards, and it with payment
#   reportcells   the same rule on the motor cells as they are (readout_good 4), seeds 5-12
#   recheck       the first report's main arm again (it ran before the tie rule went in)
#   cap           the closed loop with the rate held at target, MNIST, seeds 5-8
#   toys          xor and parity of 3 with the blob's own output teaching the cells
#   toys2         parity of 3 with a wider margin, with no sensor cell read
#   map           two rows of the quick map that lean on the output (scripts/oneset.sh)
#   speed         one MNIST life each, alone (--jobs 1): the life's ticks a second
#   smoke         one seed, every part (use with --quick)
# Arms (scripts/outputinside.py has the flags; scripts/everyevent.py the ones before): born;
# outint2 (the outside whole-number output); fld, fldh, fldbit, fld12, fld8, fld8s3, fld8s6
# (its read and width taken down); fldr1, fldr2, fldr3 (the engine's rules on it); com,
# comreg, bit1 (one-bit votes, one-bit wiring); outtop, fldtop, ... (only the top wrong line
# steps); eng, enggate, engrun, engrunstr, engrun2, engrunf2 (the engine's readout); engans,
# enganstop, enganstope (it told the right line); alone, alonee (the blob's own output on a
# sheet that does not learn); in, intop, intope (the closed loop; e: the buttons' cells
# spread evenly); intopem1 ... intopeall (its knobs); pay, paytop, paytope (payment).
# Examples:
#   scripts/outputinside.sh report                     run what is missing, print the tables
#   scripts/outputinside.sh smoke --quick              short lives: a check of the whole line
#   scripts/outputinside.sh ladder --arms fld,comtop --tests mnist
#   scripts/outputinside.sh report --table-only
#   scripts/outputinside.sh toys --seeds 5-8 --arms bornown,inown
# Options: --seeds 5-8, --jobs 3, --arms a,b, --coins a,b|none, --shuffled a,b|none,
# --tests mnist,pair, --quick, --table-only, --reverse.
# Results in out/outputinside/NAME/ (NAME-quick for --quick); finished runs are kept. A
# closed-loop life with its read is about a minute here, a life under the old readout five.
# OUTPUTINSIDE_BUILD picks another build folder.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || [ -n "${THONK_MNIST:-}" ] || scripts/get_mnist.sh
[ -n "${OUTPUTINSIDE_BUILD:-}" ] || scripts/build.sh --target blobmnist --target blobprobe --target blobrun >/dev/null
exec python3 scripts/outputinside.py "$@"
