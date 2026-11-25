#!/usr/bin/env bash
# Watch a blob learn a task in the viewer. Starts slow, because Echo is learned in about
# 3,000 ticks. Space: full speed. W: one task step. E: one tick.
#   scripts/watch.sh                 echo with 2 cues
#   scripts/watch.sh echo --k 4
#   scripts/watch.sh cartpole
#   scripts/watch.sh inv-rotation --inv-motion 50     the invariance battery (docs/curriculum.md)
# Agents must not run this: it opens a window. Use --capture or scripts/offscreen.sh.
set -euo pipefail
cd "$(dirname "$0")/.."
if [ -n "${CLAUDECODE:-}" ] && [ -z "${THONK_ON_SCREEN:-}" ]; then
    echo "watch.sh opens a window on the real display; agents use scripts/offscreen.sh" >&2
    exit 1
fi
scripts/build.sh
task="${1:-echo}"
[ $# -gt 0 ] && shift
# cartpole has 24 number lines, which need smaller line groups; it is run with the reward
# that says "nearer upright and centred" and with each number as one lit line
side=3
extra=""
if [ "$task" = cartpole ]; then
    side=2
    extra="--cart-reward lean --number-code place"
fi
# the invariance battery's pictures and pitch need one sensor cell a line (16x12 bitmap)
case "$task" in
inv-shift | inv-tempo) ;;
inv-*) side=1 ;;
esac
exec ./build/blobview --task "$task" --ticks 20 --sensor-side "$side" --motor-side 3 --window 50 \
    --slow 400 $extra "$@"
