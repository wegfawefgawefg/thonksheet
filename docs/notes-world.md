# World notes

Handoff for `src/world/` (plug, tasks, stand-in) and `src/run/` (`blobrun`).

## Files

- `world/plug.hpp/.cpp`: lines to neurons, writing senses, counting motor spikes.
- `world/plug_print.cpp`: the layout as text (`blobrun --show-plug`).
- `world/task.hpp/.cpp`: `TaskKind`, the one `Task` struct, switch dispatch, window scores.
- `world/task_reflex.cpp`: echo, shuffled echo, delayed echo, go/no-go, beat.
- `world/task_sequence.cpp`, `world/task_control.cpp` (catch, cartpole).
- `world/task_feature.cpp`: order, both-not-either, gap, place (features nobody designed in).
- `world/task_script.hpp/.cpp`: tiers 9 and 10 (the rest of the map), every one a trial
  script; `task_relation.cpp`, `task_sheet_number.cpp`, `task_grammar.cpp`,
  `task_chance.cpp`, `task_memory.cpp`, `task_unsupervised.cpp` draw the trials.
  `world/plug_hidden.cpp`: fill-in's count of hidden cells' own predictions.
- `world/task_function.hpp/.cpp`: the function battery (points, number, two-numbers, past):
  function names, the `--fn` flags, drawing inputs, the shared trial; the class of every
  input in `task_function_points.cpp` (points, past) and `task_function_number.cpp`
  (number, two-numbers, the sheet they are drawn on).
- `world/task_bandit.cpp` (bandit, switching bandit, chain), `world/task_move.cpp` (track,
  forage), `world/task_classic.cpp` (mountain car, pendulum, with a 16.16 series sine):
  the more worlds of idea 63.
- `world/task_invariance.hpp/.cpp`, `world/task_invariance_draw.cpp`: the invariance
  battery (one task kind, `--task inv-<kind>`), its moving stretches, its pictures, tunes
  and 3D objects. `world/plug_map.cpp`: `--bitmap-map logpolar`.
- `world/task_calc.hpp/.cpp`, `world/calc_ops.hpp/.cpp`, `world/task_calc_flags.cpp`: the
  calculation battery (`--task calc --calc OP`), answers that are patterns of bits.
- `world/plug_pattern.cpp`: reading a pattern answer off pairs of motor lines, and the
  marks for its good or bad.
- `world/standin.hpp/.cpp`: a Blob shell with only the arrays the plug touches, and a fake
  tick (coin-flip motors, or the right motor group firing). Needs no core.
- `run/main.cpp`: the driver calls (birth, tick, reward, death; declared in `run/drive.hpp`)
  and `main`. `run/run_flags.cpp`: the command line; `run/run_flags_worlds.hpp` the
  more-worlds flags (a header, so the build lists that name `run_flags.cpp` stay as they are).
- `run/life.cpp`: one life as phases (task steps or unplugged ticks), never reset.
- `run/life_protocols.cpp`: plain life, retention, idle, capacity; the csv and the summary.
- `run/life_measures.cpp`: `--reach` and `--speed`.
- `run/life_end.cpp`: room and aliveness at the end of a life.
- `run/life_shown_damage.cpp`: the world's two hooks into every engine (`blob.silenced`,
  `blob.forced`) for `--damage` and `--shown`.
- `run/life_extra.cpp`: a task's extra scores per window (the calculation battery) into the
  summary and `<stem>-extra.csv`.
- `probe/rhythm_find.hpp`, `probe/carry_measure.hpp`: shared with `blobprobe`.
- `probe/reader.cpp`: `blobprobe reader`, level (a) of the function battery (a ridge reader
  of the frozen sheet and of the input lines; held-out values scored apart). Results so
  far: `docs/results-2026-11-25-function-battery.md` (unfinished; resume lines there).
- `probe/invariance.cpp`, `probe/invariance_show.cpp`, `probe/linear_split.cpp`:
  `blobprobe invariance`, level (a) of the battery, and `--show 1`, its pictures as text.

## The plug

Settings (`PlugSettings`, all flags on `blobrun`):

