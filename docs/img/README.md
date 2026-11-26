# Pictures

One line per picture: what it shows, and the script line that remakes it. The plots' numbers
are in `scripts/figures_data.py`, each block with the doc table it was copied from. The
viewer pictures are single frames from blobview's capture mode (no window, software Vulkan),
cut to 256 colours.

- `viewer-random-input.png`: the sheet (4,096 neurons) at tick 3,000 with random bits on its sensor cells, one excitatory cell picked (its sources ringed, its numbers in the neuron panel), the raster, the lines over time, the sectors and the health panel. `scripts/captures.sh random-input`
- `viewer-delayed-echo.png`: delayed echo (answer 2 steps after the cue) under the one setting at step 5,000 of a life: the task's lines and the buttons' cells marked on the sheet, the task panel with the last steps and the score per window (the blob at 1.00, its learning-off twin near 0.5). `scripts/captures.sh delayed-echo`
- `viewer-xor-readout.png`: exclusive-or under the output rule alone at step 10,000 (score 0.84 and still rising), a cell of one button picked: the readout panel and that cell's evidence picture on the sheet (green for a source, red against). `scripts/captures.sh xor-readout`
- `viewer-probe-which.png`: the readability probe as the world: each tile tinted and numbered by how well "which cue" can be read from it, the cue tile ringed; also the sectors and the histograms. `scripts/captures.sh probe-which`
- `rule-ladder.png`: from backprop down to a rule in bits, in the model of the sheet's cells: one bar a rung for MNIST and the pair test, each rung's coin control, the fall at the counter step with a small chance and the recovery when every marked input counts. Main rungs only, 3 seeds a rung. `scripts/figures.py ladder`
- `headline.png`: what the real sheet's inside learns from a 60,000-digit life, MNIST and the pair test: competitive learning, the old one-hop rule, the closed loop inside the blob at both settings with its coin and shuffled-digit controls, and the backprop teacher's wiring written in, all against the born sheet. The closed loop is told the right answer; one family of data. `scripts/figures.py headline`
- `own-answer-life.png`: the blob's own answer's share right over a 60,000-digit MNIST life: the closed loop, the same output on a sheet that does not learn, and the old readout. `scripts/figures.py life`
- `hidden-cells.png`: what ten hidden cells of one sheet fire for, as born and after a closed-loop life: the average test digit weighted by the cell's spikes, less the plain average digit; dots are the cell's pixel inputs as born. It is what the cell fires for, not its wiring: the wiring after a life is not written out by any flag. `scripts/figures.py cells` (runs two MNIST lives of seed 5 first, under a minute; needs `scripts/build.sh` and `scripts/get_mnist.sh`)
- `task-map.png`: the quick map of 22 worlds at tabling: the late score of each world under four settings of the sheet and three reference engines, filled where the row is passed, rows passed at the bottom. `scripts/figures.py map`
