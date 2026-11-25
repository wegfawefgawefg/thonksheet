#!/usr/bin/env bash
# Two mechanisms for the readout and the inside: docs/results-2026-11-25-tags-and-margin.md
#
# Barely correct (Baldassi 2007): on good, when the vote was close, the pressed button's cells
# deepen the counters of the sources that helped. Action-feedback tags (AGREL / BrainProp):
# the pressed button's cells tag the inner neurons that fed them; the verdict moves those
# neurons' hidden counters. Both sit on the best readout (learn mode 5, evidence counters,
# 128 motor cells per button).
#
# One life next to its learning-off twin; anything after the task overrides:
#   scripts/tags.sh life both-not-either                  readout + tags
#   scripts/tags.sh life echo --k 8 --motor-spread 256 --g.tag_mode 0 --g.margin_percent 20
#   scripts/tags.sh margin echo --k 16 --motor-spread 256  readout + margin rule only
#   scripts/tags.sh both order                            readout + tags + margin rule
# Did the inside learn? A fresh readout on the frozen sheet after a life with tags, against
# a fresh readout on the sheet as born (seed 5):
#   scripts/tags.sh transfer both-not-either order        live xor, then learn order fresh
#   scripts/tags.sh feature both-not-either xor           feature probe, born and lived
# The tables in the results doc (seeds 5-12, 2 runs at a time under nice; many hours):
#   scripts/tags.sh tables
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobprobe blobrun
what="${1:-life}"
[ $# -gt 0 ] && shift
STD="--ticks 20 --sensor-side 3 --motor-side 3"
READOUT="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1"
TAGS="--g.tag_mode 1"
MARGIN="--g.margin_percent 20"
LIFE="$STD --steps 40000 --window 1000 --motor-spread 128"
case "$what" in
life | margin | both)
    task="${1:-both-not-either}"
    [ $# -gt 0 ] && shift
    rule="$TAGS"
    [ "$what" = margin ] && rule="$MARGIN"
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
tables)
    # ROWS picks a subset (names below), so two machines can share the work
    rows="${ROWS:-xor order gap echo2 echo3 echo5 k8 k16 k32 k64 shuffled}"
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
    echo "scripts/tags.sh life|margin|both|transfer|feature|tables ..." >&2
    exit 1
    ;;
esac
