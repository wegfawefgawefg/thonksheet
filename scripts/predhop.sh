#!/usr/bin/env bash
# Idea 164, prediction errors down the one-hop wires (docs/results-2026-11-25-predhop.md).
# Each input line's horizon-1 prediction error (idea 162's predictor cells) becomes an error
# line of idea 159's one-hop rule, so every hidden cell is taught by what the sheet failed to
# predict. One blobprobe predict run per world, arm and seed: a 20,000-step life on the
# prediction battery's sheet (scripts/predict.py), freeze, then read the same world afresh
# with moves from a coin. Each run gives:
#   the future   a fresh ridge reader of every predicted line 1 and 8 steps on, from the
#                hidden cells, from the lines, and from both (the test: does the inside come
#                to hold more of the future than the lines?)
#   the inside   which way the paddle moved, time since the cue, last press, beat phase, the
#                symbol before, from hidden cells only, with the firing rate
#   the life     the predictor cells' skill, the task's late score, ticks per second, and
#                what the rule did
# Arms: born; pred (the rule off: only motor cells learn); hop (prediction errors only, no
# reward reaches the inside); coin (the same errors on random lines); shuf (the hop life's
# frames replayed time-shuffled, no task); buttons (the buttons' errors only: idea 159 as it
# is); both; local (each line's error reaches only cells within --reach cells of it); compete
# (hop with competitive learning). Rule: --g.hop_shift 5 --g.hop_fired 0; local reach 8.
#   scripts/predhop.sh smoke --quick                every arm and world, 1 seed, short lives
#   scripts/predhop.sh main                         seeds 5-12, 3 runs at a time
#   scripts/predhop.sh main --table-only            the tables from finished runs
#   scripts/predhop.sh tune --seeds 1,2 --arms born,pred,hop,coin --g.hop_shift 5
#   scripts/predhop.sh main --worlds chain,beat --arms born,hop --jobs 2 --reach 8
# Writes out/predhop/NAME/ (a log and csv per run), out/predhop/NAME.md and NAME.csv (every
# number, one per line). Finished runs are kept: running again only runs what is missing.
# Anything else on the line goes to every run (blobprobe flags, --g.name value; later wins).
# Runs read lean (--lean 1: hidden-cell and line readers only; the ridge fits are most of a
# run's time); --full adds the whole-sheet and hidden-plus-lines readers.
# One run by hand (chain, arm hop, seed 5):
#   ./build/blobprobe predict --task chain --ticks 20 --sensor-side 2 --motor-side 3
#     --motor-spread 64 --window 1000 --g.sensor_w 64 --g.motor_stride 1 --g.learn_mode 5
#     --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --predict 1,2,4,8
#     --predict-shift 2 --g.hop_on 1 --g.hop_shift 5 --g.hop_fired 0 --g.hop_teach 0
#     --predict-hop 1 --seed 5 --trials 3000 --life 20000 --future 1,8 --label hop
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobprobe >/dev/null
exec python3 scripts/predhop.py "$@"
