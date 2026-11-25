#!/usr/bin/env bash
# The vm engine (idea 135, the owner's vvm in bits; src/vm/,
# docs/results-2026-11-25-vm-route.md): a state that looks up its next operations by content.
#
#   scripts/vm.sh task delayed-echo --delay 20      one life, beside learning off and coin flips
#   scripts/vm.sh task echo --k 8 --seed 5 --v.active 32       --v.name value sets a vm number
#   scripts/vm.sh probe delayed-echo --delay 2      learn, then print the walks through the bank:
#                                                   which entries fire, how alike trials are,
#                                                   how the gap is held (fixed point/cycle/chain)
#   scripts/vm.sh probe order --show 6 --test-steps 4000
#   scripts/vm.sh rows mytag hold,cues,steps 5-12   named rows on seeds 5..12, two at a time,
#                                                   then mean, spread, learning off, coin
#   scripts/vm.sh suite                             the whole bench suite (scripts/suite.sh vm)
#
# Groups for rows: hold (delayed echo, gaps 0 1 2 5 20, and 8 cues at gap 2), cues (echo with
# 2 8 16 32 cues, shuffled), steps (order, gap, both-not-either, two-step), time (beat),
# beyond (distract 1 2 4 8, serial-add, majority, place: what a lookup cannot do).
# Another engine on the same rows: add --engine memory or --engine gru at the end.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh
what="${1:-task}"
[ $# -gt 0 ] && shift
case "$what" in
task)
    task="${1:-echo}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/blobrun --engine vm --task "$task" --ticks 20 --sensor-side 3 \
        --motor-side 3 --steps 40000 --window 1000 "$@"
    ;;
probe)
    task="${1:-delayed-echo}"
    [ $# -gt 0 ] && shift
    exec nice -n 19 ./build/vmprobe --task "$task" --ticks 20 --sensor-side 3 --motor-side 3 \
        --steps 20000 "$@"
    ;;
rows)
    exec python3 scripts/vm_rows.py "$@"
    ;;
suite)
    exec scripts/suite.sh vm --engine vm "$@"
    ;;
*)
    sed -n '2,18p' "$0"
    exit 1
    ;;
esac
