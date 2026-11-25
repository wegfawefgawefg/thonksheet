#!/usr/bin/env bash
# Idea 162, every line is a target (docs/results-2026-11-25-predict.md). Beside the world's
# buttons the sheet predicts every input line 1, 2, 4 and 8 steps ahead with predictor cells
# taught by the working output rule (blobrun --predict). The battery, on one sheet for every
# arm (scripts/predict.py has the flags):
#   task   blobrun on 11 worlds: off, pred, pred + error lines, pred + last press, last
#          press alone, competitive learning alone, pred + competitive learning. Gives the
#          prediction skill beside "same as now", each line's most common value and the
#          learning-off twin; the task score of every arm; ticks per second.
#   read   blobprobe predict on 8 worlds: after a life, freeze, and fit a fresh linear reader
#          to things not on the input lines at one instant (which way the paddle moved, time
#          since the cue, the next symbol, ...). Arms: born, task, pred, pred + error lines,
#          pred on time-shuffled frames (the control), compete, pred + compete,
#          pred + error lines + compete. Firing rates beside them.
#   old    predict-next with the old guess (blob.predicted, learn mode 0), for comparison.
#   scripts/predict.sh main                    everything, seeds 5-12, 3 runs at a time
#   scripts/predict.sh smoke --quick           1 seed, short lives: a smoke check
#   scripts/predict.sh main --only task --worlds catch,chain --seeds 1,2
#   scripts/predict.sh main --table-only       print the tables from what is there
#   scripts/predict.sh main --jobs 2 --steps 40000 --trials 3000
# Writes out/predict/NAME/ (one log per run), out/predict/NAME.md and NAME-*.csv. Finished
# runs are kept: running it again only runs what is missing. Anything else on the line goes
# to every run (blobrun flags, --g.name value).
# One life by hand (what the task part runs for catch, arm pred):
#   ./build/blobrun --task catch --ticks 20 --sensor-side 2 --motor-side 3 --motor-spread 64
#     --g.sensor_w 64 --g.motor_stride 1 --g.learn_mode 5 --g.readout_evidence 1
#     --g.readout_shift 4 --g.readout_balance 1 --predict 1,2,4,8 --predict-shift 2
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun --target blobprobe >/dev/null
exec python3 scripts/predict.py "$@"
