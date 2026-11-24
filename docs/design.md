# Design

What the blob is, which brain mechanisms it has, and how each one is shaped for an ordinary
computer. This is the reference. When code and this file disagree, fix one of them the same
day.

## The goal

A persistent brain you pump input into. It ticks nonstop on its own clock. There are no
forward passes, no episodes and no split between training and use. The world writes bits onto
sensor neurons and reads bits off motor neurons whenever it likes. A game ending resets the
game, never the blob.

It should have: locality, the ability to keep discovering new things, a sense of time,
learning without gradients, and features that end up near related features.

We want the smallest set of real brain mechanisms that is enough, each in the form the
hardware runs best, even when that form is a strange version of the biology.

## Layout

- Neurons sit on a 2D sheet of cells. The sheet is cut into **tiles** of 8x8 cells. One tile
  is 64 neurons, so one tile's spikes are one 64-bit word.
- A neuron's index is `tile * 64 + bit`, with `bit = (y % 8) * 8 + (x % 8)`.
- A **spike map** is one word per tile, plus one extra word that is always zero. Tiles off
  the edge of the sheet point at the zero word, so the loop has no edge cases.
- The blob keeps the last 16 spike maps in a ring. For 4,096 neurons that is 8 KB.
- A **sector** is a square group of tiles (2x2 by default). Regulation happens per sector.

A tile is only a storage unit. Listening areas overlap everywhere; there are no walls.

## Who a neuron listens to

Every neuron has exactly 11 **slots**. A slot is one source tile.

- Slots 0 to 8 are local: the neuron's own tile and the 8 around it, read from the previous
  tick. That is 576 possible sources.