| setting | default | flag |
|---|---|---|
| sensor group side | 2 (a line drives 2x2 = 4 neurons) | `--sensor-side` |
| motor group side | 2 (a line is read from 4 neurons) | `--motor-side` |
| ticks per task step | 8 | `--ticks` |
| good/bad line on for | 2 ticks after a reward | `--reward-ticks` |
| rest after a rewarded step | 0 ticks, task lines off | `--rest-ticks` |
| tiles from cue lines to motor lines | 0 = packed (1 with the default genome) | `--distance` |

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
- `--distance N` widens the gap after packing: the regions face each other along one axis;
  the motor lines move away by whole tiles (up to half the shift, as far as the region
  allows), then the sensor lines, until the tile row of the cue cell nearest the motors and
  the tile row of the motor cell nearest the cues are N apart. Good and bad move with the
  cues. Whole tiles keep each group's place inside its tile. Default genome: the cues sit
  in tile row 3 and the buttons in row 4 (distance 1); 2 moves the buttons to row 5, 3 the
  cues to row 2, 5 gives rows 1 and 6, 7 is the most. Too far, or closer than packed, is
  refused with a message.

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
| order, gap 1 | A, B, ask; 2 buttons | right / trials | 0.44 | 1.00 |
| both-not-either | A, B, ask; 2 buttons | right / trials | 0.45 | 1.00 |
| gap 1 or 3 | cue, ask; 2 buttons | right / trials | 0.44 | 1.00 |
| place 4x4, 3 held out | ask, 4x4 sheet; 2 buttons | right / held-out trials | 0.44 | 1.00 |
| bandit k=2 (k=4) | one line always on; k buttons | presses on the best / steps | 0.47 (0.24) | 1.00 |
| switching bandit k=2 | as bandit | best / steps, second half of each block | 0.46 | 1.00 |
| chain, stage shown or not | 2 stage lines (or 1 always on); 2 buttons | 2 x chains / steps | 0.29 | 1.00 |
| track 5, shaped or arrive | 5x2 bitmap; left, right | steps ending nearer / steps | 0.46 | 1.00 |
| forage 5x5, warmer or not | 5x5 bitmap (side 2); 4 buttons | steps ending nearer / steps | 0.38 | 1.00 |
| mountain car | 2 numbers x 8 lines (side 2) | episodes at the goal / episodes | 0.00 | 1.00 |
| pendulum | 2 numbers x 8 lines (side 2) | steps upright / steps | 0.05 | 0.86 |
| inv-position | ask, 16x12 sheet at side 1; 2 buttons | right / held-out trials | 0.48 | 1.00 |
| inv-scale | | | 0.47 | 1.00 |
| inv-rotation | | | 0.47 | 1.00 |
| inv-bright | | | 0.48 | 1.00 |
| inv-clutter | | | 0.48 | 1.00 |
| inv-mixed | | | 0.47 | 1.00 |
| inv-view | | | 0.47 | 1.00 |
| inv-shift | A, B, C, ask at side 3; 2 buttons | | 0.48 | 1.00 |
| inv-tempo | | | 0.49 | 1.00 |
| inv-pitch | ask, 16x8 sheet at side 1; 2 buttons | | 0.50 | 1.00 |

The function battery (79 members, `scripts/functions.py --list`; 20,000 steps, window 200,
seed 1): perfect is 1.000 on every member; coin is 0.47-0.48 on two-button members and a
little under 1/answers on the rest (0.31 for 3, 0.23 for 4, 0.18 for 5, 0.11 for 8, 0.10
for 9). `--task points --fn parity --fn-lines 2` gives a csv byte-identical to
both-not-either, whose own 40,000-step csv did not change.

The battery rows: 20,000 steps, window 200, `--ticks 20`, mean over all windows; the
rewarded versions score the same (coin 0.45-0.48, perfect 1.00). Definitions:
`docs/curriculum.md`, "Invariance".

The four new rows were checked with the core linked (coin and perfect are the stand-in
either way), mean over all 100 windows. Definitions: `docs/curriculum.md`, "Features nobody
designed in".

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

The invariance battery (`scripts/invariance.sh` runs it at three levels):

