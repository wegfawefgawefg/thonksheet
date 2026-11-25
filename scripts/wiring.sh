#!/usr/bin/env bash
# Wiring at birth (ideas 19-21, 77-80, 83): layers inside a tile, fine locality, fewer local
# tiles, mirror links, hubs, streams, far-link topography, inhibitory reach, two sheets.
# Each is a wire_* genome number; all 0 is today's wiring. Results:
# docs/results-2026-11-25-family-wiring.md
#
#   scripts/wiring.sh stats --g.wire_layers 2       what the wiring looks like, and alive
#   scripts/wiring.sh stats --g.wire_mirror_percent 50 --age 600000    the same after a life
#   scripts/wiring.sh screen                        every variant, seeds 1-4, learning off
#   scripts/wiring.sh life                          the best few after 600,000 ticks learning on
#   scripts/wiring.sh search                        500 random sheets, old and new numbers
#   scripts/wiring.sh report                        what the search found
#   scripts/wiring.sh readout                       the readout rule, frozen sheet, seeds 5-12
# Everything runs under nice, 2 at a time; the screen and search cache what they have done.
set -euo pipefail
cd "$(dirname "$0")/.."
what="${1:-stats}"
[ $# -gt 0 ] && shift
[ "$what" = report ] || scripts/build.sh --target blobprobe blobrun
SEARCH="--name wiring --features which,order,xor,gap,hold20"
LIFE="--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000"

case "$what" in
stats)
    exec nice -n 19 ./build/blobprobe wiring --g.learning_on 0 "$@"
    ;;
screen)
    exec python3 scripts/wiring_screen.py --variants scripts/variants/wiring-screen.txt \
        --seeds 1-4 --jobs 2 --name screen "$@"
    ;;
life)
    exec python3 scripts/wiring_screen.py --variants scripts/variants/wiring-life.txt \
        --seeds 1-4 --jobs 2 --name life --life 600000 "$@"
    ;;
search)
    exec python3 scripts/richness_search.py $SEARCH run --samples 500 --jobs 2 "$@"
    ;;
report)
    python3 scripts/richness_search.py $SEARCH report "$@"
    ;;
check)
    exec python3 scripts/richness_search.py $SEARCH check --top 10 --seeds 2,3,4 --jobs 2 "$@"
    ;;
readout)
    sw() { python3 scripts/sweep.py --jobs 2 --seeds 5-12 --out-dir out/wiring/sweep \
        --variants scripts/variants/wiring-readout.txt "$@"; }
    sw --name "order" -- --task order $LIFE
    sw --name "both-not-either" -- --task both-not-either $LIFE
    sw --name "gap" -- --task gap $LIFE
    sw --name "echo k=2, 3 tiles" -- --task echo --distance 3 $LIFE
    sw --name "echo k=2, 5 tiles" -- --task echo --distance 5 $LIFE
    sw --name "echo k=8" -- --task echo --k 8 $LIFE
    sw --name "echo k=16" -- --task echo --k 16 $LIFE
    ;;
*)
    sed -n '2,16p' "$0"
    exit 1
    ;;
esac
