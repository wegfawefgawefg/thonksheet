#!/usr/bin/env bash
# Idea 62: credit walks backward through the recorded ticks (reverse replay).
# Results: docs/results-2026-11-25-reverse-replay.md
#
# On top of the best frozen-sheet readout (learn mode 5, evidence counters, 128 cells per
# button), each error step walks the readout's sign back through the last 16 spike maps and
# moves hidden evidence counters of the inner neurons along the edges that carried a spike.
#
# One life next to its baselines; anything after the task overrides:
#   scripts/replay.sh life order
#   scripts/replay.sh life order --g.replay_depth 4
#   scripts/replay.sh life echo --distance 3
# What the walk does, window by window (spikes walked per depth, counter steps, rewires):
#   scripts/replay.sh diag order
# The linear reader (blobprobe linear) on a sheet walked for a whole life, then frozen with
# the motor cells back at birth; --g.replay_depth 0 gives the same for the born sheet:
#   scripts/replay.sh probe order --seed 5
#   scripts/replay.sh probe order --seed 5 --g.replay_depth 0
#
# The tables in the results doc (seeds 1-4 tuning, 5-12 report; 2 runs at a time under nice;
# several hours):
#   scripts/replay.sh tables
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobprobe blobrun
what="${1:-life}"
[ $# -gt 0 ] && shift
STD="--ticks 20 --sensor-side 3 --motor-side 3"
BEST="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --motor-spread 128"
WALK="--g.replay_depth 2"
LIFE="$STD --steps 40000 --window 1000"
case "$what" in
life)
    task="${1:-order}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/blobrun --task "$task" $LIFE $BEST $WALK "$@"
    ;;
diag)
    task="${1:-order}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/blobprobe replay --task "$task" $LIFE $BEST $WALK "$@"
    ;;
probe)
    # 40,000 steps of 20 ticks with the walk, then frozen and the motor cells reset
    task="${1:-order}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/blobprobe linear --task "$task" $STD $BEST $WALK --learning 1 \
        --g.replay_until 800000 --g.replay_reset 2 --settle 801000 --label "$task" "$@"
    ;;
tables)
    sw() { python3 scripts/sweep.py --jobs 2 --seeds 5-12 --out-dir out/replay/sweep "$@"; }
    DELAYS="--g.local_delay_min 4 --g.local_delay_max 8"
    f=scripts/variants/replay.txt
    sw --name "order" --variants $f -- --task order $LIFE
    sw --name "both-not-either" --variants $f -- --task both-not-either $LIFE
    sw --name "gap" --variants $f -- --task gap $LIFE
    sw --name "order, delays 4-8" --variants $f -- --task order $LIFE $DELAYS
    sw --name "gap, delays 4-8" --variants $f -- --task gap $LIFE $DELAYS
    sw --name "echo k=2, 2 tiles" --variants $f -- --task echo --distance 2 $LIFE
    sw --name "echo k=2, 3 tiles" --variants $f -- --task echo --distance 3 $LIFE
    sw --name "echo k=2, 5 tiles" --variants $f -- --task echo --distance 5 $LIFE
    sw --name "echo k=4" --variants $f -- --task echo --k 4 $LIFE
    f=scripts/variants/replay-many.txt
    sw --name "echo k=8" --variants $f -- --task echo --k 8 $LIFE
    sw --name "echo k=16" --variants $f -- --task echo --k 16 $LIFE
    f=scripts/variants/replay.txt
    sw --name "shuffled echo" --variants $f -- --task shuffled-echo $LIFE
    f=scripts/variants/replay-depth.txt
    sw --name "order" --variants $f -- --task order $LIFE
    sw --name "both-not-either" --variants $f -- --task both-not-either $LIFE
    sw --name "echo k=2, 3 tiles" --variants $f -- --task echo --distance 3 $LIFE
    sw --name "echo k=2, 5 tiles" --variants $f -- --task echo --distance 5 $LIFE
    # a fresh readout on the walked sheet, same task and another one (windows 40-79)
    f=scripts/variants/replay-relearn.txt
    sw --windows 40:80 --name "order, then order" --variants $f -- --task order \
        --retention order --retest 1000 $LIFE
    sw --windows 40:80 --name "both-not-either, then both-not-either" --variants $f -- \
        --task both-not-either --retention both-not-either --retest 1000 $LIFE
    sw --windows 40:80 --name "order, then both-not-either" --variants $f -- --task order \
        --retention both-not-either --retest 1000 $LIFE
    sw --windows 40:80 --name "echo 3 tiles, then order" --variants $f -- --task echo \
        --distance 3 --retention order --retest 1000 $LIFE
    ;;
*)
    echo "scripts/replay.sh life|diag|probe|tables ..." >&2
    exit 1
    ;;
esac