```
nice -n 19 ./build/blobrun --task inv-scale --sensor-side 1 --ticks 20       picture kinds
nice -n 19 ./build/blobrun --task inv-rotation --sensor-side 1 --inv-motion 50    with motion
nice -n 19 ./build/blobrun --task inv-tempo --sensor-side 3 --ticks 20        time kinds
nice -n 19 ./build/blobrun --task inv-position --sensor-side 1 --inv-hold r6  6 places held
nice -n 19 ./build/blobrun --task inv-scale --sensor-side 1 --bitmap-map logpolar
nice -n 19 ./build/blobprobe invariance --task inv-view --sensor-side 1 --ticks 20   level (a)
scripts/invariance.sh show rotation                                  the pictures as text
```

New tasks and settings:

```
nice -n 19 ./build/blobrun --task order --order-gap 1 --steps 20000
nice -n 19 ./build/blobrun --task both-not-either --steps 20000
nice -n 19 ./build/blobrun --task gap --gap-short 1 --gap-long 3 --steps 20000
nice -n 19 ./build/blobrun --task place --place-size 4 --place-hold 3 --steps 20000
nice -n 19 ./build/blobrun --task echo --distance 3 --show-plug     any task, N tiles apart
nice -n 19 ./build/blobrun --task echo --cue-offset 2               cues and buttons 2 and 3
```

The function battery (definitions: `docs/curriculum.md`, "The function battery"):

```
nice -n 19 ./build/blobrun --task points --fn parity --fn-lines 3          and or parity majority
nice -n 19 ./build/blobrun --task points --fn exactly --fn-k 1 --fn-lines 4  exactly table
nice -n 19 ./build/blobrun --task points --fn table-nonlin --fn-lines 4    table-lin count first
nice -n 19 ./build/blobrun --task number --fn sine --fn-lines 16 --fn-bands 4 --sensor-side 2
nice -n 19 ./build/blobrun --task number --fn sine --fn-lines 16 --fn-bands 4 --fn-hold 3 \
    --sensor-side 2                                                      held-out x only scored
nice -n 19 ./build/blobrun --task two-numbers --fn product --fn-lines 8 --fn-bands 2 --sensor-side 2
nice -n 19 ./build/blobrun --task past --fn same --fn-gap 2                recall majority parity
nice -n 19 ./build/blobprobe reader --task points --fn parity --fn-lines 3 --ticks 20 \
    --sensor-side 3 --motor-side 3 --seed 5                              level (a): linear reader
scripts/functions.sh main                                               the whole battery
```

Number kinds lay x on the bitmap sheet, 8 lines a row in a snake, so they need 2x2 sensor
groups or smaller (`--sensor-side 2`); at 3x3 the plug refuses a sheet 8 wide.
More worlds (idea 63; definitions in `docs/curriculum.md`, "More worlds"). Easiest through
`scripts/task.sh <world>`, which picks 2x2 sensor groups and the place code where a world
needs them; `scripts/ladder.py` runs the whole set for every engine and rule:

```
scripts/task.sh bandit --k 4
scripts/task.sh switch-bandit --switch-every 1000
scripts/task.sh chain --chain-cue 0
scripts/task.sh track --track-reward arrive
scripts/task.sh forage --forage-warmer 0
scripts/task.sh mountain-car --classic-reward goal
scripts/task.sh pendulum
scripts/task.sh cartpole --cart-start wide | --cart-reward relative | --cart-integrator euler
scripts/task.sh cartpole --million                  1,000,000 steps in 100 windows
python3 scripts/ladder.py run --rules minimal [--long]; python3 scripts/ladder.py table
```

- Every life now prints episodes (and wins where a kind has them) per driver, for catch,
  cartpole and the new worlds: `blob 1162 falls`, `coin 6666 dots, 1633 caught`. The
  summary gets `episodes` and `wins` keys; the window csv is unchanged.
- `--windows N` makes the window steps / N, so a long life keeps a small csv.
- Cartpole's step was gym's Euler step: a push moves the pole only on the next step, so a
  step's reward does not judge its own press. `--cart-integrator semi` changes that, and
  since the critic merge (2026-11-25) it is the default; `--cart-integrator euler` keeps the
  old step for reproducing older cartpole numbers (curriculum, notes on cartpole).

Life protocols (one per call; without them a call is one task for the whole life, as
before, with byte-identical csvs). Definitions: `docs/curriculum.md`, stage 4. Easiest
through `scripts/task.sh`, which adds the standard plug:

