#!/usr/bin/env bash
# MNIST through the sheet, against plain learners on the same bits, timed on this machine.
# Results: docs/results-2026-11-25-mnist.md. Every number is test accuracy on the 10,000
# test digits unless --val 1 is given (then the last 10,000 training digits, and training
# uses only the first 50,000).
#
#   scripts/mnist.sh                    everything below in turn, then the table
#   scripts/mnist.sh baselines          linear, two-layer (100 hidden, one pass), random
#                                       projection; on the bits the sheet is shown
#   scripts/mnist.sh ceiling            a linear reader of the never-taught sheet's spikes
#   scripts/mnist.sh online             the sheet's own readout, learning from good and bad
#   scripts/mnist.sh online --seed 6 --checks 5000,60000      anything after overrides
#   scripts/mnist.sh watch              watch digits land on the sheet (opens a window)
#
# The sheet: 4,096 neurons, the 28x28 digit at half ink on every second cell of the whole
# sheet, motor cells on the odd cells between (102 per button), sparser born wiring. The
# rule: learn mode 5 (only the motor cells' inputs move), hidden 8-bit evidence counters,
# sources wired at strength 1, good also moves (the right button gains), ten buttons
# balanced. One digit is 20 ticks. Takes about 15 minutes, one core.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || scripts/get_mnist.sh
scripts/build.sh --target blobmnist blobrun >/dev/null
SHEET="--mnist-code t28 --g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1 \
--g.motor_w 64 --g.motor_h 64 --motor-spread 102 --g.local_connect_shift 4"
RULE="--g.learn_mode 5 --g.readout_evidence 2 --g.readout_evidence_strong 0 \
--g.readout_evidence_bits 8 --g.readout_shift 6 --g.readout_balance 1 --g.readout_good 1"
READER="--scale 2 --passes 10 --lr-1000 1"
out=out/mnist/run
mkdir -p "$out"
what="${1:-all}"
[ $# -gt 0 ] && shift
run() { nice -n 19 ./build/blobmnist "$@"; }
case "$what" in
baselines)
    run baselines --mnist-code t28 "$@" | tee "$out/baselines.txt"
    ;;
ceiling)
    run ceiling $SHEET $READER --fit 60000 --shift 1 "$@" | tee "$out/ceiling.txt"
    ;;
online)
    run online $SHEET $RULE --checks 1000,5000,20000,60000,120000,180000 --check-seconds 60 \
        --shift 1 "$@" | tee "$out/online.txt"
    ;;
watch)
    exec scripts/watch.sh mnist --sensor-side 1 $SHEET $RULE "$@"
    ;;
all)
    "$0" baselines "$@"
    "$0" ceiling "$@"
    "$0" online "$@"
    echo
    echo "test accuracy (moved 1 and 2 pixels: mean over right, left, down, up)"
    grep -E "^(linear|two-layer|random)" "$out/baselines.txt"
    grep -E "left out|bits alone" "$out/ceiling.txt" | sed 's/^/ceiling: /'
    grep -E "^ +[0-9]+ +[0-9.]+ +0\.|moved:" "$out/online.txt" | cut -c1-55 | sed 's/^/online: /'
    ;;
*)
    sed -n '2,20p' "$0"
    exit 1
    ;;
esac
