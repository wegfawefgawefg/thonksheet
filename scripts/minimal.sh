#!/usr/bin/env bash
# The minimal engine (src/minimal/, docs/results-2026-11-24-minimal-engine.md).
#
#   scripts/minimal.sh task echo                one life, beside learning off and coin flips
#   scripts/minimal.sh task echo --k 4 --seed 5
#   scripts/minimal.sh task echo --m.cowire_on 0       --m.name value sets a minimal number
#   scripts/minimal.sh seeds echo2 5 12         a preset on seeds 5..12, two at a time, then
#                                               the mean and spread of the late scores
#   scripts/minimal.sh seeds far3 5 12 --m.budget_on 0      extra flags go to every life
#   scripts/minimal.sh seeds echo2 5 12 --engine blob       the full engine, same preset
#   scripts/minimal.sh probe alive [--input 0]  rate, pulse, silent/stuck on a fresh sheet
#   scripts/minimal.sh probe life --steps 200000            a long Echo life, then the wiring
#   scripts/minimal.sh probe idle               teach Echo, unplug 2M ticks, retest
#   scripts/minimal.sh probe speed              ticks/s at 4,096, 65,536, 1,048,576 neurons
#   scripts/minimal.sh probe trace --task delayed-echo --delay 2 --test-steps 40000
#                                               teach, freeze, then how readable the cue is
#                                               tick by tick after it goes off
#   scripts/minimal.sh seeds far5 5 12 --m.far_slots 1      far slots (off by default)
#   scripts/minimal.sh seeds delayed 5 12 --m.delay_max 8   slot delays (off by default)
#   scripts/minimal.sh seeds far3 5 12 --m.ev_on 1 --m.far_slots 2 --m.far_percent 100
#                                               evidence counters on the motor neurons (2:
#                                               every neuron), off by default; --m.ev_bits 4..8
#                                               (docs/results-2026-11-24-evidence.md)
#   (probe takes --engine full for the full engine measured the same way)
#
# Presets: echo2 echo4 echo8 echo16 shuffled delayed catch cartpole far1..far5 (echo k=2 with
# cue and button 1..5 tiles apart), delayed0 delayed1 (the ask right after the cue step, or one
# blank step between), beat2 beat4 (press every 2 or 4 steps, no cue), order both gap (the
# feature tasks, cue and buttons one tile apart).
# Results land in out/minimal/<preset>[-tag]/ (OUTDIR=dir to put them elsewhere).
# Set TAG=name to keep runs with different extra flags apart.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun minimalprobe

base=(--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000)

preset() {
    case "$1" in
    echo2) echo --task echo --k 2 ;;
    echo4) echo --task echo --k 4 ;;
    echo8) echo --task echo --k 8 ;;
    echo16) echo --task echo --k 16 ;;
    order) echo --task order ;;
    both) echo --task both-not-either ;;
    gap) echo --task gap ;;
    shuffled) echo --task shuffled-echo --k 2 ;;
    delayed) echo --task delayed-echo --k 2 --delay 2 ;;
    delayed0) echo --task delayed-echo --k 2 --delay 0 ;;
    delayed1) echo --task delayed-echo --k 2 --delay 1 ;;
    beat2) echo --task beat --period 2 ;;
    beat4) echo --task beat --period 4 ;;
    catch) echo --task catch --sensor-side 2 --catch-size 6 6 ;;
    cartpole) echo --task cartpole --cart-reward lean --number-code place \
        --sensor-side 2 --steps 100000 ;;
    cartsparse) echo --task cartpole --sensor-side 2 --steps 100000 ;; # the old reward and lines
    far1) echo --task echo --k 2 --g.motor_y 32 --g.motor_h 16 ;;
    far2) echo --task echo --k 2 --g.motor_y 40 --g.motor_h 16 ;;
    far3) echo --task echo --k 2 --g.motor_y 48 --g.motor_h 16 ;;
    far4) echo --task echo --k 2 --g.sensor_y 10 --g.sensor_h 16 --g.motor_y 48 --g.motor_h 16 ;;
    far5) echo --task echo --k 2 --g.sensor_y 2 --g.sensor_h 16 --g.motor_y 48 --g.motor_h 16 ;;
    *) echo "no preset $1" >&2; exit 1 ;;
    esac
}

# mean and spread (sd) of the late score of each driver over the given runs
summarize() {
    awk 'NF == 4 && ($1 == "blob" || $1 == "learn_off" || $1 == "coin") {
             n[$1]++; s[$1] += $3; q[$1] += $3 * $3 }
         END {
             for (d in n) {
                 m = s[d] / n[d]; v = q[d] / n[d] - m * m; if (v < 0) v = 0
                 printf "  %-10s late %.3f  sd %.3f  (%d lives)\n", d, m, sqrt(v), n[d]
             }
         }' "$@" | sort
}

cmd="${1:-task}"
[ $# -gt 0 ] && shift
case "$cmd" in
task)
    task="${1:-echo}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/blobrun --engine minimal --task "$task" "${base[@]}" "$@"
    ;;
seeds)
    name="$1" from="$2" to="$3"
    shift 3
    dir="${OUTDIR:-out/minimal}/$name${TAG:+-$TAG}"
    mkdir -p "$dir"
    read -r -a flags <<< "$(preset "$name")"
    seq "$from" "$to" | xargs -P 2 -I{} sh -c \
        'nice -n 19 ./build/blobrun --engine minimal "$@" --seed {} --task-seed {} \
            --out "'"$dir"'/s{}.csv" > "'"$dir"'/s{}.txt"' _ "${base[@]}" "${flags[@]}" "$@"
    echo "$name${TAG:+ ($TAG)}, seeds $from-$to: $*"
    summarize $(seq -f "$dir/s%g.txt" "$from" "$to")
    ;;
probe)
    mode="$1"
    shift
    exec nice -n 19 ./build/minimalprobe "$mode" "${base[@]}" "$@"
    ;;
*)
    echo "scripts/minimal.sh task|seeds|probe ..." >&2
    exit 1
    ;;
esac
