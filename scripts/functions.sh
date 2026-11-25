#!/usr/bin/env bash
# The function battery: cheap worlds where the right button is a fixed function of what is
# shown (N-point boolean patterns, functions of a number, of two numbers, of a short past),
# graded in difficulty, each scored at three levels: (a) a linear reader of the sheet as
# born, (b) the readout rule on a frozen sheet, (c) the minimal engine and table neurons.
# Prints one table and the ladder (the largest size each level still gets).
# Definitions: docs/curriculum.md, "The function battery".
# Results: docs/results-2026-11-25-function-battery.md
#
#   scripts/functions.sh main                       every member, levels a b c, seeds 5-12
#   scripts/functions.sh main --quick               10 members, seeds 5 and 6, short lives
#   scripts/functions.sh main --levels b            one level
#   scripts/functions.sh main --table-only          print what has been run so far
#   scripts/functions.sh main --members parity      only members whose id matches
#   scripts/functions.sh big16 --levels ab --members 'parity-n-[34]' --g.tiles_w 16 --g.tiles_h 16
#   scripts/functions.sh main --list                the members and their blobrun flags
#
# Runs go to out/functions/NAME/; finished runs are kept, so a rerun only does what is
# missing (use a new NAME for new flags). 2 runs at a time under nice (--jobs N).
# One task by hand:
#   nice -n 19 ./build/blobrun --task points --fn parity --fn-lines 3 --ticks 20 \
#       --sensor-side 3 --motor-side 3 --steps 40000 --window 1000
#   nice -n 19 ./build/blobprobe reader --task number --fn sine --fn-lines 16 --fn-bands 4 \
#       --fn-hold 3 --ticks 20 --sensor-side 2 --motor-side 3 --seed 5
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobprobe blobrun
exec python3 scripts/functions.py "$@"
