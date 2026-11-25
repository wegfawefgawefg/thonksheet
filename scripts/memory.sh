#!/usr/bin/env bash
# The memory engine (idea 123, src/memory/, docs/results-2026-11-25-memory-engine.md): no
# sheet; sensed bits become a code, the code finds slots in a store by content, slots hold
# a counter per button, good and bad move them.
#
#   scripts/memory.sh task echo                 one life (standard plug), beside learning off
#   scripts/memory.sh task echo --k 16 --seed 5      and coin flips
#   scripts/memory.sh task delayed-echo --delay 50   a cue held across 1,000 blank ticks
#   scripts/memory.sh task echo --y.tables 64        --y.name value sets a memory number
#   scripts/memory.sh suite                     the full suite, seeds 5-12 (scorecard memory)
#   scripts/memory.sh tries                     echo k=2, right or wrong on each of the first
#                                               40 steps, seeds 5-12
#   scripts/memory.sh pairs 1024 5              N random line patterns to 16 buttons: how many
#                                               pairs it holds (seed 5), beside learning off
#   scripts/memory.sh hold 2 5                  delayed echo with 2 other lines blinking in
#                                               the gap (seed 5): how many events a cue survives
#   scripts/memory.sh sheet place 5             the combination: the memory reading a sheet
#                                               held fixed (1) and a learning one (2)
#   scripts/memory.sh ladder                    pairs and hold ladders, seeds 5-12
#   scripts/memory.sh speed                     ticks per second (random input) and bytes
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun memprobe >/dev/null

base=(--engine memory --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000)

cmd="${1:-task}"
[ $# -gt 0 ] && shift
case "$cmd" in
task)
    task="${1:-echo}"
    [ $# -gt 0 ] && shift
    side=3
    extra=()
    if [ "$task" = cartpole ]; then
        side=2
        extra=(--cart-reward lean --number-code place)
    fi
    exec nice -n 19 ./build/blobrun "${base[@]}" --task "$task" --sensor-side "$side" \
        "${extra[@]}" "$@"
    ;;
suite)
    exec scripts/suite.sh memory --engine memory "$@"
    ;;
tries)
    mkdir -p out/memory/tries
    for s in 5 6 7 8 9 10 11 12; do
        nice -n 19 ./build/blobrun "${base[@]}" --task echo --steps 40 --window 1 \
            --no-baselines --alive-ticks 0 --rhythm-ticks 0 --seed "$s" \
            --out "out/memory/tries/s$s.csv" "$@" >/dev/null
        printf "seed %2s  " "$s"
        awk -F, 'NR > 1 { printf "%s", ($3 > 0.5 ? "+" : ".") } END { print "" }' \
            "out/memory/tries/s$s.csv"
    done
    ;;
pairs)
    n="${1:-256}" seed="${2:-5}"
    shift 2 || shift $#
    nice -n 19 ./build/memprobe pairs --n "$n" --seed "$seed" "$@"
    nice -n 19 ./build/memprobe pairs --n "$n" --seed "$seed" --off 1 "$@"
    ;;
hold)
    d="${1:-1}" seed="${2:-5}"
    shift 2 || shift $#
    nice -n 19 ./build/memprobe hold --distractors "$d" --seed "$seed" "$@"
    ;;
ladder)
    # pairs 16..4096 (and learning off) and hold with 0..3 distractors, seeds 5-12, 2 at a
    # time; lines land in out/memory/ladder.txt
    mkdir -p out/memory
    for s in 5 6 7 8 9 10 11 12; do
        for n in 16 64 256 1024 2048 4096; do
            echo "pairs --n $n --seed $s"
            echo "pairs --n $n --seed $s --off 1"
        done
        for d in 0 1 2 3; do
            echo "hold --distractors $d --seed $s"
            echo "hold --distractors $d --seed $s --off 1"
        done
    done | xargs -P 2 -I{} sh -c 'nice -n 19 ./build/memprobe {} '"$*"' >> out/memory/ladder.txt'
    ;;
sheet)
    task="${1:-place}" seed="${2:-5}"
    shift 2 || shift $#
    for s in 0 1 2; do
        echo "== --y.sheet $s"
        nice -n 19 ./build/blobrun "${base[@]}" --task "$task" --seed "$seed" \
            --task-seed "$seed" --y.sheet "$s" --alive-ticks 0 --rhythm-ticks 0 \
            --out "out/memory/sheet-$task-$s-s$seed.csv" "$@" | grep -E '^(blob|learn_off|coin) +[0-9]'
    done
    ;;
speed)
    nice -n 19 ./build/blobrun --engine memory --speed "$@"
    nice -n 19 ./build/memprobe bytes "$@"
    ;;
*)
    sed -n '2,20p' "$0"
    exit 1
    ;;
esac
