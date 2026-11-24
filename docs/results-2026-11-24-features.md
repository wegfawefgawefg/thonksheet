# Are features nobody designed in already there? (2026-11-24)

Idea 49. The owner's thesis: a sheet rich enough in features turns learning into
selection. The suite's version of the test (freeze the sheet, let the readout learn) came
back at chance on all four feature tasks. That cannot tell "the feature is not there" from
"the readout cannot find it". This probe can.

## Method

`scripts/feature.sh` (the probe is `src/probe/feature.cpp`). Full engine, 4,096 neurons,
seed 1, no reward anywhere. One tile's sensor cells are cue A and cue B. 4,000 tries after
30,000 ticks of the same input. For every neuron: the gap between its mean spike count in
a reading window under the two classes, in units of its try-to-try spread; a tile's number
combines its 64 neurons less what chance gives. It is what an ideal reader of that tile
could get: 1.0 is easy, under 0.3 is lost.

- **which**: A or B on for the whole try.
- **order**: A then B, or B then A; read only after both are off.
- **xor**: A and B each on or off; exactly one, against both or neither. A neuron that just
  adds its inputs scores zero here.
- **gap**: two blinks with a gap of 3 or 9 ticks; read the same number of ticks after the
  second blink either way.

## Result

Readability at 0, 1, 2, 3, 4 tiles from the cue.

| feature | variant | 0 | 1 | 2 | 3 | 4 |
| --- | --- | --- | --- | --- | --- | --- |
| which | as born | 9.6 | 9.6 | 2.3 | 0.76 | 0.48 |
| which | learning on, no reward | 13.1 | 15.3 | 3.6 | 1.55 | 1.33 |
| order | as born | 3.1 | 3.4 | 1.05 | 0.33 | 0.20 |
| order | all delays 1 | 1.5 | 2.2 | 0.78 | 0.45 | 0.21 |
| order | delays 4 to 8 | 5.4 | 5.4 | 0.99 | 0.44 | 0.23 |
| xor | as born | 2.0 | 1.7 | 0.88 | 0.30 | 0.23 |
| xor | after 600k ticks | 2.4 | 2.1 | 1.37 | 0.42 | 0.18 |
| xor | delays 4 to 8 | 1.5 | 1.3 | 0.60 | 0.22 | 0.13 |
| gap | as born | 1.3 | 1.5 | 1.66 | 0.59 | 0.26 |
| gap | all delays 1 | 0.43 | 0.69 | 1.30 | 0.38 | 0.26 |
| gap | delays 4 to 8 | 4.0 | 4.4 | 3.21 | 1.82 | 0.97 |

Growth and pruning off gives the "as born" numbers again in every case.

## Reading

- **The features are there, at birth, with nothing learned.** Order, exclusive-or and gap
  length are all readable well above 1.0 within a tile or two of the input. Nobody put
  them there. The sheet's own mixing of signals through thresholds, inhibition, loops and
  delays makes them.
- **So the feature tasks fail at the reading, not at the sheet.** The frozen-readout runs
  score 0.44 to 0.49 on tasks whose answer an ideal reader can get from the tile next
  door. The wall is a rule that can pick out information spread thinly across many
  neurons.
- **Delays buy time.** With every delay at 1, gap length is nearly unreadable near the
  input (0.43) and order is halved. With delays of 4 to 8 ticks, gap goes to 4.0 and stays
  readable four tiles out, and order nearly doubles. On Echo, delays measured as doing
  nothing. They are what gives the sheet a sense of time.
- **Delays cost a little elsewhere.** Exclusive-or reads slightly worse with long delays.
- **Wiring by co-activity helps "which" most** (9.6 to 15.3) and the others a little.

One seed and an ideal reader. The effects are large; they want more seeds, and the same
probe on the minimal and table engines.
