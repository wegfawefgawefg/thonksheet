#!/usr/bin/env bash
# Ideas 66-75: what a connection does over a few ticks and what a neuron computes.
# Results: docs/results-2026-11-25-family-dynamics.md. Every mechanism is a genome number,
# 0 = off: syn_tire_ticks, syn_warm_ticks, tire_jump, slow_leak_shift, rebound_level,
# updown_drop, any_branch_percent, veto_percent, coincide_percent, tile_winners.
#
# The screen: feature probe (which, order, xor, gap, hold 20/60/150) on the sheet as born,
# seeds 1-4, each mechanism at 3-4 strengths, plus an aliveness life; then the table.
#   scripts/dynamics.sh screen            (about 1,300 runs; 2 at a time here)
#   scripts/dynamics.sh table
# The random search over the old and new numbers together, then which numbers matter:
#   scripts/dynamics.sh search --samples 300
#   scripts/dynamics.sh report
#   scripts/dynamics.sh check --top 10 --seeds 2,3
# The readout rule on a frozen sheet with and without a mechanism (seeds 5-12):
#   scripts/dynamics.sh readout                 every task in the results doc
#   scripts/dynamics.sh life delayed-echo --delay 1 --g.slow_leak_shift 8 --g.slow_share_shift 6
# Cost: ticks per second (learning off and on) and bytes per neuron, each mechanism on:
#   scripts/dynamics.sh cost
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobprobe blobrun blobbench
what="${1:-table}"
[ $# -gt 0 ] && shift
FEATURES=which,order,xor,gap,hold20,hold60,hold150
LIFE="--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000"
BEST="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --motor-spread 128"
case "$what" in
screen)
    exec python3 scripts/dynamics_screen.py run "$@"
    ;;
table)
    exec python3 scripts/dynamics_screen.py table "$@"
    ;;
search)
    exec python3 scripts/richness_search.py --name dynamics --features $FEATURES run "$@"
    ;;
report | check)
    exec python3 scripts/richness_search.py --name dynamics --features $FEATURES "$what" "$@"
    ;;
life)
    task="${1:-delayed-echo}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/blobrun --task "$task" $LIFE $BEST "$@"
    ;;
readout)
    sw() { python3 scripts/sweep.py --jobs 2 --seeds 5-12 --out-dir out/dynamics/sweep "$@"; }
    v=scripts/variants/dynamics-readout.txt
    sw --name "order" --variants $v -- --task order $LIFE
    sw --name "both-not-either" --variants $v -- --task both-not-either $LIFE
    sw --name "gap" --variants $v -- --task gap $LIFE
    for d in 0 1 2; do
        sw --name "delayed echo, delay $d" --variants $v -- --task delayed-echo --delay $d $LIFE
    done
    sw --name "echo k=8" --variants $v -- --task echo --k 8 $LIFE
    ;;
cost)
    while read -r name flags; do
        off=$(nice -n 19 ./build/blobbench speed --tiles 8 --learn 0 --seconds-ms 1500 $flags | tail -1 | awk '{print $3}')
        on=$(nice -n 19 ./build/blobrun --speed --speed-ms 1500 --out out/dynamics/cost.csv $flags | grep "8x8" | awk '{print $5, $10}')
        echo "$name: learning off $off ticks/s; learning on, ticks/s and bytes/neuron: $on"
    done <<'EOF'
default
syn-tire --g.syn_tire_ticks 4
syn-warm --g.syn_warm_ticks 4
tire-neuron --g.tire_jump 4
slow --g.slow_leak_shift 8 --g.slow_share_shift 6
rebound --g.rebound_level 16
updown --g.updown_drop 4
any-branch --g.any_branch_percent 50
veto --g.veto_percent 50
coincide --g.coincide_percent 50
winners --g.tile_winners 4
default
EOF
    ;;
*)
    sed -n '2,22p' "$0"
    exit 1
    ;;
esac
