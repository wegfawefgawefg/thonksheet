# thonksheet

A free-running brain made of bits. One blob of binary spiking neurons ticks nonstop, wires
itself, and learns without backprop. The world only writes bits onto sensor neurons and reads
bits off motor neurons. Code calls the thing a `Blob`; keep the project name out of
identifiers.

Read `docs/design.md` before touching `src/blob/`. It lists every mechanism and how each one
is laid out for the machine. `docs/plan.md` is the work queue. `docs/requests.md` records what
the owner asked for, in his words, with dates. Add to it when he asks for something new.

## Style

C+- as in `~/Coding/AGENTS.md`, splonks-cpp and Mans In My House: plain structs, free
functions, direct dispatch, a thin `main.cpp` with the loop visible. No classes with behavior,
no templates of our own, no virtuals, no inheritance. Files stay under 500 lines and carry a
meaningful name (no `utils`, no `common`). Terse lowercase "what" comments above code
paragraphs. Format with the repo `.clang-format`.

## Hardware comes first

The owner cannot stress this enough. Any mechanism is only accepted in a form the machine
runs well:

- State is flat arrays of 64-bit words and small integers, the same shape for every neuron.
  No per-neuron allocation, no pointers between neurons, no lists of different lengths.
- The hot loop is AND plus popcount over whole words. No floats in `src/blob/`. No branches
  per synapse.
- Chance is a random word ANDed down to the wanted rate, never a float compare per bit.
- A blob is deterministic for a given genome and seed.
- If a brain mechanism does not fit this shape, find the strange cheap version that does, or
  leave it out and say so in `docs/design.md`.
- Measure before and after any change to the tick with `blobbench`, and put the numbers in
  the commit message.

## Evidence, not test suites

No unit-test suites and no maintained regression harness. The evidence that something works
is a run: a learning curve from `blobrun`, a speed number from `blobbench`, or a capture from
`blobview`. Say in the commit what you ran and what it showed. If it did not learn, say that.

## Being kind to the machine

The owner uses this PC while agents work.

- Builds: `nice -n 19 cmake --build build -j4`. Runs: `nice -n 19`, one run at a time per
  agent, on at most 2 cores unless the job is a measured speed test.
- Never open a window on the real display. The viewer is tested with
  `scripts/offscreen.sh` (private Xvfb) or its own headless capture mode. New or changed
  shaders go through lavapipe first (`LP_NUM_THREADS=2`), then the real GPU only briefly
  under `timeout`.
- Do not use `pkill -f` or `pgrep -f` with text that is on your own command line.
- Disk low or memory low: stop and report. Never delete things to make room.

## Working in the repo

- Each area has one owner at a time: `src/blob/` and `src/bench/` (core), `src/view/` and
  `shaders/` (viewer), `src/world/` and `src/run/` (tasks). Stay in your area. If you need a
  change in someone else's, write it at the bottom of the current effort doc.
- `src/blob/blob.hpp` and `src/blob/genome.hpp` are the contract between areas. Only the
  core owner edits them.
- Commit finished, verified chunks often, with focused messages. Do not push; there is no
  remote.
- No GitHub Actions, no CI.
- Docs are blunt and plain: say the thing, give the number, skip the pitch.