```
scripts/task.sh echo --retention echo --window 200 --retest 10000   A, B, A again
scripts/task.sh echo --idle 400000 --window 200 --retest 10000      A, unplugged, A again
scripts/task.sh echo --capacity                                     k = 2, 4, 8, 16
scripts/task.sh echo --frozen-readout                               only into motors learns
nice -n 19 ./build/blobrun --reach                                   cue readable how far
nice -n 19 ./build/blobrun --speed                                   ticks/s, bytes/neuron
```

- `--frozen-readout` sets the genome's `learn_mode` to 1 by name (readout: only connections
  into motor neurons learn). It is on main since the rival learning modes landed. With `--table`
  or `--engine minimal` blobrun refuses it (they have no readout mode), and the suite then
  shows the frozen-sheet rows as n/a.
- `--reach` uses the same driver calls as a life, so it measures whatever engine blobrun
  drives; `--seed 1` prints exactly what `blobprobe carry --seed 1` does.
- `--speed` ticks one blob per size (8x8, 16x16, 32x32 tiles) with random sensor input and
  learning on for `--speed-ms` each; bytes per neuron is the heap the birth took (glibc
  `mallinfo2`), so it counts any engine's arrays.

End of every real-blob life, printed and in the summary:

- room: connections per neuron; share of connections at strength 1, 4, 5; share of
  neurons at the connection cap; share with the threshold at its cap.
- alive: spikes per 1024 per tick against the genome target, share of silent neurons (no
  spike) and stuck ones (spikes on over a quarter of the ticks), over the last
  `--alive-ticks` ticks (16,384) with the task on, sensor cells left out; then everything
  unplugged for `--rhythm-ticks` ticks (16,384) and the pulse period and strength
  (`blobprobe rhythm`'s finder; under 0.05 counts as none, period 0).
- An array an engine does not have gives -1 ("not known"), never a guess.

Every call also writes `<csv stem>-sum.csv`, lines of `driver,key,value` for every number
it printed. That file is what the suite reads. Retention and idle csvs have a `phase`
column after `end_step`; capacity writes `<stem>-k2.csv` to `-k16.csv`; reach writes
`distance,blob,learn_off`.

## Answers that are patterns: the calculation battery (2026-11-25)

Definitions: `docs/curriculum.md`, "Calculation". Results:
`docs/results-2026-11-25-calculation.md`.

- **Reading.** A task sets `TaskShape.pattern_bits = B` and gets 2B motor lines: line 2b is
  bit b "off", 2b + 1 "on". `ticker_after_tick` calls `plug_pattern` in place of
  `plug_choice`: each pair votes like two buttons (more spikes over the step wins, a tie is
  no answer) into `TaskStepIn.pattern` and `pattern_tied`; `move` is -1.
- **Marks.** Before a good or bad, `life.cpp` calls `plug_mark_pattern` in place of
  `plug_mark_acted`: only the cells of the output bits in `Task.pattern_graded` are marked
  as buttons' cells (`motor_attached`), and of each the group that won as pressed
  (`motor_acted`; neither on a tie). The working readout rule moves nothing on good and,
  on bad, moves only marked cells, so this is all a per-bit verdict needs: no change in
  `src/blob/`. Whole-answer grading marks every output; per-bit grading marks only the
  wrong bits (and all bits on a good, where nothing moves). The minimal engine and table
  neurons read no marks: for them both gradings are one good or bad for the whole answer.
- **Scores.** The window score is bits right on seen cases. A task may fill `Task.extras`
  scores per window; `life.cpp` keeps them per driver and `life_extra.cpp` writes their
  early and late to the summary (`whole_seen_late`, `bits_held_late`, `whole_held_late`,
  `bit<k>_seen_late`, `bit<k>_held_late`) and every window to `<stem>-extra.csv`. Tasks
  without extras are byte-identical.
- **Perfect player.** For a pattern task `task_perfect_move` returns the answer's bits and
  the stand-in fires the right group of every pair.
- **Plug limits.** With `--motor-spread` the region's 256 cells go to 2B lines (B = 9 gives
  14 cells a line). The checkerboard deals cell (x, y) to line (x + y) mod lines, which has
  only 31 diagonals, so at most 15 output bits; `turn3` runs at size 2 (12 bits) for that
  reason. Past 25 lines the packed layout no longer fits; with spread on, the extra lines
  share the first packed place (it only sets how near a motor cell may sit). At sensor side
  3 there are 23 task lines; every member of the battery fits.
