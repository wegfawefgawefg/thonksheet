#!/usr/bin/env bash
# The actor-critic report tables (docs/results-2026-11-25-critic.md): every acting world,
# the readout alone against the readout with the critic, each beside its learning-off twin.
#   scripts/critic_tables.sh                     40,000-step lives, seeds 5-12, 2 at a time
#   SEEDS=1-4 scripts/critic_tables.sh           the tuning seeds
#   STEPS=1000000 WINDOW=1000 scripts/critic_tables.sh   the million-step lives (use the mini)
#   WORLDS="lean sparse" scripts/critic_tables.sh
# Tables go to stdout; runs are cached in out/critic/report/ so a rerun only does what is missing.
set -euo pipefail
cd "$(dirname "$0")/.."
SEEDS="${SEEDS:-5-12}"
STEPS="${STEPS:-40000}"
JOBS="${JOBS:-2}"
V="${VARIANTS:-scripts/variants/critic.txt}"
P="--ticks 20 --motor-side 3 --steps $STEPS --window 1000 --motor-spread 128"
sw() {
    python3 scripts/sweep.py --jobs "$JOBS" --out-dir out/critic/report --name "$1" \
        --seeds "$SEEDS" --variants "$V" -- "${@:2}"
}
for w in ${WORLDS:-bandit chain lean alive sparse catch}; do
    case $w in
    bandit) sw bandit --task bandit --sensor-side 3 $P ;;
    chain) sw chain --task chain --sensor-side 3 $P ;;
    lean) sw "cartpole lean" --task cartpole --cart-reward lean --number-code place --sensor-side 2 $P ;;
    alive) sw "cartpole alive" --task cartpole --cart-reward alive --number-code place --sensor-side 2 $P ;;
    sparse) sw "cartpole sparse" --task cartpole --cart-reward sparse --number-code place --sensor-side 2 $P ;;
    catch) sw catch --task catch --sensor-side 2 $P ;;
    esac
done
