#!/usr/bin/env bash
# One long life on a long-memory world (docs/curriculum.md "Long memory"), headless, with
# coin flips and a perfect player beside it. Prints each bucket (an age, teaching, common
# or rare, kept or changed) over the whole life and before/after the split, and for pairs
# and once how many pairs are held at once. Rows per window go to the csv.
#   scripts/longlife.sh pairs                      table neurons, 10,000,000 steps
#   scripts/longlife.sh pairs blob --seed 5        the full engine with the evidence readout
#   scripts/longlife.sh pairs minimal --steps 2000000
#   scripts/longlife.sh hold table --hold-steps 50 --steps 200000
#   scripts/longlife.sh hold table --hold-steps 50 --hold-noise 1   distractors in the gap
#   scripts/longlife.sh once table ; scripts/longlife.sh rare table ; scripts/longlife.sh drift table
#   scripts/longlife.sh pairs table --t.lock_levels 3              the freezer
#   scripts/longlife.sh hold table --t.timeline_levels 10 --t.timeline_percent 10
# Engines: table (default), blob (full engine: --g.learn_mode 5 with the evidence readout),
# blob-plain (the full engine as born, learn_mode 0), minimal. Add --baselines for the
# learning-off twin (it costs as much as the life). Speed: a 10M-step life is 200M ticks;
# table neurons about an hour here, the full engine longer; run long ones on stevejobs
# (scripts/mini.sh).
set -euo pipefail
cd "$(dirname "$0")/.."
world="${1:-pairs}"
engine="${2:-table}"
[ $# -gt 0 ] && shift
[ $# -gt 0 ] && shift
[ -x build/blobrun ] || scripts/build.sh
# hold needs few lines; the pattern worlds need 32, so their lines are 2x2 cells
side=2
steps=10000000
window=100000
if [ "$world" = hold ]; then
    side=3
    steps=400000
    window=10000
fi
case "$engine" in
table) eng="--table" ;;
blob) eng="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --motor-spread 128" ;;
blob-plain) eng="" ;;
minimal) eng="--engine minimal" ;;
*) echo "engine is table, blob, blob-plain or minimal" >&2; exit 1 ;;
esac
base="--no-baselines"
args=()
for a in "$@"; do
    if [ "$a" = --baselines ]; then base=""; else args+=("$a"); fi
done
seed=1
for ((i = 0; i < ${#args[@]}; i++)); do
    [ "${args[$i]}" = --seed ] && seed="${args[$((i + 1))]}"
done
mkdir -p out/longlife
exec nice -n 19 ./build/blobrun --long --task "$world" --ticks 20 --sensor-side "$side" \
    --motor-side 3 --steps "$steps" --window "$window" $eng $base \
    --out "out/longlife/$world-$engine-s$seed.csv" "${args[@]}"