- **Flags** live in `world/task_calc_flags.cpp` (one hook in `run_flags.cpp`), so every
  program that parses blobrun's flags (probes included) has them.
- **A button per answer** (`--calc-form buttons`): `pattern_bits` 0 and one motor line per
  possible answer (every pattern of up to 4 bits, or the distinct answers of a small case
  set, at most 16), graded whole. For the engines that press one button (memory, mlp,
  gru). A held-out case whose answer no seen case gives is not scored (nan in the
  summary; nan windows are left out of means). A member with too many answers asks the
  plug for 1,000 motor lines, so the plug refuses it with a message.
- **Held-out balls** (`--calc-ball R`, exact case sets only): held cases are taken in hash
  order and each brings every valid case within R line flips. R = 0 is the old draw.
- **Layout** (`--calc-lay 1`): two operands bit by bit side by side (a0 b0 a1 b1 ...).
- **`blobprobe calc`** reports five readers: the best constant answer, the nearest stored
  seen trial (Hamming distance over every step's lines; ties vote per bit), a ridge reader
  of the input lines, one of the sheet, and for programs the sheet asked for the
  instruction one step back. Each: bits and whole answers, seen (later trials) and held
  out, and every bit, to `--out` as `reader,key,value`.
- `scripts/calc.py`: `CALC_BUILD=dir` runs another build's binaries (used while a run of
  the default build was going); a run cut off leaves a summary without its scores and is
  run again.

```
scripts/calc.sh main                       the battery, seeds 5-12, 2 runs at a time
scripts/calc.sh main --quick               2 seeds, short lives
scripts/calc.sh main --only arith --configs probe,bit
scripts/calc.sh main --table-only          the table from what has run
nice -n 19 ./build/blobrun --task calc --calc add --calc-bits 4 --calc-grade bit \
    --ticks 20 --sensor-side 3 --motor-side 3 --motor-spread 128 --steps 40000 \
    --window 1000 --g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 \
    --g.readout_balance 1
nice -n 19 ./build/blobprobe calc --task calc --calc add --calc-bits 4 --ticks 20 \
    --sensor-side 3 --motor-side 3 --motor-spread 128 --seed 5
```

## The suite

```
scripts/suite.sh blob-main                       every bench row, seeds 5 to 12
scripts/suite.sh blob-main --quick               seeds 5 and 6, 4,000-step lives
scripts/suite.sh no-pulse --g.sector_smooth_shift 6
scripts/suite.sh table --table                   another engine through a blobrun flag
scripts/suite.sh blob-main --resume              keep finished runs, do the rest
scripts/scoreboard.sh                            all scorecards into docs/scoreboard.md
```

Rows and their blobrun lines are in `scripts/suite.py` (`jobs_for`, `rows_for`). The
standard life is `scripts/task.sh`'s: 20 ticks a step, 3x3 groups, 40,000 steps, windows of
1,000. Exceptions: catch is 4x4 (6x6 does not fit at 3x3 groups), cartpole uses 2x2 sensor
groups (24 number lines do not fit at 3x3) and the place number code, run twice: with
the lean reward (the main row) and the sparse one (slow credit), retention and idle use windows of 200 so the
first retest window is short. Everything after the name is appended to every call, so it
overrides the row's own flags. For `--table` the suite reads the `table` and `table_off`
drivers in place of `blob` and `learn_off`. Engines without weight planes (table, minimal)
show the room rows as n/a. Runs and logs go to `out/suite/<name>/`; the scorecard to
`out/scorecards/<name>.csv` (committed; the rest of `out/` is not). Speed is measured last,
alone among the suite's runs, but other agents may be running: the scorecard says so.

More-worlds rows (idea 63) come after every older row, under "more worlds (idea 63)", so
old scorecards still line up; `--only JOB,...` runs only some jobs (`--only worlds`: the
new rows) and writes a scorecard with only those rows.

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

