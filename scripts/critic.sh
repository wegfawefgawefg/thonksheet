#!/usr/bin/env bash
# The actor and critic (docs/results-2026-11-25-critic.md): a group of critic cells learns
# to fire more where things go well, and its surprise, not raw good and bad, teaches the
# readout. One setting for every world (CRITIC below).
#   scripts/critic.sh life cartpole             one cartpole life (lean reward) beside its twin
#   scripts/critic.sh life cartpole --cart-reward sparse --steps 1000000
#   scripts/critic.sh life chain                 also: bandit, catch, echo, ...
#   scripts/critic.sh life cartpole --no-critic  the same readout without the critic
#   scripts/critic.sh track cartpole --seed 5    does the critic's level know how far a fall
#                                                is? (bins its level by steps to the next fall)
#   scripts/critic.sh tables                     the report tables (scripts/critic_tables.sh)
# Watch it (opens a window, so for the owner, not agents):
#   scripts/watch.sh cartpole --motor-spread 128 --g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --g.critic_on 1 --g.readout_good 1
set -euo pipefail
cd "$(dirname "$0")/.."
READOUT="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1"
CRITIC="$READOUT --g.critic_on 1 --g.readout_good 1"
what="${1:-life}"
[ $# -gt 0 ] && shift
task="${1:-cartpole}"
[ $# -gt 0 ] && shift
genome="$CRITIC"
args=()
for a in "$@"; do
    if [ "$a" = --no-critic ]; then genome="$READOUT"; else args+=("$a"); fi
done
# cartpole's 24 number lines need 2x2 sensor groups, as in scripts/task.sh; catch's 6x6 too
side=3
extra=""
case "$task" in
cartpole) side=2; extra="--cart-reward lean --number-code place" ;;
catch) side=2 ;;
esac
plug="--task $task --ticks 20 --sensor-side $side --motor-side 3 --motor-spread 128 --window 1000 $extra"
case "$what" in
life)
    scripts/build.sh
    exec nice -n 19 ./build/blobrun $plug --steps 40000 $genome "${args[@]}"
    ;;
track)
    scripts/build.sh
    mkdir -p out/critic
    nice -n 19 ./build/blobprobe critic $plug --steps 40000 --log 10000 $genome "${args[@]}" \
        --csv out/critic/track.csv
    nice -n 19 ./build/blobprobe critic $plug --steps 40000 --log 10000 $genome "${args[@]}" \
        --learning 0 --csv out/critic/track-off.csv
    python3 scripts/critic_track.py out/critic/track.csv out/critic/track-off.csv
    ;;
tables)
    scripts/build.sh
    exec scripts/critic_tables.sh "$@"
    ;;
*)
    sed -n '2,15p' "$0"
    exit 1
    ;;
esac
