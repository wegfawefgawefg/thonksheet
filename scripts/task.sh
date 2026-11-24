#!/usr/bin/env bash
# Run one blob on one task for a whole life, headless, next to its baselines.
#   scripts/task.sh                    echo with 2 cues
#   scripts/task.sh echo --k 4         echo with 4 cues
#   scripts/task.sh shuffled-echo      other tasks: delayed-echo go-no-go beat sequence catch cartpole
#   scripts/task.sh echo --seed 3 --task-seed 3
# Prints the score early and late in the life for the blob, the same blob with learning
# off, coin flips and a perfect player. Score per window goes to out/<task>-s<seed>.csv.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh
task="${1:-echo}"
[ $# -gt 0 ] && shift
exec nice -n 19 ./build/blobrun --task "$task" --ticks 20 --sensor-side 3 --motor-side 3 \
    --steps 40000 --window 1000 "$@"