- Retention's B is an echo on two new cue lines and buttons, so during B there are four
  buttons and coin scores about 0.2 on it, not 0.45. B's lines sit further along the shelf
  (at 3x3 groups the fourth cue is on the shelf above), so B is also a harder echo than A.
- The place task's training places are rewarded but not scored; their right/tries is kept
  in tally[2]/[3] and not printed yet.
- End-of-life aliveness is over the last 16,384 ticks of the life with its task still on,
  so the rate includes the cue-driven firing; the rhythm is taken unplugged afterwards so
  the task's step does not set the beat.
- Bytes per neuron counts heap only; a static or mmap'd engine would read low.

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

## The rest of the map (2026-11-25)

Tiers 9 and 10, being shown the answer and damage: 21 worlds with an easy and a hard
setting each, and two life protocols. Definitions: `docs/curriculum.md`, "The rest of the
map". The list of worlds with their flags is `scripts/map_worlds.py`, shared by
`scripts/map.py` and `scripts/suite.py`. Results: `docs/results-2026-11-25-rest-of-the-map.md`.

How the new tasks are built:

- Every one is a trial script (`Task::Script` in `task.hpp`): per step the single lines on,
  the sheet bits on, the button wanted (-1 none) and flags: `SCRIPT_SCORED` (counts toward
  the score), `SCRIPT_REWARDED` (good or bad follow), `SCRIPT_PROBE` (the offline reader
  keeps this step, label = the wanted button). `task_script.cpp` runs it; one file per family
  draws trials. New kinds sit at the end of `TaskKind`; `task.cpp` hands them all to the
  script runner before its old switches.
- Held-out trials are scored and not rewarded; taught trials of a held-out world are
  rewarded and kept in `tally[2..3]`, not printed.
- No-teacher worlds attach no good or bad. Their buttons exist only to give the reader its
  classes; presses do nothing. Their window score comes from `task.extra`: the share of sensor
  cells predicted right (predict-next), hit rate less false alarms of the hidden cells' own
  predictions (fill-in, counted every tick by `plug_count_hidden` from `TaskSense.hidden`),
  wrong predictions on breaks less on regular steps (odd-event). The minimal engine fills no
  `predicted` and no `surprise`: its prediction share reads 1.00 and the others 0, which
  mean nothing; `map.py` shows them as "-". (The suite's old sequence row has the same
  problem for the minimal engine.)
- `--shown N` and `--damage P` / `--damage-tile 1` are life protocols (`life_protocols.cpp`)
  with two phases. Phase settings in `LifePhase`: `shown_ticks`, `withhold` (the ticker scores
  the answer but passes no valence and raises no good/bad line), `damage_pct`, `damage_tile`.
  The hooks are two arrays on the Blob shell, `silenced` and `forced`, tile_count + 1 words
  each, null unless a protocol makes them (`life_shown_damage.cpp`); every engine's fire
  step ANDs out `silenced` and ORs in `forced` (sensor cells untouched). With both null every
  engine's csv is byte-identical to before.
- `blobprobe linear` keeps a step of a new world when `task.probe_marks` moves (the old
  tasks still use `tally[1]`). It also prints the same reader fitted to the task's own input
  lines over the last `--history` steps (16) and to their brightness, one feature per count:
  `--lines-from N --lines-to N` keep only some lines, to ask what one item alone tells.

```
scripts/map.sh map                          every world, three levels, seeds 5-12
scripts/map.sh map --quick                  seeds 5 and 6, 4,000-step lives (a few minutes)
scripts/map.sh map --only copy-easy,copy-hard --levels b
scripts/map.sh map --print                  the table from runs already in out/map/map/
scripts/task.sh same --item-lines 6 --item-on 3 --hold 6         one life, one world
scripts/task.sh echo --damage 20 --window 200 --retest 10000      damage protocol
scripts/task.sh echo --shown 6 --window 200 --retest 10000        being shown the answer
scripts/readout.sh reference brackets --length 8 --seed 5         the reader on one world
```

