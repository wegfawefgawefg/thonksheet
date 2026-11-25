#!/usr/bin/env bash
# Watch a blob learn a task in the viewer. Starts slow, because Echo is learned in about
# 3,000 ticks. Space: full speed. W: one task step. E: one tick. V: pick a button's cell and
# see its evidence over the sheet. C: tint the sheet by how readable a feature is (X again).
#   scripts/watch.sh                           echo with 2 cues, the full engine
#   scripts/watch.sh echo --k 4
#   scripts/watch.sh cartpole
# The working rule (learn mode 5: evidence counters at the buttons' cells, 128 cells a
# button, the inside of the sheet fixed as born):
#   scripts/watch.sh echo --readout
#   scripts/watch.sh both-not-either --readout          exclusive-or, the evidence piling up
#   scripts/watch.sh order --readout --g.local_delay_min 4 --g.local_delay_max 8
# Other engines behind the same sheet:
#   scripts/watch.sh echo --engine minimal             --m.NAME N sets its numbers
#   scripts/watch.sh echo --table                      table neurons; --t.NAME N
# Readability, the feature probe's measurement live (no task; which, order, xor, gap, hold):
#   scripts/watch.sh --probe order
#   scripts/watch.sh --probe gap --g.local_delay_min 4 --g.local_delay_max 8
# Anything after the task goes to blobview as is (blobview --help).
#   scripts/watch.sh bandit          any world of scripts/task.sh (track, forage, pendulum, ...)
# Agents must not run this: it opens a window. Use --capture or scripts/offscreen.sh.
set -euo pipefail
cd "$(dirname "$0")/.."
if [ -n "${CLAUDECODE:-}" ] && [ -z "${THONK_ON_SCREEN:-}" ]; then
    echo "watch.sh opens a window on the real display; agents use scripts/offscreen.sh" >&2
    exit 1
fi
scripts/build.sh

# --probe FEATURE: no task, the probe is the world
if [ "${1:-}" = --probe ]; then
    exec ./build/blobview "$@"
fi

task="${1:-echo}"
[ $# -gt 0 ] && shift
# cartpole has 24 number lines, which need smaller line groups; it is run with the reward
# that says "nearer upright and centred" and with each number as one lit line. mountain car
# and pendulum have 16 number lines, forage a 5x5 bitmap: smaller groups too
side=3
extra=""
if [ "$task" = cartpole ]; then
    side=2
    extra="--cart-reward lean --number-code place"
fi
if [ "$task" = catch ]; then
    side=2
fi
# --readout: the working rule, as scripts/readout.sh's BEST
readout="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --motor-spread 128"
args=()
window=50
for a in "$@"; do
    if [ "$a" = --readout ]; then
        extra="$extra $readout"
        window=200
    else
        args+=("$a")
    fi
done
exec ./build/blobview --task "$task" --ticks 20 --sensor-side "$side" --motor-side 3 \
    --window "$window" --slow 400 $extra "${args[@]}"
case "$task" in
cartpole) side=2; extra="--cart-reward lean --number-code place" ;;
mountain-car | pendulum) side=2; extra="--number-code place" ;;
forage) side=2 ;;
esac
exec ./build/blobview --task "$task" --ticks 20 --sensor-side "$side" --motor-side 3 --window 50 \
    --slow 400 $extra "$@"
