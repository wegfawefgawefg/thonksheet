# The minimal engine (2026-11-24)

The full engine (`src/blob/`) has about 21 mechanisms. The owner agreed the elegant core is
probably three things and asked for a simpler design to be tried. This is it:
`src/minimal/`, run with `blobrun --engine minimal` or `scripts/minimal.sh`.

## Predictions

Written after the engine compiled and before any task was run on it.

1. **Echo k=2 at one tile apart: the minimal engine scores within 0.05 of the full engine**
   (mean late score over seeds 5 to 12). Confidence 50%.
2. **The noise-graded rule carries the result.** Replacing it with "grade everything
   recently active" costs more than 0.10 on Echo k=2; turning off the budget or wiring by
   co-activity each costs less than 0.05. Confidence 45%.
3. **It is at least 4x faster per tick than the full engine at 65,536 neurons on one core,
   with under half the bytes per neuron.** Confidence 75%.