- Slots 9 and 10 are far: a tile drawn at birth from a distance distribution with a long
  tail. Most neurons draw none (the slot's masks stay empty). Only excitatory neurons send
  far. A far slot is read from an older spike map: the delay in ticks grows with the
  distance. A pruned-empty far slot is redrawn from the same distribution at a slow rate.

Per slot the neuron stores two 64-bit masks, `w0` and `w1`. Together they are a 2-bit weight
per possible source: the code `(w1, w0)` counts 0, 1, 2, 3 and stands for strengths
**0, 1, 4, 5**. Zero means not connected. The lopsided steps are on purpose: brains have many
weak synapses and a few strong ones.

Whether a connection excites or inhibits belongs to the sender, as in the brain. One fixed
word per tile says which of its neurons are inhibitory (about 20%).

## One tick

For each neuron, for each slot, with `s` the source tile's spike word and `inh` that tile's
inhibitory word:

```
e = s & ~inh                      i = s & inh
excite  = popcount(e & w0) + 4 * popcount(e & w1)
inhibit = popcount(i & w0) + 4 * popcount(i & w1)
branch  = excite - inhibit_gain * inhibit
if excite >= branch_threshold: branch += branch_bonus
drive  += branch
```

Then, per neuron:

```
potential = potential - (potential >> leak_shift) + drive
would_fire = potential >= threshold + sector_bias  and  cooldown == 0
```

- A sensor neuron fires exactly when the world says so. Its `would_fire` is kept as its
  prediction.
- Any other neuron fires if `would_fire`, or by chance (noise).
- On firing: potential resets, cooldown starts. Otherwise the cooldown counts down.

The new spike map goes into the ring. Then regulation and learning run.

## Mechanisms

Each one: what the brain does, then what we do.

1. **Binary spikes.** Neurons send all-or-nothing pulses. We store one bit per neuron per
   tick.
2. **Leaky potential and threshold.** Input builds up and leaks away. We keep one int16 and
   leak by a shift. Each neuron has its own leak, so some are fast and some are slow.
3. **Cooldown.** A neuron cannot fire again right away. One small counter per neuron, with a
   length that differs per neuron.
4. **Excitation and inhibition by sender.** One fixed bit per neuron; one word per tile.
   Inhibitory neurons may target inhibitory neurons. That opens a pathway instead of closing
   it, and it costs nothing because nothing forbids it.
5. **Local wiring.** Most connections are short. The 9 local slots are that rule made into a
   memory layout: neighbours in space are neighbours in memory.
6. **Long fibers.** A minority of neurons reach far. Two far slots per neuron, drawn from a
   distribution, overproduced, pruned by use, redrawn slowly.
7. **Delays.** Signals take time to arrive, which lets a brain sense order and distance. Far
   slots read an older spike map; older by more the farther the source. Local stays at one
   tick. (A second set of local masks reading two or three ticks back is the next step if
   local order detection turns out weak.)
8. **Dendrite branches.** Inputs that land together on one branch count for more. A slot is
   a branch: sources from one tile are physically close to each other. If a slot's excitation
   passes a small threshold it gets a bonus. No extra popcounts.
9. **Few-level, lopsided weights.** Two bit planes, strengths 0, 1, 4, 5.
10. **Changes happen by chance.** A synapse is not nudged a little; it flips up or down a
    level with some probability. We AND the candidates with a sparse random word. That chance
    is the learning rate.
11. **Varied neurons.** Threshold, leak, cooldown length and wiring density are drawn per
    neuron from spreads. Identical neurons behave identically, which is useless.
12. **Pacemakers.** A neuron born with threshold zero and cooldown k fires every k+1 ticks.
    A few of these give the blob clocks of mixed periods and a floor under its activity. No
    extra mechanism; it falls out of the spread.
13. **Noise.** Neurons fire spontaneously now and then. One sparse random word per tile per
    tick. This is the exploration.
14. **Sector regulation (fast).** Inhibition and gain are local. Each sector counts its own
    spikes (popcount of its words, smoothed) and moves a bias on its neurons' thresholds up
    or down toward its target. Sectors can have different targets: a busy sector has finer
    time resolution.
15. **Per-neuron regulation (slow).** A neuron that fires a lot gets harder to fire. No rate
    counter: when a neuron fires, raise its threshold with a small chance; every tick, lower
    it with a much smaller chance. The balance point is the target rate.
16. **Predicting your own input.** Sensor neurons also get input from inside the blob. Each
    tick `would_fire` is the blob's guess and the world's bit is the truth. Connections that
    guessed right get stronger, ones that guessed wrong get weaker. This needs no reward and
    never stops. `popcount(predicted ^ actual)` per sector is **surprise**.
17. **Good and bad.** Reward is not a number handed in. Good and bad are ordinary sensor
    lines that are also wired to a broadcast: for a few ticks after one fires, connections
    between neurons that were both recently active move up (good) or down (bad), by chance.
    Nothing is stored per synapse for this. Each neuron already has a "fired recently" bit
    (the OR of the last few spike maps).
18. **Surprise as a second broadcast.** High surprise raises the chance of change for a
    while. Later it can also count as mildly good, which is curiosity.
19. **Growth with feature locality.** When a neuron fires, its candidates for a new
    connection are `recent & ~connected` within its slots: neurons that were just active,
    are not yet wired in, and are nearby. One is picked and starts at strength 1. Wiring
    follows correlated activity with no coordinate maths.
20. **Pruning.** When a neuron fires, weak connections from sources that were not active
    lose a level by chance; at zero they are gone. Strong connections are exempt, so what was
    learned is not overwritten by churn. A cap on connections per neuron stops runaway
    growth.
21. **Quiet phases.** Sleep: input cut, noise up, every weight has a small chance to lose a
    level, strong ones a smaller chance. Planned, not in the first build.

## Running with no input

The blob never stops, so it must not wreck itself when nothing is plugged in. An ordinary
network left running either blows up, goes flat, or overwrites what it knew. Ours has to
idle safely. This is an open design problem; the starting rules are:

- **Activity stays bounded either way.** Sector regulation, per-neuron regulation, cooldown,
  pacemakers and noise hold activity near target with or without input.
- **Silence is not evidence.** Prediction learning only runs on sensor lines a task has
  attached. An unplugged line being quiet must not teach the blob that the world is empty.
- **Change needs a reason.** Strengthening is gated by good, bad or surprise. With no input
  there is none of the three, so nothing is strengthened on noise alone.
- **Churn only touches the weak.** Growth and pruning keep going at a low rate, but strong
  connections are exempt, so idling can only tidy weak wiring.
- **Idle is the quiet phase.** No attached input for a while puts the blob in its sleep
  state (mechanism 21). That is where replay and downscaling will live.

The check for this is a run: teach a blob something, leave it unplugged for a long stretch,
plug it back in and compare.

Things that are not mechanisms: grid cells, mirror neurons, columns. Those are what neurons
end up doing. We look for them in the viewer; we do not build them.

## The genome

Everything above has numbers: spreads, chances (as shifts), targets, the far distance
distribution, sector size, sensor and motor regions. Those numbers are the `Genome`, about 50
to 200 integers. `blob_birth(genome, seed)` builds a blob from it. The blob is large; the
genome is small. Later a search over genomes, scored on how much a blob improves during its
life on tasks it has not seen, looks for a blob that learns in general.

## Why this shape suits the machine

- **Same shape everywhere.** Every neuron has 11 slots of 2 words. The loop is the same
  instructions for every neuron, which is what SIMD and GPUs want.
- **Cost grows with neuron count, not its square.** About 44 AND-plus-popcounts per neuron
  per tick, whatever the size of the blob.
- **Cache.** 4,096 neurons need 721 KB of weights, inside one core's L2. The spike history
  is 8 KB, inside L1, so even the far reads are cheap.
- **AVX-512 popcount** does 8 words per instruction on the Ryzen 9950X.
- **Learning is rare and local.** Mask edits happen only for neurons that fired, and reward
  edits only on reward ticks.
- **Bits pack.** State for thousands of blobs fits in memory, so a genome search runs many
  lives side by side, one per core now, a compute shader later. Flat word arrays port
  straight to Vulkan compute.

- **It is a stencil.** A tile computes from itself, its 8 neighbours and at most two far
  words per neuron. That is the shape GPUs and FPGAs are built for. On an FPGA a tile is a
  small block wired to its neighbours, the masks sit in block RAM, and a far link is a
  routed word. The random part is the contents of the masks, never the shape of the
  circuit, so flexibility in what features form (in time, shape and place) costs the
  hardware nothing.

Measured before this layout existed (all-to-all masks, naive C, one core): 1,024 neurons at
110,000 ticks per second; 4,096 at 11,000; 16,384 at 252 because the masks left the cache.
That cliff is why wiring is local.

## The world plug

A task talks to the blob only through:

- **Sensor lines.** The plug maps each line to a sensor neuron. Images map onto the sensor
  region as a sheet. A number (a pole angle) becomes a few lines that switch on at
  thresholds. Sound, later, is a row of frequency lines over time.
- **Motor lines.** The plug reads motor neurons and counts spikes over the task's step.
- **Good and bad.** `blob_reward` plus the two sensor lines.

The blob ticks several times per task step. The task never resets the blob.

## The viewer

The owner wants to see it: dying, saturating, cycling, self-similar activity should be
obvious at a glance. The sim runs flat out on its own thread and the viewer samples it.

- The sheet, lit by firing, colored by kind (excitatory, inhibitory, sensor, motor).
- A scrolling raster: one row per neuron or per tile, time across.
- Per-sector activity against its target, and the sector bias.
- Histograms: weight levels, firing rates, connection lengths, thresholds.
- Lines over time: grown, pruned, strengthened, weakened, surprise, good, bad.
- Click a neuron: its sources and targets drawn on the sheet, with strengths.
- Pause, single step, slow, and full speed without drawing.
