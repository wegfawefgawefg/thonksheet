# Viewer notes (blobview)

What the viewer is, how to run it, and what is rough. Written by the viewer job of
`docs/effort-2026-11-24-first-blob.md`.

## Running it

```
cmake -S . -B build
nice -n 19 cmake --build build -j4
./build/blobview                       # the window, the real blob (fake until the core lands)
./build/blobview --fake mixed          # the viewer's own fake blob
./build/blobview --capture out.png --ticks 3000 --fake cycling   # no window at all
```

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
`--tiles W H`, `--seed N`, `--capture out.png`, `--ticks N`, `--size W H`, `--select N`,
`--select-cell X Y`, `--raster neurons|tiles`, `--paused`, `--slow TPS`, `--turbo`,
`--input` (random bits on the sensor lines of a real blob), `--frames N`, `--fullscreen`,
`--validation`.

## Keys

Left hand and mouse:

| key | does |
| --- | --- |
| space | pause / run |
| E | one tick (pauses first; holds repeat) |
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
random input for a real blob.

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
- **Health** (side). Silent (never fired on the tape), stuck (fired on more than a quarter of
  the ticks), spikes per 1,024 per tick, and the strongest repeat: the overlap of each tick
  with the one `lag` ticks before, for lags 1 to 64, against what chance would give. Each
  number turns red when it is out of a sane range.
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
  lattices (every 2nd cell), as in the design.
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
- The real-blob path (`BLOB_HAS_CORE`) compiles (`-fsyntax-only`) but has never run: the
  core did not exist yet.

## Rough

- Neuron rows of the raster on a 4,096 sheet are 16 per pixel, so per-neuron rhythm blurs;
  zoom in with the wheel, or use tile rows. Cycles show far better in the recurrence square
  and the lag curve than in the raster.
- Sector targets are the genome's `sector_target_per_1024` times the sector's neurons;
  if the core changes what `sector_activity` means, fix `fill_sectors` in `frame_fill.cpp`.
- The fake's learning stand-in drifts fan-in up to the cap over ~50,000 ticks, and its
  regulation does not feed back (biases pin at the limits). It is a stand-in, not a model.
- `--input` writes `sense_in` and `sense_attached` directly every tick (1 in 8 sensor
  lines on). Once the world plug exists the viewer should drive a task through it instead.
- Ticks per second in a capture read 0 (there is no clock there).
- Neuron targets through far slots are found by scanning every neuron's far tiles on each
  snapshot while a neuron is picked: fine at 100,000, worth a look at a million.

## Needs from other areas

- Core: `slot_delay` in `blob.hpp` (coming); then change `slot_delay()` in
  `frame_fill.cpp`. If `sensor_stride` / `motor_stride` land in `genome.hpp`, the fake
  should read them (it hard-codes 2).
