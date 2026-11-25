#!/usr/bin/env bash
# Run one blob on one task for a whole life, headless, next to its baselines.
#   scripts/task.sh                    echo with 2 cues
#   scripts/task.sh echo --k 4         echo with 4 cues
#   scripts/task.sh shuffled-echo      other tasks: delayed-echo go-no-go beat sequence catch cartpole
#   scripts/task.sh echo --seed 3 --task-seed 3
# More worlds (idea 63; docs/curriculum.md, "More worlds"):
#   scripts/task.sh bandit                       no cue; one of 2 buttons pays 80%, the other 20%
#   scripts/task.sh bandit --k 4                 4 buttons
#   scripts/task.sh switch-bandit                the paying button moves every 1,000 steps
#   scripts/task.sh chain                        press A then B to be paid; a line shows the stage
#   scripts/task.sh chain --chain-cue 0          the same, stage not shown
#   scripts/task.sh track                        walk a marker onto a target; nearer good, further bad
#   scripts/task.sh track --track-reward arrive  paid only on arrival
#   scripts/task.sh forage                       5x5 wrapped grid, walk onto the food; warmer/colder
#   scripts/task.sh forage --forage-warmer 0     paid only on reaching the food
#   scripts/task.sh mountain-car                 rock the car out; energy gained is good
#   scripts/task.sh mountain-car --classic-reward goal
#   scripts/task.sh pendulum                     swing it up and hold it; energy, then upright
#   scripts/task.sh cartpole --cart-start wide   starts drawn from a wider range
#   scripts/task.sh cartpole --cart-reward relative   good when nearer upright than of late
#   scripts/task.sh cartpole --cart-integrator semi   a push moves the pole the same step, so a
#                                                     step's reward judges its own press
# Any engine or rule: --engine minimal, --table, or genome numbers (--g.name value).
# A long life: --long is 1,000,000 steps in 100 windows (the csv stays 100 lines):
#   scripts/task.sh cartpole --long --engine minimal
# Prints the score early and late in the life for the blob, the same blob with learning
# off, coin flips and a perfect player, and episodes per life (falls, dots, arrivals).
# Score per window goes to out/<task>-s<seed>.csv. The whole ladder: scripts/ladder.py.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh
task="${1:-echo}"
[ $# -gt 0 ] && shift
# cartpole has 24 number lines, which need smaller line groups; it is run with the reward
# that says "nearer upright and centred" and with each number as one lit line. mountain car
# and pendulum have 16 number lines, forage a 5x5 bitmap: smaller groups too
side=3
extra=""
case "$task" in
cartpole) side=2; extra="--cart-reward lean --number-code place" ;;
mountain-car | pendulum) side=2; extra="--number-code place" ;;
forage) side=2 ;;
esac
args=()
for a in "$@"; do
    if [ "$a" = --long ]; then
        args+=(--steps 1000000 --windows 100)
    else
        args+=("$a")
    fi
done
exec nice -n 19 ./build/blobrun --task "$task" --ticks 20 --sensor-side "$side" --motor-side 3 \
    --steps 40000 --window 1000 $extra "${args[@]}"
