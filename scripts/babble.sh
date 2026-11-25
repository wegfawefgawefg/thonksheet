#!/usr/bin/env bash
# Ideas 60 (babbling: the world urges a random button for a whole step now and then) and 61
# (balance: thin the commoner of good and bad). Results:
# docs/results-2026-11-24-babble-and-balance.md
#   scripts/babble.sh tune          choose settings, seeds 1-4 (shuffled echo also 6 7 25 26)
#   scripts/babble.sh summary-tune  print the tuning tables (runs nothing)
#   scripts/babble.sh report        idea 60 on report seeds, all three engines, with twins
#   scripts/babble.sh long          stability: echo k=2, 160,000-step lives
#   scripts/babble.sh balance       idea 61 (and both together): cartpole, catch, echo
#   scripts/babble.sh coin          coin flips on the same tasks and seeds
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
CART="--ticks 20 --sensor-side 2 --motor-side 3 --steps 40000 --window 1000 --number-code place"
LONG="--ticks 20 --sensor-side 3 --motor-side 3 --steps 160000 --window 1000"
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
report)
  for i in 0 1 2; do
    E=${ENGINES[$i]}
    n=${NAMES[$i]}
    sweep "$n shuffled k=2" 11-24 $V/babble.txt -- --task shuffled-echo --k 2 $C $E
    sweep "$n echo k=2" 11-18 $V/babble.txt -- --task echo --k 2 $C $E
    sweep "$n echo k=4" 11-18 $V/babble.txt -- --task echo --k 4 $C $E
    sweep "$n echo k=8" 11-18 $V/babble.txt -- --task echo --k 8 $C $E
    sweep "$n echo 2 tiles apart" 11-18 $V/babble.txt -- --task echo --k 2 --distance 2 $C $E
    sweep "$n go-no-go" 11-18 $V/babble.txt -- --task go-no-go $C $E
  done
  ;;
balance)
  for i in 0 1 2; do
    E=${ENGINES[$i]}
    n=${NAMES[$i]}
    sweep "$n cartpole lean" 11-18 $V/balance.txt -- --task cartpole --cart-reward lean $CART $E
    sweep "$n cartpole sparse" 11-18 $V/balance.txt -- --task cartpole --cart-reward sparse \
      $CART $E
    sweep "$n catch 4x4" 11-18 $V/balance.txt -- --task catch --catch-size 4 4 $C $E
    sweep "$n echo k=2" 11-18 $V/balance.txt -- --task echo --k 2 $C $E
    sweep "$n echo k=4" 11-18 $V/balance.txt -- --task echo --k 4 $C $E
  done
  ;;
long)
  for i in 0 1 2; do
    sweep "${NAMES[$i]} echo k=2, 160k" 11-18 $V/babble.txt --no-twin -- --task echo --k 2 \
      $LONG ${ENGINES[$i]}
  done
  ;;
coin)
  # coin flips (the stand-in) per task and seed; the minimal engine is the cheapest host
  mkdir -p out/babble/coin
  for t in "shuffled-echo --k 2:11-24" "echo --k 2:11-18" "echo --k 4:11-18" \
    "echo --k 8:11-18" "echo --k 2 --distance 2:11-18" "go-no-go:11-18" \
    "catch --catch-size 4 4:11-18"; do
    task=${t%%:*}
    seeds=$(python3 -c "import sys; a, b = sys.argv[1].split('-'); print(*range(int(a), int(b) + 1))" ${t##*:})
    for s in $seeds; do
      f=out/babble/coin/$(echo "$task" | tr -d ' -')-s$s.csv
      [ -f "${f%.csv}-sum.csv" ] || nice -n 19 $BIN --engine minimal --task $task $C --seed $s \
        --out $f >/dev/null
    done
  done
  for r in lean sparse; do
    for s in 11 12 13 14 15 16 17 18; do
      f=out/babble/coin/cartpole$r-s$s.csv
      [ -f "${f%.csv}-sum.csv" ] || nice -n 19 $BIN --engine minimal --task cartpole \
        --cart-reward $r --signal-report 1 $CART --seed $s --out $f >/dev/null
    done
  done
  for f in out/babble/coin/*-s11-sum.csv; do
    stem=${f%-s11-sum.csv}
    printf "%-40s %s\n" "$(basename $stem)" "$(cat $stem-s*-sum.csv | grep '^coin,late,' |
      cut -d, -f3 | awk '{s += $1; n++} END {printf "coin late %.3f over %d seeds", s / n, n}')"
  done
  ;;
summary)
  for i in 0 1 2; do
    E=${ENGINES[$i]}
    n=${NAMES[$i]}
    echo "### $n, shuffled echo k=2"
    split --seeds 11-24 --variants $V/babble.txt -- --task shuffled-echo --k 2 $C $E
    for t in "echo k=2:--task echo --k 2" "echo k=4:--task echo --k 4" \
      "echo k=8:--task echo --k 8" "echo 2 tiles apart:--task echo --k 2 --distance 2" \
      "go-no-go:--task go-no-go"; do
      echo "### $n, ${t%%:*}"
      split --seeds 11-18 --no-split --variants $V/babble.txt -- ${t#*:} $C $E
    done
    echo "### $n, echo k=2, 160,000 steps"
    split --seeds 11-18 --stable --variants $V/babble.txt -- --task echo --k 2 $LONG $E
    for t in "cartpole lean:--task cartpole --cart-reward lean $CART" \
      "cartpole sparse:--task cartpole --cart-reward sparse $CART" \
      "catch 4x4:--task catch --catch-size 4 4 $C" "echo k=2:--task echo --k 2 $C" \
      "echo k=4:--task echo --k 4 $C"; do
      echo "### $n, balance, ${t%%:*}"
      split --seeds 11-18 --signals --variants $V/balance.txt -- ${t#*:} $E
    done
  done
  ;;
*)
  echo "unknown: $what" >&2
  exit 1
  ;;
esac
