#!/usr/bin/env bash
# Idea 155: one inner rule from tags and reverse replay. docs/results-2026-11-26-inner-rule.md
#
# The rule: after an answer, the chosen button's cells against the other buttons' send one
# step of credit back to the inner neurons whose spikes drove theirs (spike-exact, as the
# walk), a neuron is marked when its net credit reaches 4 (as the tags), the verdict moves
# marked neurons' hidden counters for the sources that drove their own spikes, and each
# neuron rewires at its born counts of excitatory, inhibitory and strong connections;
# inhibitory neurons do not learn. On the best readout (learn mode 5, evidence counters,
# 128 motor cells per button; 256 over all buttons for 8 and 16 cues).
#
# One life (anything after the task overrides):
#   scripts/inner.sh life both-not-either
#   scripts/inner.sh life echo --distance 5
# Did the inside learn? A fresh output on the frozen lived sheet against one on the born
# sheet (seed 5 unless --seed is given; the second task defaults to the first):
#   scripts/inner.sh transfer both-not-either
#   scripts/inner.sh transfer both-not-either "--task points --fn parity --fn-lines 3"
# MNIST: a linear reader of the sheet as born, after a life of 20,000 digits with the rule,
# after the same life with the readout alone, and after one where good and bad were a coin:
#   scripts/inner.sh mnist
# The tables in the results doc (sweeps of 2 runs at a time under nice; hours):
#   scripts/inner.sh tune                  choosing the setting, seeds 1-4 (tune2: round 2)
#   scripts/inner.sh tables                against both parents, seeds 5-12
#   scripts/inner.sh pieces                each piece switched off, and the two controls
#   ROWS="xor echo5" JOBS=2 scripts/inner.sh tables      some rows only
#   scripts/inner.sh long                  160,000-step lives, seeds 5-8
set -euo pipefail
cd "$(dirname "$0")/.."
[ -n "${NO_BUILD:-}" ] || scripts/build.sh --target blobprobe blobrun blobmnist
what="${1:-life}"
[ $# -gt 0 ] && shift
T="--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000"
READOUT="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1"
INNER="--g.tag_mode 2 --g.tag_inh 0 --g.tag_min 4 --g.tag_exact 1"
OUT="${OUT:-out/inner}"
sw() { python3 scripts/sweep.py --jobs "${JOBS:-2}" --seeds "${SEEDS:-5-12}" \
    --out-dir "$OUT/sweep" "$@"; }
# one row of a table: its name and its task flags; many-button rows use 256 cells
row() {
    local r="$1" one="$2" many="$3"
    case $r in
    xor) sw --name xor --variants "$one" -- --task both-not-either $T ;;
    order) sw --name order --variants "$one" -- --task order $T ;;
    gap) sw --name gap --variants "$one" -- --task gap $T ;;
    echo[0-9]) sw --name "echo, ${r#echo} tiles" --variants "$one" -- \
        --task echo --k 2 --distance "${r#echo}" $T ;;
    k[0-9]*) sw --name "echo k=${r#k}" --variants "$many" -- --task echo --k "${r#k}" $T ;;
    parity3) sw --name "parity of 3" --variants "$one" -- \
        --task points --fn parity --fn-lines 3 $T ;;
    shuffled) SEEDS="${SHUFFLED_SEEDS:-5-20}" sw --name "shuffled echo" --variants "$many" -- \
        --task shuffled-echo --k 2 $T ;;
    delayed) sw --name "delayed echo" --variants "$one" -- --task delayed-echo --delay 2 $T ;;
    long3 | long5) sw --name "echo, ${r#long} tiles, 160,000 steps" --variants "$one" -- \
        --task echo --k 2 --distance "${r#long}" --ticks 20 --sensor-side 3 --motor-side 3 \
        --steps 160000 --window 4000 ;;
    esac
}
case "$what" in
life)
    task="${1:-both-not-either}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/blobrun --task "$task" $T --motor-spread 128 $READOUT $INNER "$@"
    ;;
transfer)
    task="${1:-both-not-either}"
    then="${2:---task $task}"
    shift $(($# < 2 ? $# : 2))
    for born in 1 0; do
        nice -n 19 ./build/blobprobe transfer --life "--task $task $T --motor-spread 128 $READOUT" \
            $INNER --then-flags "$then $T --motor-spread 128" --then-steps 20000 --seed 5 \
            --born $born "$@"
    done
    ;;
mnist)
    [ -f data/mnist/t10k-labels-idx1-ubyte ] || scripts/get_mnist.sh
    SHEET="--mnist-code t28 --g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1 \
--g.motor_w 64 --g.motor_h 64 --motor-spread 102 --g.local_connect_shift 4"
    RULE="--g.learn_mode 5 --g.readout_evidence 2 --g.readout_evidence_strong 0 \
--g.readout_evidence_bits 8 --g.readout_shift 6 --g.readout_balance 1 --g.readout_good 1 \
--g.margin_percent 20 --g.margin_win 2"
    READER="--scale 2 --passes 10 --lr-1000 1 --fit 60000"
    c() { nice -n 19 ./build/blobmnist ceiling $SHEET $RULE $READER "$@"; }
    c "$@" | grep -E "^ceiling|left out|bits alone"
    c --live 20000 $INNER "$@" | grep -E "^live|left out"
    c --live 20000 "$@" | grep -E "^live|left out"
    c --live 20000 --live-coin 1 $INNER "$@" | grep -E "^live|left out"
    ;;
tune)
    for r in ${ROWS:-xor order echo2 echo5 k8}; do
        SEEDS="${SEEDS:-1-4}" row "$r" scripts/variants/inner-tune.txt \
            scripts/variants/inner-tune-many.txt
    done
    ;;
tune2)
    # the second round: the walk's verdict and growable drive on top of spike-exact credit
    for r in ${ROWS:-xor order echo2 echo5 k8}; do
        SEEDS="${SEEDS:-1-4}" row "$r" scripts/variants/inner-tune2.txt \
            scripts/variants/inner-tune2-many.txt
    done
    ;;
tables)
    for r in ${ROWS:-xor order gap echo2 echo3 echo5 k8 k16 parity3 shuffled delayed}; do
        row "$r" scripts/variants/inner-report.txt scripts/variants/inner-report-many.txt
    done
    ;;
pieces)
    for r in ${ROWS:-xor order echo2 echo5 k8 parity3}; do
        row "$r" scripts/variants/inner-pieces.txt scripts/variants/inner-pieces-many.txt
    done
    ;;
long)
    for r in ${ROWS:-long3 long5}; do
        SEEDS="${SEEDS:-5-8}" row "$r" scripts/variants/inner-report.txt ""
    done
    ;;
*)
    sed -n '2,29p' "$0"
    exit 1
    ;;
esac
