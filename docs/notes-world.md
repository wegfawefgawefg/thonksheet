# World notes

Handoff for `src/world/` (plug, tasks, stand-in) and `src/run/` (`blobrun`).

## Files

- `world/plug.hpp/.cpp`: lines to neurons, writing senses, counting motor spikes.
- `world/plug_print.cpp`: the layout as text (`blobrun --show-plug`).
- `world/task.hpp/.cpp`: `TaskKind`, the one `Task` struct, switch dispatch, window scores.
- `world/task_reflex.cpp`: echo, shuffled echo, delayed echo, go/no-go, beat.
- `world/task_sequence.cpp`, `world/task_control.cpp` (catch, cartpole).
- `world/standin.hpp/.cpp`: a Blob shell with only the arrays the plug touches, and a fake
  tick (coin-flip motors, or the right motor group firing). Needs no core.
- `run/main.cpp`: the life loop. `run/run_flags.cpp`: the command line.

## The plug

Settings (`PlugSettings`, all flags on `blobrun`):

| setting | default | flag |
|---|---|---|
| sensor group side | 2 (a line drives 2x2 = 4 neurons) | `--sensor-side` |
| motor group side | 2 (a line is read from 4 neurons) | `--motor-side` |
| ticks per task step | 8 | `--ticks` |
| good/bad line on for | 2 ticks after a reward | `--reward-ticks` |
| rest after a rewarded step | 0 ticks, task lines off | `--rest-ticks` |

How lines land on cells:

- The sensor region (genome `sensor_x/y/w/h`, 16x16 at (0,0) by default) is cut into a grid of
  side x side groups. Items are shelf-packed left to right, a new shelf below when a row is
  full. Order: good, bad, the task's on/off lines, its bitmap as one block (pixel (px, py) is
  group (px, py) from the block's corner, so neighbours stay neighbours), then each number as
  a row of threshold lines.
- Good and bad are always the two top-left groups, so they sit on the same neurons in every
  task. A task with no reward (sequence) leaves them unattached.
- A number becomes `n` lines with thresholds evenly spaced over the task's [lo, hi]:
  line k is on when `value >= lo + (hi - lo)(2k + 1) / 2n` (a thermometer).
- Motor lines are packed the same way in the motor region (16x16 at (48,48)).
- Only the lines a task uses are attached (`blob_sense` marks them). The rest of the sensor
  region is never written. `plug_detach` clears a task's lines for task switching.
- Every tick, before `blob_tick`, `plug_write` writes every attached line. After the tick
  `plug_count` adds motor spikes per group. At the end of the step `plug_choice` returns the
  group with the most spikes; a tie for the top or no spikes at all is "no move" (-1).
- The plug checks every cell is in the blob's `sensor` / `motor` mask and refuses to attach
  if not, or if the lines do not fit.

`blobrun --task catch --show-plug --steps 200 --no-baselines` prints:

```
  GGBB############     G good, B bad, # the 6x6 bitmap at 2x2 cells per pixel
  GGBB############
  ....############     . sensor cells left unattached
  ...
motor region at (48,48)
  0011............     motor line 0 (left), 1 (right)
```

## One task step

```
task_sense -> plug_set_lines
repeat ticks_per_step: plug_write, tick, plug_count   (surprise read after the first tick)
move = plug_choice
valence = task_step(move)
if valence: blob_reward(valence), good/bad line on for the next reward_line_ticks ticks,
            then rest_ticks quiet ticks
every window steps: score = task_window_score
```

The reward lands after the step's last tick, so the "fired recently" bits still cover the
ticks that chose the move. The blob is born once per life and never reset; a fall in
cartpole or a missed dot restarts the game only.

## Tasks and their check scores

Stand-in, no core. 20,000 steps, window 200 (cartpole also 200,000 steps, window 1,000, five
seeds). Coin: each motor neuron fires with chance 1/4 per tick. Perfect: a scripted player
that fires the right group every tick and predicts its input exactly.

