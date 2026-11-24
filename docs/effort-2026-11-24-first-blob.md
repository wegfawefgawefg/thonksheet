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
   - SDL3 + Vulkan + ImGui, structured like `~/Coding/Graphics/foolswud` (`src/vk`,
     `src/app`). Vendor ImGui the same way.
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
