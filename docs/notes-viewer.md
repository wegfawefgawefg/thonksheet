# Viewer notes (blobview)

What the viewer is, how to run it, and what is rough. Written by the viewer job of
`docs/effort-2026-11-24-first-blob.md`.

## Running it

```
cmake -S . -B build
nice -n 19 cmake --build build -j4
./build/blobview                       # the window, the real blob, nothing plugged in
./build/blobview --fake mixed          # the viewer's own fake blob
./build/blobview --capture out.png --until 3000 --fake cycling   # no window at all
```

### Watching Echo learn

```
./build/blobview --task echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --window 50 --slow 400
```

It starts at 400 ticks a second (20 task steps a second), because this blob learns Echo
within the first 150 or so steps (3,000 ticks): at full speed that is over in a tenth of a
second. Watch the task panel (right of the raster): "last 100" goes from about 60 good to
80 good, the strip of tries turns from mixed to mostly matching, and the amber score line
leaves the blue learning-off line. Then press space for full speed: over the next 30,000
steps (600,000 ticks, under a minute) the score climbs slowly from about 0.75 to 0.9.
F runs undrawn. Every blobrun task, plug and window flag works
the same way (`--task catch`, `--rest-ticks 2`, `--g.NAME N`, ...).

Agents: never run it bare. Your shell has `DISPLAY=:0`, which is the owner's screen. Use
`--capture` (opens no window and initialises no SDL video) or `scripts/offscreen.sh`
(a private Xvfb). New shaders go through lavapipe first:

```
env -u DISPLAY LP_NUM_THREADS=2 VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json \
    nice -n 19 ./build/blobview --fake mixed --capture captures/x.png --validation
scripts/offscreen.sh --shot captures/w.png --key space --click "330 300" -- --fake mixed --frames 600
```

`scripts/offscreen.sh` defaults to lavapipe with validation; `--gpu` uses the real card.
It can press keys, click, resize the window and take a screenshot of the Xvfb screen; give
blobview `--frames N` so it ends by itself.

Flags (`blobview --help`): `--fake [mixed|healthy|dying|saturating|cycling]`, `--real`,
`--tiles W H`, `--seed N`, `--capture out.png`, `--until N` (ticks before a capture,
default 2,000), `--size W H`, `--select N`, `--select-cell X Y`, `--raster neurons|tiles`,
`--paused`, `--slow TPS`, `--turbo`, `--input` (random bits on the sensor lines of a real
blob, through `blob_sense`), `--frames N`, `--fullscreen`, `--validation`, `--no-twin`,
`--g.NAME N` (a genome number).

`--task NAME` and every other blobrun flag (`--k`, `--ticks`, `--window`, `--sensor-side`,
`--rest-ticks`, `--task-seed`, ...) go to blobrun's own parser (`src/run/run_flags.cpp`),
so a task is set up exactly as blobrun sets it up. `--ticks` is ticks per task step, as in
blobrun; the capture length is `--until` (it was `--ticks` before tasks came in, and
`--ticks` without `--task` now says so). `--steps` is read but the viewer runs until
closed. `--no-baselines` is the same as `--no-twin`.

A task capture: `blobview --task echo ... --capture out.png --until 600000` ticks the
whole way on the calling thread, waits for the twin, draws, and prints the window scores
of the blob, the learning-off twin and the coin flips; they equal blobrun's csv columns
for the same flags.

## Keys

Left hand and mouse:

| key | does |
| --- | --- |
| space | pause / run |
| E | one tick (pauses first; holds repeat) |
| W | one task step, up to its answer (pauses first; holds repeat; with `--task`) |
| S | slow motion on / off |
| A / D | slow motion half / twice as fast |
| F | full speed with nothing drawn but the tick rate; F again draws |
| T / G | tile lines / sector lines |
| R | raster rows: a row per neuron / a row per tile |
| Z | reset the zoom of the sheet and the raster |
| left click | pick a neuron (a cell on the sheet, or a row of the raster in neuron rows) |
| right drag | pan the sheet; scroll the raster rows when zoomed |
| wheel | zoom the sheet (about the mouse) or the raster rows |
| esc | drop the pick; with nothing picked, quit |
| F11 | fullscreen |

The panel has the same controls as buttons, plus good and bad (the reward broadcast) and
random input for a real blob. With a task: "task step (W)", "task marks" (the rings and
names on the sheet), and "learning" with "restart": a new life (same genome, seed and
task) with `genome.learning_on` set from the box.

## What each view shows

