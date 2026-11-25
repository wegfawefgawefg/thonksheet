#!/usr/bin/env bash
# Idea 58: a readout that can read what is there. Results: docs/results-2026-11-24-readout.md
#
# The reference: a plain linear reader (ridge fit) of every neuron's spike count over each
# scored step of a real task, frozen sheet, no learning. Share right on held-out trials for
# the whole sheet, each tile, and each 3x3 block of tiles (what a motor cell can hear).
#   scripts/readout.sh reference order                     one task, seed 1
#   scripts/readout.sh reference echo --distance 3 --seed 5
#   scripts/readout.sh reference gap --g.local_delay_min 4 --g.local_delay_max 8
#   scripts/readout.sh probe order                         the same reader on the feature probe
#
# The best rule (genome learn_mode 5: nothing inside learns; at the motor cells an error rule
# moving hidden evidence counters, balanced between buttons, 128 motor cells per button).
# One life next to its baselines; anything after the task overrides:
#   scripts/readout.sh life order
#   scripts/readout.sh life order --g.local_delay_min 4 --g.local_delay_max 8
#   scripts/readout.sh life echo --k 8 --motor-spread 256
#   scripts/readout.sh life echo --distance 3 --g.learn_mode 6     whole sheet learning too
#
# The tables in the results doc (seeds 5-12, 2 runs at a time under nice; several hours):
#   scripts/readout.sh tables
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobprobe blobrun
what="${1:-reference}"
[ $# -gt 0 ] && shift
STD="--ticks 20 --sensor-side 3 --motor-side 3"
BEST="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --motor-spread 128"
LIFE="$STD --steps 40000 --window 1000"
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
    exec nice -n 19 ./build/blobrun --task "$task" $LIFE $BEST "$@"
    ;;
tables)
    sw() { python3 scripts/sweep.py --jobs 2 --seeds 5-12 "$@"; }
    for v in readout readout-whole; do
        f=scripts/variants/$v.txt
        sw --name "echo k=2, 1 tile" --variants $f -- --task echo --distance 1 $LIFE
        sw --name "echo k=2, 2 tiles" --variants $f -- --task echo --distance 2 $LIFE
        sw --name "echo k=2, 3 tiles" --variants $f -- --task echo --distance 3 $LIFE
        sw --name "order" --variants $f -- --task order $LIFE
        sw --name "both-not-either" --variants $f -- --task both-not-either $LIFE
        sw --name "gap" --variants $f -- --task gap $LIFE
        sw --name "echo k=4" --variants $f -- --task echo --k 4 $LIFE
    done
    sw --name "order" --variants scripts/variants/readout-delays.txt -- --task order $LIFE
    sw --name "gap" --variants scripts/variants/readout-delays.txt -- --task gap $LIFE
    sw --name "echo k=8" --variants scripts/variants/readout-extra.txt -- --task echo --k 8 $LIFE
    sw --name "shuffled echo" --variants scripts/variants/readout-extra.txt -- \
        --task shuffled-echo $LIFE
    ;;
*)
    echo "scripts/readout.sh reference|probe|life|tables ..." >&2
    exit 1
    ;;
esac
