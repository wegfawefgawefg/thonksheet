#!/usr/bin/env bash
# The fixed reference networks (src/nn/, docs/results-2026-11-25-fixed-net-reference.md): an
# ordinary mlp and gru, one size and one setting, told only good and bad, run on the whole
# suite unchanged. Their one setting is the default of `--n.` numbers; never set it per task.
#   scripts/reference.sh one echo                 one life, mlp (add --engine gru for the gru)
#   scripts/reference.sh one delayed-echo --engine gru --delay 2
#   scripts/reference.sh select                   how the setting was chosen (seeds 1-4)
#   scripts/reference.sh suite                    mlp-fixed and gru-fixed scorecards
#   scripts/reference.sh told                     the labelled extra: told the right button
#   scripts/reference.sh compare                  reference against the home-grown engines
#   scripts/reference.sh hold                     the gru on delayed echo, delay 4 to 16
# The suites run 2 lives at a time under nice, like scripts/suite.sh.
set -euo pipefail
cd "$(dirname "$0")/.."
what="${1:-}"
[ $# -gt 0 ] && shift
life="--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000"
case "$what" in
one)
    scripts/build.sh --target blobrun
    task="${1:-echo}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/blobrun --engine mlp --task "$task" $life "$@"
    ;;
select)
    scripts/build.sh --target blobrun
    # the grid the setting was picked from: highest mean late score over the six
    # network-by-task cells won (Adam 0.0001, 0.904; Adam 0.0003 0.896)
    mkdir -p out/nn/select
    {
        for lr in 0.01 0.003 0.001 0.0003 0.0001 0.00003; do
            echo "adam$lr: --n.adam 1 --n.lr $lr"
        done
        for lr in 0.3 0.1 0.03 0.01; do
            echo "sgd$lr: --n.adam 0 --n.lr $lr"
        done
    } >out/nn/select/grid.txt
    for engine in mlp gru; do
        for task in "echo" "both-not-either" "delayed-echo --delay 2"; do
            python3 scripts/sweep.py --name "$engine ${task%% *}" --seeds 1-4 --jobs 2 \
                --no-twin --variants out/nn/select/grid.txt --out-dir out/nn/select/grid -- \
                --engine "$engine" --task $task $life --alive-ticks 0 --rhythm-ticks 0
        done
    done
    ;;
suite)
    scripts/suite.sh mlp-fixed --engine mlp "$@"
    scripts/suite.sh gru-fixed --engine gru "$@"
    ;;
told)
    scripts/suite.sh mlp-told --engine mlp --n.teach 1 "$@"
    scripts/suite.sh gru-told --engine gru --n.teach 1 "$@"
    ;;
compare)
    exec python3 scripts/reference_compare.py "$@"
    ;;
hold)
    # the gru's wall: delayed echo past its 8-step backprop window, seeds 5-8
    scripts/build.sh --target blobrun
    mkdir -p out/nn/hold
    for d in 4 6 7 10 16; do
        for s in 5 6 7 8; do
            echo "$d $s"
        done
    done | xargs -P 2 -n 2 sh -c 'nice -n 19 ./build/blobrun --engine gru --task delayed-echo \
        --delay $0 '"$life"' --seed $1 --task-seed $1 --alive-ticks 0 --rhythm-ticks 0 \
        --out out/nn/hold/d$0-s$1.csv > out/nn/hold/d$0-s$1.txt 2>&1'
    for d in 4 6 7 10 16; do
        printf "delay %2d  late, seeds 5-8:" "$d"
        for s in 5 6 7 8; do
            awk '$1 == "blob" && $2 ~ /^[0-9]/ { printf " %.2f", $3 }' "out/nn/hold/d$d-s$s.txt"
        done
        echo
    done
    ;;
*)
    sed -n '2,12p' "$0"
    exit 1
    ;;
esac