| task | lines | score | coin | perfect |
|---|---|---|---|---|
| echo k=2 | k cues, k buttons | right / tries | 0.43 | 1.00 |
| echo k=8 | | | 0.09 | 1.00 |
| shuffled-echo k=2 | button for cue drawn per life | right / tries | 0.44 | 1.00 |
| delayed-echo k=2 d=2 | k cues + ask, k buttons | right / asks | 0.44 | 1.00 |
| delayed-echo k=8 | | | 0.13 | 1.00 |
| go-no-go | go, no-go; press, rest | right / tries | 0.49 | 1.00 |
| beat period 4 | none; press, rest | balanced accuracy | 0.49 | 1.00 |
| sequence 8 lines, length 4 | 8 lines, no reward | right predictions / sensed, first tick | 0.50 | 1.00 |
| catch 6x6 | 6x6 bitmap; left, right | caught / dots | 0.16 | 1.00 |
| cartpole | 4 numbers x 6 lines; left, right | steps per fall | 23 | no fall in 200,000 steps |

Coin is below 1/k on the echo family because ties for the top count are "no move". The
perfect cartpole player is a linear push rule on the true state (angle, 0.5 spin, 0.05
position, 0.15 speed), bang-bang.

Same flags give a byte-identical csv: the task, the stand-in and the cartpole physics are
all integer.

## blobrun

```
cmake -S . -B build && nice -n 19 cmake --build build -j4
nice -n 19 ./build/blobrun --task echo --steps 20000
nice -n 19 ./build/blobrun --task echo --k 4 --steps 40000
nice -n 19 ./build/blobrun --task shuffled-echo --steps 20000
nice -n 19 ./build/blobrun --task delayed-echo --delay 2 --steps 40000
nice -n 19 ./build/blobrun --task go-no-go --steps 20000
nice -n 19 ./build/blobrun --task beat --period 4 --steps 40000
nice -n 19 ./build/blobrun --task sequence --seq-lines 8 --seq-length 4 --steps 20000
nice -n 19 ./build/blobrun --task catch --steps 40000
nice -n 19 ./build/blobrun --task cartpole --steps 100000 --window 1000
```

`blobrun --help` lists every flag. Output: `out/<task>-s<seed>.csv` with columns
`window,end_step` then one per life run (`blob`, `learn_off`, `coin`, `perfect`), and a table:

```
task echo  seed 1  task seed 1  steps 20000  ticks/step 8  window 200
blob      life took ... s, ... ticks/s
100 windows of 200 steps, early/late = 10 windows each
                early       late   late-early
blob           ...
learn_off      ...
coin           0.4235     0.4365      +0.0130
perfect        1.0000     1.0000      +0.0000
```

Early and late are the means of the first and last `--edge` windows (a tenth by default).
Without the core only `coin` and `perfect` run. With the core (`BLOB_HAS_CORE`, set by CMake
once `src/blob/*.cpp` exists) `blob` and `learn_off` run the real blob; coin always uses the
stand-in, since only the motor choices matter for it. The core path was checked once by
linking against a throwaway stub core; it builds and runs.

## Waiting on the core

- **A learning switch in the genome.** `learn_off` currently sets the predict, reward, grow,
  prune and far-redraw shifts to 30 and `surprise_boost` to 0. That is nearly off, and only
  sane if the core draws a 2^-30 chance cheaply. A plain `learning = 0/1` field would be
  exact.
- **What `stats.surprise` counts.** The sequence score assumes it counts only attached
  sensor neurons, on the latest tick. If it counts unattached ones too, the score is wrong.
- **Detaching.** `plug_detach` clears `sense_attached` and `sense_in` bits directly. A
  header function for it would keep the world out of the arrays.

## Rough

- No-move on echo is neutral. If a quiet blob sits still forever it gets no signal; making
  no-move bad is a one-line change in `judge_answer`.
- Delayed echo's delay is in steps, not ticks. Beat's period is in steps, and rest ticks
  stretch the gap in ticks, so leave `--rest-ticks` at 0 for beat.
- Cartpole's good every 25 steps is a guess. The four numbers each take a whole shelf row
  (6 of 8 groups), wasting a little of the region.
- Catch must have width <= height to be always catchable. At side 2 the sensor grid is 8x8
  groups, so the bitmap fits up to 6 wide next to good/bad, or 8 wide x 7 tall on a shelf of
  its own; 8x8 needs `--sensor-side 1`. The plug refuses cleanly when lines do not fit.
- Score per window is 0 when a window holds no try (a window shorter than one delayed-echo
  trial). Keep windows well above the trial length.