- **Sheet** (top left). Every neuron one cell, dim in its kind's colour (excitatory amber,
  inhibitory blue, sensor green, motor pink), lit when it fires and fading over `fade ticks`
  (6 by default). On the newest tick a cell is lit a little whiter. Tile and sector lines are
  hidden by themselves when a tile is under 24 pixels (big sheets). With a neuron picked:
  a thick white ring on it; a ring round each source (white excitatory, blue inhibitory,
  violet far, dim and thin for strength 1, bright and thick for 4 and 5); a dot on each
  target; violet lines for far links.
- **Recurrence** (top middle). Pixel (i, j) is how alike tick i and tick j were (shared
  spikes over all spikes) over the last 256 ticks. Grey is how alike at all; amber is more
  alike than the usual pair. The diagonal is each tick against itself. Diagonal lines beside
  it are a cycle with that period; a flat bright square is a stuck pattern; plain dark
  speckle is healthy. Dark red is both ticks silent.
- **Raster** (bottom). Time across, newest at the right, the whole tape (1,024 ticks, fewer
  on very big sheets). One row per neuron (in index order, so 64 rows per tile) or per tile.
  When a pixel covers several rows it shows the share that fired, against the target rate:
  on target is about a third lit, three times target nearly full, dead rows black. The
  picked neuron's row has a white mark at the left edge.
- **Lines over time** (middle). Fired, surprise, grown, pruned, strengthened, weakened,
  good, bad: a point per tick over the tape, or a point per 256 ticks over the last 262,144
  ("long history"). Fired is scaled against twice the target, so dying sits low.
- **Task on the sheet** (with `--task`). A ring round every group of cells the plug has
  attached: good green, bad red, the task's own lines white, motor lines pink. A sensor
  group is filled while its line is on. The motor group that gave the last answer is filled
  green (good) or red (bad). Names above the groups: good, bad, cue 0, cue 1, motor 0,
  motor 1 (go / no-go, ask, press / rest, left / right, bitmap WxH, cart x ... by task).
  Round the whole sheet a frame in the last answer's colour fades out over one step, so a
  good or bad event shows the moment it lands (and the good and bad lines tick up).
- **Task** (right of the raster, with `--task`). The task and its settings, learning on or
  off, the step and the tick inside it. The last try in words (cue 1, answered motor 0:
  bad) and the last 100 tries as good / bad / still. A strip of the last tries, newest at
  the right: what was wanted on top, what the blob did under it (blue motor 0, amber motor
  1, dark: held still), and a green or red bar. Under it the score per window over the
  whole life: the blob amber, its learning-off twin blue, a dashed blue line at the twin's
  mean and a dashed grey line at the coin flips' mean (chance). Past 60 windows the raw
  scores go faint and a running mean is drawn bold.
- **Health** (side). Silent (never fired on the tape), stuck (fired on more than a quarter of
  the ticks), spikes per 1,024 per tick, and the strongest repeat: the overlap of each tick
  with the one `lag` ticks before, for lags 1 to 64, against what chance would give. Each
  number turns red when it is out of a sane range. Then the rhythm: the fired count per
  tick correlated with itself `lag` ticks later (-1..1, over the whole tape). A sheet that
  pulses every P ticks peaks at P, 2P, ...; the period is the first peak above 0.3 that is
  at least 0.6 of the highest, and it is marked (amber lines at P, 2P, ...) on that curve
  and on the overlap curve, which is now scaled to its own range so its shape shows.
- **Neuron** (side). The picked neuron's numbers: threshold plus its sector's bias,
  potential, leak, cooldown, fan-in by strength, inhibitory and far sources, targets, far
  slots with their delays, sources by delay, and its own spikes over the last 256 ticks.
- **Sectors** (side). A map of the sectors coloured by activity over target (blue under,
  grey on, red over, black dead) with ratio and bias written in when the cells are big
  enough, and the spread.
- **Histograms** (side). Weight levels (1, 4, 5, and the empty count), spikes per neuron
  (log bins), thresholds, fan-in, connection lengths in cells (local hump, far tail).

## How it is built

- `ViewFrame` (`src/view/view_frame.hpp`) is everything drawn, as plain numbers. Nothing
  past it reads a `Blob`.
- `frame_fill.cpp` fills it from a `const Blob&`; `tape.cpp` records every tick's spike map
  and counts into a ring (the tape). Far delays are read in one function there
  (`slot_delay`); when `blob.hpp` gets `slot_delay[neuron * SLOTS + slot]` that function is
  the one line to change, and the neuron panel already lists sources by delay.
- `fake_birth.cpp` and `fake_tick.cpp` make a fake blob: a real `Blob` struct the viewer
  allocates and moves itself. Its activity is made up; its sectors can die, saturate or
  cycle on purpose so each kind of trouble can be looked at. Sensors and motors are
  lattices at the genome's `sensor_stride` / `motor_stride`, as in the design.