`map.sh` levels: (a) `blobprobe linear`, full engine as born, `--motor-spread 128`, 3,000
probe steps after 30,000 settling ticks; no-teacher worlds also (a') after a whole
40,000-step life of the design rule with no reward (`--learning 1 --settle 800000`); (b)
`blobrun` with the readout rule (`--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4
--g.readout_balance 1 --motor-spread 128`, the same for every world), no-teacher worlds with
the plain full engine instead; (c) `--engine minimal` and `--table`, no extra flags. Every
life is the standard one (20 ticks a step, 3x3 groups, 40,000 steps, windows of 1,000;
protocols windows of 200 and a 10,000-step retest); the sheet worlds use 2x2 sensor groups.
Runs go to `out/map/<name>/`; split across machines with `--levels`, copy the runs into one
folder and `--print`.

## Long memory worlds and `--long` (2026-11-25)

`world/task_long.cpp` (hold, pairs, once, rare, drift; settings in `task_long.hpp`, flags in
`task_long_flags.cpp`) and `run/life_long.cpp` (the protocol). Definitions:
`docs/curriculum.md`, "Long memory". Shared edits were kept to the enum, the name table, one
case group per switch in `task.cpp`, one `LongSettings` in `TaskSettings`, one `LongState`
in `Task`, one flag hook in `run_flags.cpp` and one protocol entry.

- `blobrun --long --task pairs ...` runs the engine, coin flips and the perfect player (the
  learning-off twin only without `--no-baselines`), one life each, and writes per window
  per driver a row of each bucket's right answers over asks (`score` is the headline:
  probes for pairs and once, the rare patterns for rare, all pairs for drift, asks for
  hold). The summary has `<bucket>_all`, `_before`, `_after` (split at `drift_at` for
  drift, half the life otherwise), `_asked`, and `held` for pairs and once.
- `held` is the forgetting curve above chance rescaled to 0..1, integrated over age (flat
  before age 100, straight lines between ages), over steps per pair; an age within two
  standard errors of chance counts as 0.
- For the full engine the end of a life also prints button cells with no connection and the
  share of evidence counters at an end of their range or not zero.
- Speed: a step is 20 ticks, so 10M steps is 200M ticks. On this PC under load the full
  engine ran 20k to 38k ticks/s, table neurons about 14k; on stevejobs under load table
  neurons about 19k, the minimal engine about 58k (a 10M-step life about 1 hour).
- Ticks per step stay 20 as everywhere; a hold gap is given in steps (`--hold-steps`).

## The invariance battery (2026-11-25)

- One `TaskKind::invariance`; which change it is lives in `TaskSettings::inv` (so the
  other agents adding task kinds touch the enum once). `--task inv-<kind>` sets both.
  Its flags are parsed in `invariance_flag` (task_invariance.cpp), one hook in
  run_flags.cpp. `task_name` says "invariance", so the default csv is
  `out/invariance-s<seed>.csv` for every kind; give `--out`.
