

## Cap probes and prediction errors on planes (ideas 180, 183)

`scripts/capdefault.sh chain|first|rate|map|inside|speed|alive|probecheck|table|smoke`;
`blobprobe capsplit` (`src/probe/cap_split.cpp`): a quick-map life with the threshold cap
set apart for motor, hidden and memory cells (`--cap-motor`, `--cap-hidden`, `--cap-memory`,
`--hold-motor 1`), counting what is pressed on each stage. An honest target of 64 is
`--g.sector_target_per_1024 64 --g.threshold_down_shift 8` (the per-cell walk has its own
target). `scripts/predevent.sh NAME --worlds --arms --seeds` (arms `ev<set>f<fine>` with
letters `o<M>`, `n`, `b` and endings `c`, `s`, `k`; header of the script), `scripts/predevent.sh
moved NAME` (MNIST digits moved a pixel a step, `src/mnist/mnist_moved.cpp`). Genome:
`hop_pred_fine`; `--predict-hop` is now a set of horizons; `--predict-outside M` is a
whole-number predictor outside the sheet with a margin (a measuring stick,
`src/run/predict_outside.cpp`). `hop_coin 3` on prediction lines means another step's errors.