- Tasks: `src/run/task_ticker.cpp` is one task step spread over blob ticks
  (`ticker_before_tick`, then the caller ticks, then `ticker_after_tick`, true when the
  step's answer is in). blobrun's `run_life` and the viewer both use it, so a task runs
  the same in both (checked: same window scores). `sim_task.cpp` drives it on the sim
  thread, keeps the tries and window scores, and turns the plug's groups into `TaskLine`s;
  `sim_twin.cpp` runs the learning-off twin and the coin-flip stand-in on one more thread,
  a whole step at a time, never past the main blob's step count (two atomics between
  them; the score lists behind a mutex). The score lists only grow, so each copy takes
  just the new tail (`TaskScores`). `panels/task_panel.cpp` draws the panel and the names
  on the sheet; the rings are marks (`renderer/marks.cpp`), so no shader changed.
- `sim.cpp`: the sim thread owns the blob and ticks it (flat out, slow, paused or single
  steps). The locking is one mutex: the sim thread takes it only to fill the shared
  `ViewFrame` when the viewer asks; the viewer takes it only to copy it out. The tape is
  copied incrementally (only new rows), so a 100,000-neuron sheet costs the new rows per
  frame, not the whole tape. Everything else between the threads is one atomic number each.
  Whole-blob histograms are refilled every 6th snapshot.
- `frame_derive.cpp`: what the viewer works out from its own copy of the tape (rates,
  silent, stuck, the lag curve), so the sim thread never pays for it.
- `renderer/`: the Vulkan side. Every view and the ImGui panels draw into one screen
  image; with a window it is blitted to the swapchain, with `--capture` it is read back. The
  sheet, raster, recurrence and marks are fragment and vertex shaders (`shaders/`) reading
  the raw spike words from one storage buffer per frame slot; ImGui draws only the panels
  and plots. Every GPU wait has a 5 second timeout and a lost device ends the loop.
- `main.cpp` has the loop in paragraphs: pump input, take the latest snapshot, the panels,
  draw and present (or save the capture), rates.

### Mans In My House alignment

- Follows MIMH: one `State` in `state.hpp`; `main.cpp` with the renderer and window as
  locals and the loop in commented paragraphs; `cli.cpp` (`CliOptions`, `parse_cli`, the
  capture options), `inputs.cpp`; the renderer laid out as MIMH's `src/renderer`:
  `device.hpp` (the face), `vulkan_internal.hpp` (a `vk::Context` with `Buffer`, `Image`,
  submission `Slot`s taken in turn), `vulkan_context.cpp` (instance, device, swapchain,
  present), `vulkan_resources.cpp` (buffers, images, layouts tracked per image, begin and
  finish), `vulkan_pipeline.cpp`, `vulkan_draw.cpp`; draw into a screen image, then blit it
  to the window or read it back, as MIMH does. The ImGui layer has Gubsy's function names
  (`init_imgui_layer`, `imgui_new_frame`, `imgui_process_event`, ...).