- Held-out trials are tally 0/1 (the window score), rewarded ones 2/3.
  `task_taught_score` reads 2/3 for place and the battery; `life.cpp` keeps it per window
  and the summary gets `taught_early` and `taught_late` (place's were never printed).
- The time kinds change their lines inside a step. `ticker_before_tick` now senses on
  every tick when `inv_by_tick` says so, after telling the task the tick
  (`inv_set_tick`); every other task still senses once a step (csvs byte-identical).
- `--bitmap-map logpolar` (`plug_map.cpp`) is the only plug change: at attach the task's
  bitmap size is swapped for 16 x 8 and a table says which task pixels each mapped line
  ORs; `plug_set_lines` fills the mapped lines from it. Off by default; any task with a
  bitmap can use it (catch too, though there it means little).
- `blobprobe invariance` drives any engine (`--engine minimal`, `--table`); blobprobe now
  links the minimal and table libraries (CMakeLists, two blocks).
- Moving stretches count as steps of the life: with `--inv-motion 50` a 40,000-step life
  has half as many trials.

## Reading a scorecard run with a readout rule in its flags (2026-11-25)

The suite's "frozen sheet, readout learns" rows force `learn_mode 1`, the old readout, so in
a scorecard such as `blob-readout` (run with `--g.learn_mode 5 ...`) those rows still show
the old readout at chance. The rows labelled "whole blob learns" are the ones that ran with
the flags given, which for mode 5 means a frozen sheet with the new readout.


## Every line is a target: predictor cells and extra lines (idea 162, 2026-11-25)

Results: `docs/results-2026-11-25-predict.md`. Off by default; with no `--predict*` flag a
life is byte-identical (the four reference lives were checked).

- **Flags** (`world/predict_flags.cpp`, one hook in `run_flags.cpp`; `blobrun --help` lists
  them): `--predict 1,2,4,8` horizons in steps (1..15, at most 4); `--predict-press 1` the
  last press as lines; `--predict-errors 1` the shortest horizon's errors as lines;
  `--predict-cells N` most cells per answer group (4); `--predict-shift S` the verdict's
  chance (default the readout's own `readout_shift`); `--predict-score-from N` the first
  step tallied (default: the last quarter of the life).
- **Targets.** Good and bad when the task attaches them, then every line of the task's own
  (singles, bitmap, number lines). Not the extra lines.
- **Cells** (`run/predict.cpp`). Each (target, horizon) is a pattern bit: an "off" and an
  "on" group of motor cells, voted over the step. A tie answers off. The cells are the motor
  lattice cells no button uses, nearest the top of the sheet first, dealt round the groups.
  With the default regions the buttons take every motor cell, so the battery widens the
  regions: `--g.sensor_w 64 --g.motor_stride 1 --motor-spread 64` (sensor lattice across the
  top half; every cell of the bottom-left quadrant a motor cell, 1,024 of them; 64 cells a
  button). Catch then gets 2 cells a group, cartpole 4.
- **Verdict.** When step t + h is sensed, horizon h's answers from step t are graded against
  its lines (good and bad: the plug's timers, i.e. the reward of step t + h - 1). The wrong
  bits' cells are marked (both groups attached, the answering group acted; the world's own
  marks saved and put back) and `blob_readout_verdict` runs the readout's error step at once
  on step t's spike maps, kept in a ring of 16 steps x 16 maps (the core's one new function,
  `learn_readout.cpp`). With `readout_good` the right bits get a good the same way. With
  `readout_center` the verdict would overwrite the centre reference; not used.
- **Extra lines** (`PlugSettings.extra_lines`, `Plug.first_extra`): laid after the numbers,
  in columns as tall as the shelf they land in, attached like any line; the buttons' spread
  and `--distance` ignore them, so the buttons keep their cells. `predict_attach` lays the
  plug a second time with them. Set at a step's first tick, before the lines are written:
  press lines one per button (the last step's press), error lines two per target ("said on,
  was off", "said off, was on").
- **The loop** (`life.cpp`, `blobprobe predict`): `predict_before_tick` (senses the step's
  lines a tick early to grade and set the extra lines; the ticker then senses the same),
  `predict_after_tick` (counts), `predict_step_end` after `ticker_after_tick` (reads the
  answers, keeps the maps). Only the full engine and its twin get it; `--rest` is refused.
- **The hook for idea 159**: `Predictor::error` (`predict_errors`), per target, the shortest
  horizon's signed error at this step's start (+1 came on unpredicted, -1 predicted and did
  not come). This is where the errors would go one hop to hidden cells.
- **Summary keys** per driver: `pred_lines`, `pred_cells`, `pred_verdicts`, and per horizon
  `pred_h<h>_right|same|common|common_balanced|same_balanced|balanced|tied|changed|
  changed_right|on|on_right|off_right`.
- **`blobprobe predict`** (`probe/predict_read.cpp`, `probe/predict_labels.cpp`): live
  `--life N` steps (rewards as blobrun gives them), or `--shuffle 1` (the same life's frames
  replayed to a fresh sheet of the same seed in a shuffled order, no task, no reward), freeze,
  then read the world afresh (task seed + 7919, moves from a fixed coin) with a ridge reader
  of the whole sheet (`linear_whole`, new in `linear_fit.cpp`) and of the hidden cells, beside
  readers of the lines. `--trials`, `--settle`, `--label`, `--out`.

```
scripts/predict.sh main                    the battery, seeds 5-12
scripts/predict.sh smoke --quick           1 seed, short lives
nice -n 19 ./build/blobrun --task chain --ticks 20 --sensor-side 2 --motor-side 3 \
    --motor-spread 64 --window 1000 --steps 20000 --g.sensor_w 64 --g.motor_stride 1 \
    --g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 \
    --predict 1,2,4,8 --predict-shift 2
```
