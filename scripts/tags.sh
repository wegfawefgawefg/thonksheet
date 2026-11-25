#!/usr/bin/env bash
# Two mechanisms for the readout and the inside: docs/results-2026-11-25-tags-and-margin.md
#
# Barely correct (Baldassi 2007; Brader, Senn and Fusi 2007): the readout moves on good only
# when the vote was close (the stop-learning gate; "deepen" is Baldassi's rule as such).
# Action-feedback tags (AGREL / BrainProp): the pressed button's cells, against the others,
# tag the inner neurons that fed them; the verdict against a running mean moves those
# neurons' hidden counters. Both sit on the best readout (learn mode 5, evidence counters,
# 128 motor cells per button; give --motor-spread 256 for 8 or 16 cues).
#
# One life next to its learning-off twin; anything after the task overrides:
#   scripts/tags.sh life both-not-either                  readout + tags
#   scripts/tags.sh gate echo --k 16 --motor-spread 256   readout + the gate
#   scripts/tags.sh deepen echo --k 8 --motor-spread 256  readout + Baldassi's deepening
#   scripts/tags.sh both order                            readout + tags + the gate
# Did the inside learn? A fresh readout on the frozen sheet after a life with tags, against
# a fresh readout on the sheet as born (seed 5):
#   scripts/tags.sh transfer both-not-either order        live xor, then learn order fresh
#   scripts/tags.sh feature both-not-either xor           feature probe, born and lived
# MNIST with the gate (the MNIST job's sheet and rule, only the good half changed):
#   scripts/tags.sh mnist                                 about 8 minutes, three passes
# The tables in the results doc (seeds 5-12, 2 runs at a time under nice; many hours):
#   scripts/tags.sh tables
#   ROWS="k16" JOBS=4 scripts/tags.sh tables              one row, 4 at a time
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobprobe blobrun
what="${1:-life}"
[ $# -gt 0 ] && shift
STD="--ticks 20 --sensor-side 3 --motor-side 3"
READOUT="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1"
TAGS="--g.tag_mode 2 --g.tag_inh 0 --g.tag_min 4"
MARGIN="--g.readout_evidence 2 --g.readout_good 1 --g.margin_percent 20 --g.margin_win 2"
LIFE="$STD --steps 40000 --window 1000 --motor-spread 128"
case "$what" in
life | gate | deepen | both)
    task="${1:-both-not-either}"
    [ $# -gt 0 ] && shift
    rule="$TAGS"
    [ "$what" = gate ] && rule="$MARGIN"
    [ "$what" = deepen ] && rule="--g.margin_percent 10"
    [ "$what" = both ] && rule="$TAGS $MARGIN"
    exec nice -n 19 ./build/blobrun --task "$task" $LIFE $READOUT $rule "$@"
    ;;
transfer)
    task="${1:-both-not-either}"
    then="${2:-$task}"
    shift $(($# < 2 ? $# : 2))
    for born in 1 0; do
        nice -n 19 ./build/blobprobe transfer --life "--task $task $LIFE $READOUT" $TAGS \
            --then "$then" --then-steps 20000 --seed 5 --born $born "$@"
    done
    ;;
feature)
    task="${1:-both-not-either}"
    feature="${2:-xor}"
    shift $(($# < 2 ? $# : 2))
    # the cue tile is where the task's cue lines sit (tile 2, 3)
    P="feature --feature $feature --g.learning_on 0 --cue-x 2 --cue-y 3 --seed 5"
    nice -n 19 ./build/blobprobe $P --label "as born" "$@"
    nice -n 19 ./build/blobprobe $P --label "after a life with tags" \
        --life "--task $task $LIFE $READOUT $TAGS" "$@"
    ;;
mnist)
    scripts/build.sh --target blobmnist
    [ -f data/mnist/t10k-labels-idx1-ubyte ] || scripts/get_mnist.sh
    SHEET="--mnist-code t28 --g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1 \
--g.motor_w 64 --g.motor_h 64 --motor-spread 102 --g.local_connect_shift 4"
    RULE="--g.learn_mode 5 --g.readout_evidence 2 --g.readout_evidence_strong 0 \
--g.readout_evidence_bits 8 --g.readout_shift 6 --g.readout_balance 1 --g.readout_good 1"
    exec nice -n 19 ./build/blobmnist online $SHEET $RULE --g.margin_percent 20 \
        --g.margin_win 2 --checks 1000,5000,20000,60000,120000,180000 "$@"
    ;;
tables)
    # ROWS picks a subset (names below), so two machines can share the work
    rows="${ROWS:-xor order gap echo2 echo3 echo5 k8 k16 shuffled}"
    T="--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000"
    sw() { python3 scripts/sweep.py --jobs "${JOBS:-2}" --seeds "${SEEDS:-5-12}" \
        --out-dir out/tags/report "$@"; }
    one=scripts/variants/tags-report.txt
    many=scripts/variants/tags-report-many.txt
    for r in $rows; do
        case $r in
        xor) sw --name xor --variants $one -- --task both-not-either $T ;;
        order) sw --name order --variants $one -- --task order $T ;;
        gap) sw --name gap --variants $one -- --task gap $T ;;
        echo[0-9]) sw --name "echo, ${r#echo} tiles" --variants $one -- \
            --task echo --k 2 --distance "${r#echo}" $T ;;
        k[0-9]*) sw --name "echo k=${r#k}" --variants $many -- --task echo --k "${r#k}" $T ;;
        shuffled) SEEDS=5-20 sw --name "shuffled echo" --variants $many -- \
            --task shuffled-echo --k 2 $T ;;
        esac
    done
    ;;
*)
    echo "scripts/tags.sh life|gate|deepen|both|transfer|feature|mnist|tables ..." >&2
    exit 1
    ;;
esac