- Not from Gubsy or GView, on purpose:
  - Gubsy's engine is an SDL_Renderer host with menus, lobbies, networking, SDL_mixer,
    SDL_ttf and SDL_image. None of those are installed here, so linking it means fetching
    and building SDL3 and its three satellites from source, and it brings audio and
    networking into a viewer that needs neither. Its ImGui layer draws through an
    SDL_Renderer; MIMH only gets it onto Vulkan by replacing SDL_Renderer wholesale
    (`sdl_bridge.hpp`), which is game renderer code, not something to copy.
  - GView is for game menus and HUDs; this viewer's panels are a dev tool, which is what
    ImGui is for.
  - So the window and input are plain SDL3 (video only, no audio), and Dear ImGui is
    vendored in `third_party/imgui` (v1.92.9b with the SDL3 and Vulkan backends, unmodified;
    Gubsy's copy has no Vulkan backend).
- Still foolswud-shaped: the device-picking and swapchain code started from foolswud's and
  keeps its shape (Vulkan 1.3, dynamic rendering, synchronization2), and ImGui came from its
  vendored copy.

## Checked

- Captures on lavapipe with validation: 4,096 neurons (mixed, cycling, dying, saturating
  sectors) and 102,400 (40 x 40 tiles). No validation messages. A 102,400-neuron capture
  with 1,500 fake ticks takes 2.7 s on lavapipe.
- The window under a private Xvfb on lavapipe with validation: 55 fps drawn, about 14,700
  fake ticks per second (the fake's tick is the limit; full speed undrawn gave the same),
  pause, slow (10 ticks/s), single step, full speed undrawn, picking by click, raster rows
  key, resize to 1300 x 800. No validation messages, clean exit.
- The real GPU (RX 9070 XT, RADV), headless capture only, 1100 x 700, under `timeout 20`:
  the same picture as lavapipe. The window has not been run on the real GPU.
- The real blob with Echo (k=2, 20 ticks a step, sides 3), lavapipe with validation, no
  messages. A capture of 6,000 steps with window 500 printed blob, learn_off and coin
  window scores identical to `blobrun ... --steps 6000 --window 500`. blobrun's csvs were
  byte-identical before and after its step loop moved into `task_ticker` (echo, go-no-go
  with `--rest-ticks 3`, delayed-echo, sequence).
- Captures of one life (window 100), `captures/echo-tick2000.png`, `-tick20000`,
  `-tick600000` in the viewer's worktree:
  - tick 2,000 (step 100): last 100 tries 66 good / 28 bad / 6 still; one window, 0.66.
    Weights 49% at 1, 26% at 4, 25% at 5; sectors 1.6x over target, every bias at +16.
  - tick 20,000 (step 1,000): windows 0.66, 0.86, 0.84, 0.89, ... 0.82; learning off
    about 0.50, coin 0.48. Weights 8% / 13% / 78%; fan-in piling at the cap.
  - tick 600,000 (step 30,000): 300 windows. Means over blocks of 2,500 steps: 0.77,
    0.75, 0.73, 0.76, 0.82, 0.83, 0.83, 0.84, 0.86, 0.87, 0.89, 0.90. So a fast jump from
    0.5 to about 0.75 in the first 150 steps, a dip, then a slow climb to 0.9. Learning
    off 0.50, chance 0.48. Weights 5% / 9% / 87%, fan-in almost all at 96, activity 0.79x
    target, biases spread -9..+16. The recurrence square goes from a checker of 20-40
    tick blocks to plain speckle with one big block.
- Speed with a task: about 47,000 ticks a second in a capture (one thread for the blob,
  the twin on another); 68,000 in the window at full speed under Xvfb.
- The window in a private Xvfb on lavapipe with a task: slow 400, pause, W twice, E, F and
  back, learning off + restart (a fresh life, grown/pruned flat, blob = twin). Clean exit.
- Real GPU, headless task capture, under `timeout 20`: the same picture as lavapipe.

### Rhythm

The fired count's period (health panel), 4,096 neurons, seed 1:

| run | period | r at that lag |
| --- | --- | --- |
| random input (`--input`), tick 3,000 | 26 ticks | 0.67 |
| Echo, learning on, tick 2,000 | 12 ticks | 0.63 |
| Echo, learning on, tick 20,000 and 600,000 | none clear (smooth decay, no peak) | |
| Echo, learning off (restart), about tick 100,000 | 24 ticks | 0.54 |

The pulse with random input reads as stripes in the raster and a checker in the recurrence
square. It is the same life as the first round's `captures/real-input.png` (the picture is
identical after `--input` moved onto `blob_sense`); by eye that looked like about 20
ticks, measured it is 26. Learning on a task washes it out; a learning-off life keeps it.

## Rough

- Neuron rows of the raster on a 4,096 sheet are 16 per pixel, so per-neuron rhythm blurs;
  zoom in with the wheel, or use tile rows. Cycles show far better in the recurrence square
  and the lag curve than in the raster.
- Sector targets are the genome's `sector_target_per_1024` times the sector's neurons;
  if the core changes what `sector_activity` means, fix `fill_sectors` in `frame_fill.cpp`.
- The fake's learning stand-in drifts fan-in up to the cap over ~50,000 ticks, and its
  regulation does not feed back (biases pin at the limits). It is a stand-in, not a model.
- Neuron targets through far slots are found by scanning every neuron's far tiles on each
  snapshot while a neuron is picked: fine at 100,000, worth a look at a million.
- Echo is learned in about 150 steps, so at full speed the learning itself is invisible;
  start slow (`--slow 400`) or paused and use W.
- The task panel's strip colours only motor 0 to 3 apart (blue, amber, violet, pink, then
  round again); echo with k > 4 repeats colours.
- Names on the sheet overlap when cells are small (big sheets, or a catch bitmap's many
  groups; only the bitmap's first group is named).
- The twin is not shown for a learning-off life apart from being equal to the blob.
- No task switching from the UI (not asked for); restart keeps the task and seed.
- The sim thread's snapshot copies every `TaskLine` and the 256 tries each frame; fine
  for every task here (catch is the most lines, 38).

## Needs from other areas

- Nothing open.
