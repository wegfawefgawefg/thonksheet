#!/usr/bin/env bash
# Ideas 60 (babbling: the world urges a random button for a whole step now and then) and 61
# (balance: thin the commoner of good and bad). Results:
# docs/results-2026-11-24-babble-and-balance.md
#   scripts/babble.sh tune          choose settings, seeds 1-4 (shuffled echo also 6 7 25 26)
#   scripts/babble.sh summary-tune  print the tuning tables (runs nothing)
#   scripts/babble.sh report        idea 60 on report seeds, all three engines, with twins
#   scripts/babble.sh long          stability: echo k=2, 160,000-step lives
#   scripts/babble.sh balance       idea 61 (and both together): cartpole, catch, echo
#   scripts/babble.sh summary       print the report tables (runs nothing)
# One life by hand:
#   build/blobrun --task shuffled-echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 \
#     --steps 40000 --window 1000 --babble 128 --babble-fire 64
#   build/blobrun --task cartpole --cart-reward lean --number-code place --ticks 20 \
#     --sensor-side 2 --motor-side 3 --steps 40000 --window 1000 --balance 1
# Runs are 2 at a time under nice, from a copy of blobrun (out/babble/blobrun-sweep) so a
# rebuild does not change a sweep under way. Cache: out/babble/sweep/.
set -euo pipefail
cd "$(dirname "$0")/.."
what=${1:-summary}
BIN=out/babble/blobrun-sweep
OUT=out/babble/sweep
V=scripts/variants
C="--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000"
ENGINES=("" "--engine minimal" "--table")
NAMES=(blob minimal table)
if [[ $what != summary* ]] && [ ! -x $BIN ]; then
  scripts/build.sh --target blobrun
  mkdir -p out/babble
  cp build/blobrun $BIN
fi
sweep() { # name seeds variants [--no-twin] -- blobrun args
  local name=$1 seeds=$2 variants=$3
  shift 3
  local extra=()
  if [ "$1" = --no-twin ]; then
    extra=(--no-twin)
    shift
  fi
  python3 scripts/sweep.py --jobs 2 --blobrun $BIN --out $OUT "${extra[@]}" --name "$name" \
    --seeds "$seeds" --variants "$variants" "$@"
}
split() { # [opts] -- blobrun args
  python3 $V/babble_split.py --cache $OUT/cache.jsonl "$@"
}
case $what in
tune)
  for i in 0 1 2; do
    E=${ENGINES[$i]}
    sweep "${NAMES[$i]} shuffled tune" 1-4,6,7,25,26 $V/babble-tune.txt --no-twin -- \
      --task shuffled-echo --k 2 $C $E
    sweep "${NAMES[$i]} echo2 tune" 1-4 $V/babble-tune.txt --no-twin -- --task echo --k 2 $C $E
  done
  ;;
summary-tune)
  for i in 0 1 2; do
    E=${ENGINES[$i]}
    echo "### ${NAMES[$i]}, shuffled echo k=2, tuning seeds"
    split --seeds 1-4,6,7,25,26 --variants $V/babble-tune.txt -- \
      --task shuffled-echo --k 2 $C $E
    echo "### ${NAMES[$i]}, echo k=2, tuning seeds"
    split --seeds 1-4 --no-split --variants $V/babble-tune.txt -- --task echo --k 2 $C $E
  done
  ;;
*)
  echo "unknown: $what" >&2
  exit 1
  ;;
esac
