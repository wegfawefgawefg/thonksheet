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

## Agreed 2026-11-24: freeze, run, cut, and rivals

Nothing has run yet and the design has 21 mechanisms, which is the too-many-variables
problem. So: no new mechanisms until something has learned; run Echo; then switch each
mechanism off in turn and delete what makes no difference. The elegant core to aim for is
three things: a neuron is a popcount over a mask against a threshold; a synapse is a tiny
counter moved by chance on an outcome; something keeps activity alive.

Rivals, all on the same tasks through the same plug. The brain-style rule has to beat them
to earn its complexity:

1. **Frozen blob, learned readout.** No learning inside; only the motor neurons' input masks
   learn. If the full blob cannot beat this, the inner learning is not helping.
2. **Random flips.** Flip a few mask bits, live a while, keep the flips if reward went up,
   undo them if not.
3. **Noise is the experiment.** The blob knows which spikes were noise. Only those neurons
   get graded: reward above its running average strengthens their recently active inputs,
   below weakens them. This is the owner's original snake method at the synapse.
4. **Table neurons.** A different substrate, small and separate: a neuron reads 6 input bits
   and looks up a 64-entry table of tiny counters. One lookup per tick, any function of its
   inputs, and it is the cell FPGAs are made of. Learning nudges the entry that was just used.

## Asked for, queued behind the gate

- **Layers inside a tile.** Split the 64 bit positions into input, middle and output groups;
  birth and growth rules depend on the sender's and receiver's group. A constant mask, no
  cost per tick. Strength is a genome number, zero meaning today's flat tile, so it can be
  measured. Owner: "layers would be good i think, would force feature locality. adds
  sparseness for sure. reduces propagation bandwidth i think."
- **Scale.** 4,096 neurons is far too few for anything past the first reflex tasks; it is
  the size for checking the learning rule fast. Next: the tick across all cores (it is a
  stencil), then sheets of 100,000 to 1,000,000 neurons, then the GPU.
- **Throughput numbers in `blobbench`.** Print, per size: neuron updates per second,
  possible-synapse checks per second (704 per neuron per tick), real synaptic events per
  second (a spike arriving over an existing connection), signal bits per tick (entropy of
  the spike map), and stored bits (connections chosen plus their strengths). These are the
  numbers to set beside a float network's multiply-adds and weights. Owner: "the total
  throughput of the machine i think can be determined no? and matched to different neural
  networks?"
- **Fewer slots per neuron.** A neuron scans 704 possible sources to use at most 96. Let
  each neuron listen to a drawn subset of its 9 local tiles (say 4) plus the far slots:
  about half the work and half the memory per neuron, and more varied listening areas. A
  genome number; measure learning and speed against the full 9.
- **Fine locality inside a tile**, as a birth and growth preference by cell distance, also a
  genome number.

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
