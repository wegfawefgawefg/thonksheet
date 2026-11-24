#!/usr/bin/env bash
# Idea 26, table neurons (src/table/): the same tasks, plug and baselines as task.sh, with
# the table sheet in place of the blob. Results: docs/results-2026-11-24-table-neurons.md.
#   scripts/table.sh one echo                 one life on echo k=2, next to its twins
#   scripts/table.sh one echo --k 4 --seed 3
#   scripts/table.sh seeds echo 5 12          seeds 5..12: late scores per seed and the mean
#   scripts/table.sh seeds echo 5 12 --k 4 --t.inputs 6
#   ENGINE=blob scripts/table.sh seeds echo 5 12     the same with the popcount blob
#   scripts/table.sh ladder                   experiment 1: echo k=2,4,8, shuffled, delayed,
#                                             catch, cartpole on seeds 5-12, both engines
#   scripts/table.sh distance                 experiment 2: cue and buttons 1,2,3,5 tiles apart
#   scripts/table.sh capacity                 experiment 3: echo k = 2, 4, 8, 16
#   scripts/table.sh saturate                 experiment 4: counters and firing rates at the
#                                             start and end of a long life
#   scripts/table.sh speed                    experiment 5: ticks per second, bytes per neuron
# Table numbers are set with --t.name value (see src/table/table.hpp), sheet numbers with
# --g.name value. Runs go 4 at a time, nice'd; csv and logs land in out/table/.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh
engine="${ENGINE:-table}"
common=(--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000)

life() { # life SEED TASK OUTDIR flags...
    local seed=$1 task=$2 dir=$3
    shift 3
    local pick=()
    [ "$engine" = table ] && pick=(--table)
    nice -n 19 ./build/blobrun "${pick[@]}" --task "$task" "${common[@]}" --seed "$seed" \
        --out "$dir/$task-s$seed.csv" "$@" >"$dir/$task-s$seed.log" 2>&1
}

late() { # late LOG ROWNAME: the late score of one row of blobrun's table
    awk -v row="$2" '$1 == row && $2 ~ /^[0-9.]+$/ { print $3 }' "$1"
}

seeds() { # seeds TASK FROM TO flags...: one line per seed, then the means
    local task=$1 from=$2 to=$3
    shift 3
    local label="${LABEL:-$task}"
    local dir="out/table/$engine/$label"
    mkdir -p "$dir"
    for s in $(seq "$from" "$to"); do
        life "$s" "$task" "$dir" "$@" &
        while [ "$(jobs -rp | wc -l)" -ge 4 ]; do wait -n; done
    done
    wait
    local on=table off=table_off
    [ "$engine" = blob ] && on=blob && off=learn_off
    echo "== $engine $label: $task $*"
    printf "%6s %10s %10s %10s\n" seed "$on" "$off" coin
    for s in $(seq "$from" "$to"); do
        local log="$dir/$task-s$s.log"
        printf "%6s %10s %10s %10s\n" "$s" "$(late "$log" "$on")" "$(late "$log" "$off")" \
            "$(late "$log" coin)"
    done | tee "$dir/summary.txt"
    awk '{ a += $2; b += $3; c += $4; n++ } END { printf "%6s %10.4f %10.4f %10.4f\n", "mean", a/n, b/n, c/n }' \
        "$dir/summary.txt"
}

both() { # both LABEL TASK flags...: seeds 5-12 on the table sheet and on the blob
    local label=$1
    shift
    LABEL="$label" engine=table seeds "$1" 5 12 "${@:2}"
    LABEL="$label" engine=blob seeds "$1" 5 12 "${@:2}"
}

# cue cells on tile row 0, button cells on tile row d (plus the plug's sideways offset)
apart() {
    echo --g.sensor_h 8 --g.motor_y $((8 * $1)) --g.motor_h 8
}

case "${1:-one}" in
one)
    task="${2:-echo}"
    shift 2 || shift $#
    pick=()
    [ "$engine" = table ] && pick=(--table)
    exec nice -n 19 ./build/blobrun "${pick[@]}" --task "$task" "${common[@]}" "$@"
    ;;
seeds)
    shift
    seeds "$@"
    ;;
ladder)
    both echo2 echo --k 2
    both echo4 echo --k 4
    both echo8 echo --k 8
    both shuffled shuffled-echo
    both delayed delayed-echo
    both catch catch --sensor-side 2 --motor-side 2
    both cartpole cartpole --sensor-side 2 --motor-side 2
    ;;
distance)
    for d in 1 2 3 5; do
        # shellcheck disable=SC2046
        both "apart$d" echo $(apart "$d")
    done
    ;;
capacity)
    for k in 2 4 8 16; do
        LABEL="cap$k" engine=table seeds echo 5 12 --k "$k"
    done
    ;;
saturate)
    # one long life (4M ticks), spreads every 65,536 ticks; the first and last are the ones
    dir=out/table/saturate
    mkdir -p "$dir"
    for s in 5 6; do
        nice -n 19 ./build/blobrun --table --task echo "${common[@]}" --steps 200000 \
            --window 10000 --seed "$s" --t.report 1 --out "$dir/echo-s$s.csv" >"$dir/echo-s$s.log" &
    done
    wait
    for s in 5 6; do
        echo "== seed $s"
        grep -A6 -E "table report tick 65536 |table report end" "$dir/echo-s$s.log"
        grep -E "^(table|table_off|coin) +[0-9]" "$dir/echo-s$s.log"
    done
    ;;
speed)
    nice -n 19 ./build/tablebench "${@:2}"
    nice -n 19 ./build/tablebench --t.inputs 6 "${@:2}"
    nice -n 19 ./build/tablebench --t.inputs 4 "${@:2}"
    nice -n 19 ./build/tablebench --t.learning_on 0 "${@:2}"
    ;;
*)
    sed -n '2,20p' "$0"
    exit 1
    ;;
esac
