# Effort: first blob (2026-11-24)

Goal: one hand-set blob that stays alive, can be watched, and visibly learns the Echo task.
Three jobs run side by side on separate folders, then get put together.

## Jobs

1. **Core** (`src/blob/`, `src/bench/`). Owns `blob.hpp` and `genome.hpp`.
   - `blob_birth`, `blob_free`, `blob_tick`, `blob_reward`, as in `docs/design.md`.
   - Split by job: birth, integrate-and-fire, regulation, learning, random words.
   - `blobbench`: ticks per second and neuron updates per second at several sizes, one core.
   - Its own small check that learning does something: a few sensor lines, a few motor
     neurons, good when they match. Tune the genome defaults until the curve moves, or
     report plainly that it does not and what was tried.
   - A stage 0 run: a million ticks idle and with random input, activity per sector logged.
2. **Viewer** (`src/view/`, `shaders/`, `scripts/offscreen.sh`).
   - SDL3 + Vulkan + ImGui, following Mans In My House (`~/Coding/GameDev/mansinmyhouse`):
     Gubsy/GView for the window, input, settings and ImGui layer, and its `src/renderer`
     conventions for the Vulkan side. (First briefed against foolswud by mistake; the owner
     corrected it the same day.)
   - Draws only from a plain snapshot struct it owns (`ViewFrame`), filled from a `Blob` by
     one function. A fake filler stands in until the core lands.
   - The views listed under "The viewer" in the design. Headless capture to PNG.
3. **World** (`src/world/`, `src/run/`).
   - The plug: sensor lines, motor lines, good, bad; value-to-lines encoding.
   - Stage 1 tasks from `docs/curriculum.md`: echo, shuffled echo, delayed echo, go/no-go,
     beat. Then catch and cartpole if there is room.
   - `blobrun`: runs one blob on one task for a long life, writes score per window to CSV,
     prints late minus early, and runs the two baselines.

## Constraints

- Everything in `AGENTS.md`, above all the hardware rules and the kindness rules.
- Stay in your folders. `blob.hpp` and `genome.hpp` change only through the core job.
- CMake picks up new `.cpp` files by folder; nobody needs to edit `CMakeLists.txt` for that.
  If you must change it, keep the change inside your own target's block.
- Commit as you go on your branch. No pushing.

## Handoff notes

Each job appends here when it finishes: what works, what was measured, what is rough, and
anything it needs from another area.

### Put together (coordinator, 2026-11-24)

All three jobs are merged. Joining fixes: the viewer and its fake use `blob_weight_index`
and `slot_delay`; `blobrun` uses `learning_on` for its learning-off twin and takes
`--g.name value`; the plug maps lines onto the sensor and motor lattices and packs them from
the corner of each area nearest the other; `genome_args.cpp` moved into `src/blob/`.

First real Echo runs, `blobrun --task echo --ticks 20 --sensor-side 3 --motor-side 3
--steps 40000 --window 1000`, output area moved to share an edge with the input area
(now the genome default). Score is right presses over tries, mean of the last 4 windows:

| run | blob | learning off |
| --- | --- | --- |
| echo k=2, seed 1 | 0.89 | 0.50 |
| echo k=2, seed 2 | 0.84 | 0.49 |
| echo k=2, seed 3 | 0.72 | 0.72 |
| echo k=2, seed 4 | 0.95 | 0.28 |
| echo k=4, seed 1 (chance 0.25) | 0.46 | 0.16 |
| shuffled echo k=2, seed 1 | 0.96 | 0.51 |

So the gate holds on three seeds of four. What did not work: the plug's first defaults
(8 ticks per step, 2x2 groups, areas touching only at a corner) gave no learning at all
(0.30 against 0.28), which matches the core's finding that a cue fades about 3x per tile.
Seed 3 does not beat its own learning-off twin. k=4 learns but stays under half right.
