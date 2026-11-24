# Plan

The work queue, in order. Status is updated as things land. Detail for the current effort is
in its own dated doc.

## Now: first blob (docs/effort-2026-11-24-first-blob.md)

1. Core: birth, tick, regulation, learning, speed test. *in progress*
2. Viewer: the sheet, raster, sector bars, histograms, neuron picker. *in progress*
3. World plug, the stage 1 tasks, the headless runner with baselines. *in progress*
4. Put the three together. Stage 0: does it stay alive, with and without input.
5. **The gate: one hand-set blob visibly learns Echo.** Nothing below starts until this
   holds.

## Next

6. Stage 1 in full, including shuffled echo and the timing tasks.
7. The idle check: teach, unplug for a long stretch, plug back in.
8. Quiet phases (sleep): downscaling and replay.
9. Stage 2 tasks, then stage 3 (catch, cartpole, snake first).
10. Tick speed: hand-written AVX-512 path, then many blobs across cores.

## Later

11. Blobs past 16,000 neurons; check the cache behaviour holds.
12. Genome search over shuffled tasks, scored late minus early.
13. Tick in a Vulkan compute shader, once the search needs thousands of blobs. Measure
    against the CPU first.
14. Sound-shaped input, larger sensor sheets, walking.
15. FPGA sketch of one tile, if the GPU version shows the stencil shape holds up.

## Open questions

- Does one good/bad broadcast plus prediction give purposeful play at all? Unknown. The
  gate above is how we find out.
- Do delays matter? Every slot has one now; compare against a genome with all delays at 1.
- What should idle do beyond staying safe: replay what, downscale how fast?
