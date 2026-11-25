#!/usr/bin/env bash
# The output gap on MNIST (docs/results-2026-11-25-output-gap.md): why the sheet's own output
# reads 0.71 where a linear reader of the same cells reads 0.93 to 0.95.
#
#   scripts/outgap.sh ladder NAME [--seeds 5-12] [--jobs 3] [flags]
#       the reader ladder: record the sheet's spike counts for 60,000 training digits and the
#       10,000 test digits once, then run readers one handicap at a time (rungs 1 to 6),
#       online ones checked at 1,000 to 120,000 digits. Flags go to blobmnist ladder, e.g.
#       --life 60000 --g.compete_on 1 --g.compete_shift 5   (the lived sheet)
#   scripts/outgap.sh online NAME [--seeds 5-12] [--jobs 3] [flags]
#       rung 7: the engine's own readout (scripts/mnist.sh online's sheet and rule) checked at
#       1,000, 5,000, 20,000, 60,000 and 120,000 digits on all 10,000 test digits
#   scripts/outgap.sh table             every finished run, one line per name and rung
#
# Every run here has --g.tile_winners 4 (at most 4 cells of a tile keep their spike), the
# setting competitive learning uses, so born and lived sheets are the same sheet. Results in
# out/output-gap/NAME/seedS.txt; a finished run is not run again.
#   scripts/outgap.sh online born                                   the plain online readout
#   scripts/outgap.sh online gate --g.margin_percent 20 --g.margin_win 2
#   scripts/outgap.sh online compete --g.compete_on 1 --g.compete_shift 5
#   scripts/outgap.sh online lived --life 60000 --life-freeze 1 --g.compete_on 1 --g.compete_shift 5
#   scripts/outgap.sh ladder born
#   scripts/outgap.sh ladder lived --life 60000 --g.compete_on 1 --g.compete_shift 5
set -euo pipefail
cd "$(dirname "$0")/.."
SHEET="--mnist-code t28 --g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1 \
--g.motor_w 64 --g.motor_h 64 --motor-spread 102 --g.local_connect_shift 4 --g.tile_winners 4"
RULE="--g.learn_mode 5 --g.readout_evidence 2 --g.readout_evidence_strong 0 \
--g.readout_evidence_bits 8 --g.readout_shift 6 --g.readout_balance 1 --g.readout_good 1"
CHECKS="--checks 1000,5000,20000,60000,120000"
what="${1:-}"
if [ "$what" = table ]; then
    exec python3 scripts/outgap_table.py out/output-gap
fi
if [ "$what" != ladder ] && [ "$what" != online ] || [ $# -lt 2 ]; then
    sed -n '2,24p' "$0"
    exit 1
fi
name="$2"
shift 2
seeds="5-12"
jobs=3
flags=()
while [ $# -gt 0 ]; do
    case "$1" in
    --seeds) seeds="$2"; shift 2 ;;
    --jobs) jobs="$2"; shift 2 ;;
    *) flags+=("$1"); shift ;;
    esac
done
[ -f data/mnist/t10k-labels-idx1-ubyte ] || scripts/get_mnist.sh
[ -n "${NO_BUILD:-}" ] || scripts/build.sh --target blobmnist >/dev/null
out="out/output-gap/$what-$name"
mkdir -p "$out"
echo "$SHEET $RULE ${flags[*]}" >"$out/flags.txt"
if [[ "$seeds" == *-* ]]; then
    seeds=$(seq "${seeds%-*}" "${seeds#*-}")
else
    seeds=${seeds//,/ }
fi
for s in $seeds; do
    f="$out/seed$s.txt"
    if [ -f "$f" ] && grep -q "total wall" "$f"; then
        continue
    fi
    if [ "$what" = online ]; then
        echo "nice -n 19 ./build/blobmnist online $SHEET $RULE $CHECKS --seed $s ${flags[*]} >$f 2>&1"
    else
        echo "nice -n 19 ./build/blobmnist ladder $SHEET $RULE --seed $s ${flags[*]} >$f 2>&1"
    fi
done >"$out/jobs.txt"
# at most $jobs at once (a plain loop: the Mac's xargs cannot take lines this long)
while read -r cmd; do
    while [ "$(jobs -rp | wc -l)" -ge "$jobs" ]; do
        sleep 5
    done
    sh -c "$cmd" &
done <"$out/jobs.txt"
wait
echo "done: $out"
