#!/usr/bin/env bash
# Idea 58: a readout that can read what is there. Results: docs/results-2026-11-24-readout.md
#
# The reference: a plain linear reader (ridge fit) of every neuron's spike count over each
# scored step of a real task, frozen sheet, no reward learning. Share right on held-out
# trials for the whole sheet, each tile, and each 3x3 block of tiles (a motor cell's view).
#   scripts/readout.sh reference order            one task, seed 1
#   scripts/readout.sh reference echo --distance 3 --seed 5
#   scripts/readout.sh reference gap --read-from 5   count only ticks 5.. of the ask step
#   scripts/readout.sh probe order                same reader on the feature probe's tries
#
# The online rule (genome learn_mode 5: the error rule at the motor cells, nothing else
# learns; 6: the same plus the design rule everywhere). One life next to its baselines:
#   scripts/readout.sh life order                         error rule, packed buttons
#   scripts/readout.sh life order --motor-spread 32       32 cells per button, spread
#   scripts/readout.sh life echo --distance 3 --motor-spread 32
#
# The tables in the results doc (seeds 5-12, 2 runs at a time, under nice; long):
#   scripts/readout.sh tables
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobprobe --target blobrun
what="${1:-reference}"
[ $# -gt 0 ] && shift
STD="--ticks 20 --sensor-side 3 --motor-side 3"
case "$what" in
reference)
    task="${1:-order}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/blobprobe linear --task "$task" $STD --label "$task" "$@"
    ;;
probe)
    feature="${1:-order}"
    [ $# -gt 0 ] && shift
    [ "$feature" = both-not-either ] && feature=xor
    exec nice -n 19 ./build/blobprobe feature --feature "$feature" --g.learning_on 0 --linear 1 \
        --label "as born" "$@"
    ;;
life)
    task="${1:-order}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/blobrun --task "$task" $STD --steps 40000 --window 1000 \
        --g.learn_mode 5 "$@"
    ;;
tables)
    exec out/readout/tables.sh
    ;;
*)
    echo "scripts/readout.sh reference|probe|life|tables ..." >&2
    exit 1
    ;;
esac
