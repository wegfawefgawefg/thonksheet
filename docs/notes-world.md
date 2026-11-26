

## The blob's own output and the closed loop (idea 182)

`scripts/outputinside.sh ladder*|loop*|report|report2|reportcells|recheck|cap|toys|toys2|map|
speed|smoke` (arms in the script's header; `intope` is the closed loop, `intopeallw10` the
setting picked on validation). Genome: `out_on`, `out_top`, `out_bits`, `out_margin`,
`out_window`, `out_fine`, `out_hidden`, `out_pay` (`src/blob/learn_output.cpp`,
`output_state.hpp`); `hop_teach 6` (the output's two stepped lines are the cells' errors);
`readout_good 3|4` (the same rule on the spiking motor cells). `--motor-spread-even 1`
spreads a button's cells over the sheet (the pair test needs it). `blobmnist ceiling
--outside-form 4` with `--outside-read|act|bits|step-shift|vote|rule|zone-32|window|top` is
the ladder's outside output (`src/mnist/mnist_outside_cells.cpp`); `--life-teach 6`; the
line "own answer on the test digits, frozen". After an `out_on` life the toy probe's fresh
output is of the new kind unless `--after "out_on 0"`. On tasks with no verdict for "no
move" a tie goes to the lowest line.

