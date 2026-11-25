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
- `world/standin.hpp/.cpp`: a Blob shell with only the arrays the plug touches, and a fake
  tick (coin-flip motors, or the right motor group firing). Needs no core.
- `run/main.cpp`: the driver calls (birth, tick, reward, death; declared in `run/drive.hpp`)
  and `main`. `run/run_flags.cpp`: the command line.
- `run/life.cpp`: one life as phases (task steps or unplugged ticks), never reset.
- `run/life_protocols.cpp`: plain life, retention, idle, capacity; the csv and the summary.
- `run/life_measures.cpp`: `--reach` and `--speed`.
- `run/life_end.cpp`: room and aliveness at the end of a life.
- `run/life_shown_damage.cpp`: the world's two hooks into every engine (`blob.silenced`,
  `blob.forced`) for `--damage` and `--shown`.
- `probe/rhythm_find.hpp`, `probe/carry_measure.hpp`: shared with `blobprobe`.
- `probe/reader.cpp`: `blobprobe reader`, level (a) of the function battery (a ridge reader
  of the frozen sheet and of the input lines; held-out values scored apart). Results so
  far: `docs/results-2026-11-25-function-battery.md` (unfinished; resume lines there).

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

The function battery (79 members, `scripts/functions.py --list`; 20,000 steps, window 200,
seed 1): perfect is 1.000 on every member; coin is 0.47-0.48 on two-button members and a
little under 1/answers on the rest (0.31 for 3, 0.23 for 4, 0.18 for 5, 0.11 for 8, 0.10
for 9). `--task points --fn parity --fn-lines 2` gives a csv byte-identical to
both-not-either, whose own 40,000-step csv did not change.

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

## Reading a scorecard run with a readout rule in its flags (2026-11-25)

The suite's "frozen sheet, readout learns" rows force `learn_mode 1`, the old readout, so in
a scorecard such as `blob-readout` (run with `--g.learn_mode 5 ...`) those rows still show
the old readout at chance. The rows labelled "whole blob learns" are the ones that ran with
the flags given, which for mode 5 means a frozen sheet with the new readout.

