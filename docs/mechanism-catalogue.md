# Mechanism catalogue

Written 2026-11-25 as library research, in answer to the owner: "you tested like what, 20
mechanics? shouldnt there be a fuckton of possible ones?" and "ideally i could just set you
off for a week, you try a zillion different random mechanics." This is the list to draw from.
166 entries, mined from neuroscience, machine learning without backprop, invariance and
binding, and learning hardware, plus 20 from the owner's own books on disk (section H).
186 entries in all. Each has a source.

How sure the sources are. About 100 of entries 1 to 166 were checked against the paper's own
abstract, publisher page or a full-text copy during this pass; two were read in full for
their rule details (Baldassi et al. 2007, entry 15; Rombouts et al. 2015, entry 99). The 20
entries in section H were read from the owner's copy of the Handbook of Brain Theory and
Neural Networks (Arbib 2003) and Hawkins 2021. Sources marked (m) are cited from memory, with
the link written from memory; the claim is standard but the link was not re-opened this
pass. Where the literature disagrees, the entry says so.

## How to read an entry

- **Aim** is the wall or property it is pointed at: *inside* (neurons inside the sheet
  learning usefully from outcomes), *gap* (holding something across more than about 20
  ticks), *invariance*, *acting*, *capacity* (more than about eight things), *alive*,
  *keeping* (not losing what was learned), *speed*.
- **Here** is how it would look on this substrate: what is stored, when it is touched, and
  the bit operations.
- **Cost**: *fits* (bit masks, touched rarely), *awkward* (per-connection counters or
  per-tick arithmetic), *does not fit* (real numbers or global solves).
- **Ideas** gives the number in `docs/ideas.md` if it is already there, else "new".
- **Matters**: high, medium or low, a guess, with the reason.

Substrate words used below: a *tile* is 64 neurons in one 64-bit word; a neuron has 9 local
and 2 far *slots*, each two masks (`w0`, `w1`) for a 2-bit weight per possible source; the
*ring* holds the last 16 spike maps; *recent* is the OR of the last few maps; *evidence
counters* are the hidden 6-bit counters per possible source of finding 14.

---

## First fifteen to try, per wall

### Inside learning (neurons inside the sheet learning usefully from outcomes)

1. **Action-feedback tags (98, AGREL/BrainProp).** The pressed button's cells name their
   active sources, those get a tag bit, and the existing good/bad bit moves the tagged
   neurons' evidence; it is finding 15's one bit pushed one or two hops inward, and it is
   proven equal to backprop for one output at a time.
2. **Hidden states with a "barely correct" rule (15, Baldassi).** The closest published
   match to our evidence counters and error rule, on binary synapses, and it says exactly
   when to strengthen on good (only when the margin was thin), which is the missing half of
   finding 17.
3. **Direct random target projection (89, DRTP).** Each inner neuron is assigned a random
   button at birth and moves its evidence toward its active inputs when that button was
   right; one small int per neuron, touched only on reward, no backward path at all.
4. **e-prop (95).** Per-connection eligibility ("source fired before I did") times a
   per-neuron learning signal from fixed random feedback of the error; the cleanest online
   recipe for recurrent spiking nets, and our recent-bits are already its eligibility.
5. **Competition plus adaptive thresholds plus Hebb (116, Diehl and Cook).** Unsupervised
   STDP with lateral inhibition and a threshold that rises with use reached 95% on MNIST;
   it grows features the readout can select, which is the shape that already works here.
6. **Expansion with sparsening by feedback inhibition (56, mushroom body).** Not learning
   inside but making the inside linearly readable: k-winners per tile set by an inhibitory
   loop; the fly shows that without the loop, similar inputs stop being learnable.
7. **HTM spatial pooler (129).** Permanence counters (our evidence counters), k-winners and
   boosting of quiet columns; a known, bit-friendly recipe for growing sparse features online.
8. **Separate traces for strengthening and weakening (9).** Two eligibility bits per neuron
   with different lifetimes, converted by good and by bad respectively; cheap, and it lets
   good and bad reach back different distances.
9. **Burst-dependent plasticity (30).** A second kind of spike (two in a row) marks "should
   have fired more"; feedback raises burst odds; sign of change set by burst versus single.
10. **Two phases with the answer shown (91, contrastive Hebbian / equilibrium propagation).**
    Run a few ticks free, a few with the right button clamped, wire by the difference; the
    sheet is already recurrent, which is what these rules need.
11. **Forward-forward (93).** Each tile gets a local "goodness" goal (activity high on good
    stretches, low on bad); no signal passes between tiles.
12. **Decision-sized variability (80, songbird LMAN).** Noise injected into the motor
    pathway, as big as a decision, with per-connection eligibility in the sheet; it answers
    finding 12 directly.
13. **Dendritic prediction (29).** Each neuron learns to predict its own firing from its far
    slots, with the error as a third factor; gives every inner neuron a local target.
14. **Behavioral-timescale plasticity (11).** A rare "plateau" event in a neuron rewires it
    in one shot toward whatever was active within a wide window; one-shot, seconds-wide.
15. **Evolve the rule (136).** Let the genome search write the rule itself from a small
    grammar of bit operations, scored across the task map; this is the "set it off for a
    week" job in its proper form.

### Holding across a gap (more than about 20 ticks)

1. **Memory units fed by on/off cells (99, AuGMEnT).** A share of neurons with no leak that
   take input only from change detectors and are reset rarely; with action-feedback tags
   they learned delayed tasks from reward alone.
2. **Adaptation as memory (145, LSNN).** Some neurons get a threshold that jumps on a spike
   and decays over hundreds of ticks; this alone gave spiking nets LSTM-like memory.
3. **Short-term facilitation (21).** A per-source "fired lately" word that makes recent
   senders count extra for hundreds of ticks: memory held in synapses, not in firing.
4. **Bistable or graded persistent cells (81).** Single cells that keep firing at a level
   after input stops; built as a latch bit per neuron, exempt from rate regulation.
5. **A bump region (67).** Local excitation, wide inhibition and slow excitation relative to
   inhibition; idea 111/112's region with its own numbers.
6. **Log-spaced time cells (62).** A bank of leaky counters with leaks spaced by ratio, and
   cells that read differences of neighbors: a compressed timeline of the recent past.
7. **Learned gating of what enters memory (65, PBWM).** A basal-ganglia-like gate, trained by
   reward, decides when a memory region updates; without it, holds get overwritten.
8. **Chains from timing plus a budget (75).** STDP alone does not form long sequences; with
   a fixed budget per neuron it does, with lengths spread as 1/L.
9. **Feedforward memory (77).** Activity passed down a chain of states holds a value
   without any attractor; our delays and far links are half of this already.
10. **Fast weights (143).** A second, fast-decaying weight plane written by co-activity and
    read in the same popcount; activity-silent memory.
11. **Polychronous groups (76).** Delays plus timing-based wiring give many more stored
    timed patterns than neurons.
12. **One long delay loop (122).** A reservoir made of a single node and a long delay line
    works; a ring longer than 16 maps for a few relay cells is the cheapest test.
13. **Credit that lasts seconds (8).** An eligibility trace with a delayed onset and a long
    tail (dopamine's 0.3 to 2 s window); not holding content, but needed to learn to hold.
14. **Aggregate-label learning (109).** Feedback is only a count of cues seen over a long
    stretch, with no timing; the neuron finds which feature predicted it.
15. **A hold mode set by a modulator (37).** Acetylcholine switches cortex between taking in
    new input and keeping recurrent activity; a global bit that cuts recurrent learning and
    lowers leak while a hold is needed.

### Invariance (position, scale, turn, tempo, pitch, viewpoint)

1. **The trace rule with drifting stimuli (120).** Wire with a slow "fired recently" so
   things seen one after another map together; monkey IT changes its position tolerance this
   way within an hour of altered viewing. It needs the world to show motion.
2. **OR-pooling cells (147).** A cell fires if any one of several shifted copies of a
   detector fires (max pooling); idea 72/103.
3. **Position as a bit rotation (157).** In vector-symbolic computing a shift is a
   permutation; on 64-bit tile words a rotate by 1 or 8 is a one-instruction shift of a
   pattern, so a readout can read over shifted copies for free.
4. **Maps where the change is a slide (149).** Log-polar retina, log-frequency cochlea,
   log-spaced time constants: scale, turn, pitch and tempo become shifts.
5. **Stored templates and their orbits (148).** Remember the views of a few templates as
   they transform and pool over them; any new pattern's signature is then invariant.
6. **Move the sensor (79).** Active sensing; fixation turns position into a motor fact.
7. **Routing by control cells (153).** A few control neurons choose which input tile feeds
   a central window; position is undone before recognition.
8. **Shunting for tempo (150).** Input conductance shortens the effective time constant, so
   a pattern played faster drives the cell the same way.
9. **Divisive normalization (73).** Thickness, brightness, loudness divided out; idea 108.
10. **Slowness (119).** Learn the features that change slowest; the principled version of 1.
11. **Transient synchrony (151).** Neurons with different decay rates fall into step only
    for one pattern, at any tempo inside a range.
12. **First-spike order (152).** Rank order of first spikes is unchanged when intensity
    scales.
13. **Locations on the object (84).** Learn features at locations in the object's own frame
    (grid-like codes), so where the object sits on the sensor stops mattering.
14. **Factor what from where (126).** A resonator splits a bound vector into its factors
    (shape, position) by alternating guesses.
15. **Agreement between parts (154).** Parts vote for a whole and its pose; the vote agrees
    whatever the pose (capsules, GLOM).

### Notes on the lists from the handbook (section H)

- **Capacity** has no list of its own, but the strongest single lever found is entry 169
  (many small thresholded branches, connections placed by co-activity with branch
  neighbors: 46 times the patterns of a linear cell in Mel's numbers), then 53 (context
  gating), 15 (hidden states with the barely-correct rule) and 56 (sparsening).
- **For the gap**, entry 180 is an argument more than a mechanism: an integer counter with no
  leak integrates perfectly, so the latch and memory cells at the top of the gap list are
  cheaper here than in a brain. Entry 174 (a population that keeps changing after a cue,
  read by the readout) is the alternative when the task is "act at the right moment" rather
  than "remember which".
- **For acting**, entry 167 (cancel self-caused input with anti-Hebbian learning on a copy of
  the motor command) belongs beside the actor-critic (64) and the songbird experimenter (80).

## Pieces that only work together

These are combinations where the source shows a piece doing little or nothing alone.

- **Context gating with synaptic stabilization** (53). Masse et al. 2018: each helps only
  partly; together a network keeps 100 sequential tasks.
- **Timing-based wiring with a per-neuron budget** (75). Fiete et al. 2010: STDP alone does
  not organize long sequences; heterosynaptic competition (a budget) plus STDP does. This is
  finding 1 again, from another field.
- **Hebb with lateral inhibition with adaptive thresholds** (116). Diehl and Cook 2015: Hebb
  makes prototypes, inhibition makes them compete, the threshold gives every neuron a turn;
  the authors give each a separate role.
- **Intrinsic plasticity with Hebb** (18). Triesch 2007: together a neuron finds the
  heavy-tailed (sparse) direction in its input; Hebb alone finds the principal one.
- **Hebb with fast compensation and slow homeostasis** (19). Zenke and Gerstner 2017: slow
  homeostasis alone cannot keep Hebbian learning stable; a fast compensatory process is
  needed as well.
- **Random expansion with sparsening by feedback inhibition with output-only learning**
  (56). Lin et al. 2014: breaking the inhibitory loop leaves the expansion in place and the
  fly still cannot learn to tell similar odors apart.
- **Memory units with on/off inputs with tags from the chosen action with a global error**
  (99). Rombouts et al. 2015: memory units integrate only transient (change) cells, or they
  would integrate a held input forever; the tags give the credit.
- **Eligibility trace with a modulator with ongoing random firing** (8). Izhikevich 2007:
  the slow trace is what bridges seconds; random firing during the wait does not disturb it.
- **Actor with critic** (64). Barto, Sutton and Anderson 1983: the actor (ASE) alone could not
  reach the pole-balancing performance; the critic (ACE) gives it more frequent feedback.
  Our cartpole is an actor with no critic.
- **Hidden synaptic states with the barely-correct rule** (15). Baldassi et al. 2007: hidden
  states with the error rule alone take time exponential in network size; adding the
  barely-correct update makes it fast and roughly doubles capacity.
- **Components on several timescales that talk to each other** (13). Benna and Fusi 2016:
  the coupling between components is what raises capacity.
- **Inhibitory plasticity with excitatory memories** (25). Vogels et al. 2011: inhibition
  learns to cancel stored patterns so they sit silent in background activity yet can be
  recalled; the excitatory memory alone saturates the network.
- **Slow excitation against fast inhibition** (67). Compte et al. 2000: a bump holds only if
  recurrent excitation is slower than the inhibition; with fast excitation it oscillates or
  drops out.
- **Facilitation with background activity** (21). Mongillo et al. 2008: the synaptic memory
  is refreshed by spontaneous population spikes; how long it lasts is set by background
  activity.
- **Sleep replay after normal learning** (46). Tadros et al. 2022: the offline phase only
  restores tasks the network already learned awake.
- **Delays with timing-based wiring** (76). Izhikevich 2006: neither alone gives
  polychronous groups. Our delays did nothing on Echo (finding 7) because nothing was wiring
  by timing.
- **Competition with the trace rule** (120). Földiák 1991 uses competition between output
  cells; without it every cell learns the same invariance.
- **Burst-dependent plasticity with short-term plasticity and apical dendrites** (30).
  Payeur et al. 2021 multiplex event rate and burst rate using short-term depression and
  facilitation on different pathways; as read here, without them the feedback signal and
  the forward signal mix.
- **Opponent actors with a nonlinear update** (35). Collins and Frank 2014: two pathways,
  one for good outcomes and one for bad, only specialize because the update is Hebbian
  (activity times error), not linear.

- **A negative image needs a copy of the motor command** (167). Bastian in Arbib 2003: the
  anti-Hebbian rule cancels only what its parallel-fiber inputs can predict; without
  corollary discharge and posture inputs there is nothing to cancel self-motion with.
- **Small branches with a structural rule** (169). Mel in Arbib 2003: the capacity gain comes
  from separately thresholded branches *and* the freedom to place co-active inputs on the
  same branch; Mel frames the extra capacity as available to a structural learning rule,
  not to weight changes on a fixed random placement.

## What this sharpens or contradicts in `docs/findings.md`

1. **Finding 17 ("do not strengthen your own answer on good") is too strong.** Baldassi,
   Braunstein, Brunel and Zecchina 2007 (entry 15, read in full) learn with binary synapses
   and hidden integer states. On an error, every synapse moves. On a correct answer with a
   comfortable margin, nothing moves. On a correct answer that was *barely* correct, only the
   synapses that pushed the right way move, and only in their hidden state (the visible bit
   does not change). Without that third case learning time grows exponentially with size;
   with it, capacity rises from about 0.3 to about 0.65 patterns per synapse. So the lock
   in our runs comes from strengthening the visible answer on every good, and the fix the
   literature suggests is not "never on good" but "on good, deepen hidden evidence only when
   the vote was close". Brader, Senn and Fusi 2007 (entry 16) build the same thing into
   hardware as a "stop-learning" condition. https://arxiv.org/abs/0707.1295
2. **Finding 14 (evidence before commitment) has prior art, and a rule for the counter
   width.** HTM's "permanence" (Hawkins and Ahmad 2016, entry 83) is a hidden scalar per
   potential synapse with a threshold for being connected: our evidence counter. In binary
   networks the hidden weight has been reread as inertia (Helwegen et al. 2019) and as a
   metaplastic consolidation variable (Laborieux et al. 2021, entry 14). Baldassi et al.
   report an optimal number of hidden states that is small for sparse coding and grows as
   roughly the square root of the number of inputs when coding is dense; Bill and Legenstein
   2014 find four bistable parts per synapse enough. Our "6 bits best, 4 close, 8 worse"
   fits that.
3. **Finding 15 (the output must know which action was taken) generalizes inward.** The
   attention-gated family (Roelfsema's AGREL, AuGMEnT, BrainProp; entries 98 and 99) uses
   exactly that bit, sent back from the chosen action to earlier layers, to tag the synapses
   that fed it; BrainProp is proven equal to backprop for one output at a time and matches
   it on CIFAR-100 at 1 to 3.5 times the training time. That is the most direct route from
   what works at our readout to the inside wall.
4. **Finding 12 (an experiment must be the size of a decision) is supported, with a caveat.**
   Node perturbation is unstable and scales poorly with network size (Hiratani et al. 2022,
   entry 104); songbirds inject variability into the premotor pathway (LMAN), not into
   single cells (Fiete, Fee and Seung 2007, entry 80). The caveat: Züge, Klos and
   Memmesheimer find weight perturbation equal or better than node perturbation when tasks
   unfold over time and the dynamics are low-dimensional (preprint, entry 105), so perturbing
   connections, not neurons, deserves a test before the idea is closed.
5. **Finding 13 (a rule graded against its own average must count everything it averaged
   over) has a stronger form.** Reward-modulated STDP learns only if the reward is centered
   per stimulus, which needs a critic, not a single running average (Frémaux, Sprekeler and
   Gerstner 2010 (m), entry 107). Our global running average is the weak form; ideas 4 and
   5 (reward minus expectation, a critic per sector) are the strong one.
6. **Finding 1 (a neuron must pay for what it keeps) is a known principle with a known
   companion.** Heterosynaptic plasticity (entry 19) is the biological budget, and Zenke and
   Gerstner 2017 argue it must be *fast*. Litwin-Kumar et al. 2017 (entry 57) add a
   sharpening: few inputs per neuron are best for a fixed random expansion, but many inputs
   are better where the synapses are trained by error. That fits finding 16 (many motor
   cells) and suggests the readout's budget should be larger than the sheet's.
7. **Finding 4 (co-activity wiring must stop when idle) matches the acetylcholine story,
   but biology does not idle by stopping.** Cholinergic tone switches cortex between encoding
   and retrieval (Hasselmo 2006, entry 37); offline, sleep renormalizes strength downward
   (Tononi and Cirelli 2014, entry 45) and replays (entries 46 to 48). The project's gate on
   input is the encoding switch; the sleep half is missing. Unchecked net strengthening is
   exactly what the synaptic homeostasis hypothesis says sleep exists to undo.
8. **Findings 8 and the headline (rich sheet, learning at the readout) match two old
   biological designs.** A fixed random expansion with learning only at the output is the
   mushroom body (entry 56) and the Marr-Albus cerebellum (entry 58), and in machine learning
   the reservoir and the extreme learning machine (entries 121, 124). Litwin-Kumar et al.
   compute that cerebellar and fly expansions sit at the in-degree that maximizes
   dimension. This is support, and also a warning: both biological versions need something
   else (a hippocampus, a cortex) for the inside wall.
9. **The time wall: a known split between short and long holds.** Masse et al. 2019
   (entry 82) find short-term synaptic plasticity can hold information silently only for
   short delays, and that manipulating held information needs persistent firing. Every
   biological hold longer than a second in this catalogue uses a slow cellular or synaptic
   variable (facilitation of about a second, entry 21; a calcium-activated current, entry
   81; adaptation, entry 145) or a dedicated attractor region (67), not a generic recurrent
   sheet. Our per-neuron rate regulation and leak work against any of these; a held cell
   must be exempt from them.
10. **Finding 21 and the delays: polychronization says why they measured as nothing.** Delays
    store timed patterns only together with wiring by spike timing (Izhikevich 2006, entry
    76). Our growth rule wires by "recently active", not by "active exactly one delay
    before", which is idea 3 (causal credit) and idea 81 (wire by timing).
11. **The new-place wall: the trace rule needs motion.** Li and DiCarlo 2008 (entry 120)
    show position tolerance in monkey IT is learned from temporal contiguity, the same
    object seen at different places in quick succession. A battery where shapes jump
    between places, or appear once, gives the trace rule nothing; the invariance battery
    should include continuous drift (it plans to). Wiskott and Sejnowski 2002 (entry 119)
    add that learning several invariances at once is harder than one at a time.

---

## A. Synapse and neuron rules

**1. Pair STDP.** A connection strengthens when the source fires just before the target and
weakens when just after, with a window of tens of milliseconds.
Source: Bi and Poo 1998, "Synaptic modifications in cultured hippocampal neurons: dependence
on spike timing, synaptic strength, and postsynaptic cell type", J Neurosci (m).
https://doi.org/10.1523/JNEUROSCI.18-24-10464.1998
Aim: inside (unsupervised), gap (order). Here: nothing new stored; on a target spike, sources
in the map exactly `delay` ticks back gain, sources in the map one tick after lose; `ring[t-d]
& ~ring[t-d+1]` per slot. Cost: fits (only on spikes). Ideas: 81, 3. Matters: medium; it is
the timing half of entries 75 and 76, and growth today ignores order.

**2. Triplet STDP.** Change depends on triples of spikes, so the rule sees firing rate as
well as timing; with random spikes it reduces to BCM.
Source: Pfister and Gerstner 2006, "Triplets of spikes in a model of spike timing-dependent
plasticity", J Neurosci 26:9673. https://doi.org/10.1523/JNEUROSCI.1425-06.2006
Aim: inside. Here: a second "fired twice lately" bit per neuron (OR of two maps ANDed with a
third); strengthen only if the target fired twice. Cost: fits. Ideas: new (close to 29).
Matters: low; its value here is that "fired twice" is a cheap rate signal for 30.

**3. Voltage-based STDP.** Change depends on presynaptic spikes and the target's membrane
voltage filtered at two time constants, plus homeostasis; reproduces many STDP data and
makes connectivity reflect the code (loops for rate codes, one-way links for timing codes).
Source: Clopath, Büsing, Vasilaki and Gerstner 2010, "Connectivity reflects coding: a model
of voltage-based STDP with homeostasis", Nat Neurosci 13:344. https://doi.org/10.1038/nn.2479
Aim: inside. Here: use the int16 potential as the voltage; on a source spike, gain if the
target's potential is above a high mark, lose if between a low and high mark. Cost: awkward
(reads potential per source spike). Ideas: new. Matters: low; the subthreshold variant of
16 and 29 is cheaper.

**4. Calcium thresholds.** One calcium variable per synapse; above one threshold the synapse
weakens, above a higher one it strengthens. Many STDP curves fall out of the parameters.
Source: Graupner and Brunel 2012, "Calcium-based plasticity model explains sensitivity of
synaptic changes to spike pattern, rate, and dendritic location", PNAS 109:3991.
https://doi.org/10.1073/pnas.1109359109
Aim: inside. Here: per-neuron small counter of recent input coincidences (sum of popcounts of
active-and-connected over a few ticks), two thresholds decide sign for all just-active
sources. Cost: awkward (per-neuron counter each tick; per-synapse version does not fit).
Ideas: new. Matters: low; a family generator more than a mechanism.

**5. Competitive STDP with bounds.** STDP with hard bounds makes synapses compete: the
neuron's inputs split into strong and zero, and the output rate stabilizes.
Source: Song, Miller and Abbott 2000, "Competitive Hebbian learning through spike-timing-
dependent synaptic plasticity", Nat Neurosci 3:919 (m). https://doi.org/10.1038/78829
Aim: alive, inside. Here: what the 2-bit weights already do when depression is slightly
stronger than potentiation. Cost: fits. Ideas: 2 (budget is the explicit form). Matters:
low; the budget already supplies the competition.

**6. BCM sliding threshold.** Below a threshold that tracks the neuron's recent average
activity, active inputs weaken; above it, they strengthen. Stable and selective.
Source: Bienenstock, Cooper and Munro 1982, "Theory for the development of neuron
selectivity", J Neurosci 2:32. https://doi.org/10.1523/JNEUROSCI.02-01-00032.1982
Aim: inside, alive. Here: per-neuron slow rate estimate already exists implicitly in the
threshold regulator; when a neuron fires and its drive was below its own average drive, weaken
its active inputs, above it strengthen. Cost: fits (one int per neuron, on spikes). Ideas:
new as a learning rule (the regulator is its homeostatic half). Matters: medium; it is the
standard way to get selective features with no reward.

**7. Oja normalization.** Hebbian growth with a decay proportional to output squared keeps
the weight vector bounded and finds the principal component.
Source: Oja 1982, "Simplified neuron model as a principal component analyzer", J Math Biol
15:267 (m). https://doi.org/10.1007/BF00275687
Aim: alive. Here: the budget is a discrete Oja. Cost: fits. Ideas: 2. Matters: low; done.

**8. Three-factor rule with an eligibility trace.** Co-activity sets a decaying flag on the
synapse; a later modulator (reward, surprise, novelty) turns flags into change. Experiments
since 2014 show such traces lasting about a second; in striatum, dopamine works only 0.3 to
2 s after the input, not before.
Sources: Gerstner, Lehmann, Liakoni, Corneil and Brea 2018, "Eligibility traces and
plasticity on behavioral time scales", Front Neural Circuits 12:53,
https://doi.org/10.3389/fncir.2018.00053 ; Izhikevich 2007, "Solving the distal reward problem
through linkage of STDP and dopamine signaling", Cereb Cortex 17:2443,
https://doi.org/10.1093/cercor/bhl152 ; Yagishita et al. 2014, "A critical time window for
dopamine actions on the structural plasticity of dendritic spines", Science 345:1616,
https://doi.org/10.1126/science.1255514
Aim: gap (credit), acting. Here: today "recent" is 16 ticks. A trace needs a per-neuron
countdown (4 bits) or a second slow OR plane cleared on a schedule; touched each tick at low
cost, read on reward. Cost: fits as per-neuron slow bits; awkward per connection. Ideas: 52.
Matters: high for cartpole (reward every 25 steps) and delayed echo.

**9. Separate traces for strengthening and weakening.** In cortex, pairing leaves two
traces: one that noradrenaline turns into strengthening and one that serotonin turns into
weakening, with different lifetimes; a model with both learned reward timing.
Source: He, Huertas, Hong et al. 2015, "Distinct eligibility traces for LTP and LTD in
cortical synapses", Neuron 88:528. https://doi.org/10.1016/j.neuron.2015.09.037
Aim: inside, gap, acting. Here: two per-neuron trace bits (or countdowns) with different
decay chances; good converts the "up" trace, bad the "down" trace. Cost: fits. Ideas: new.
Matters: medium; cheap, and lets good and bad reach back different distances.

**10. Synaptic tagging and capture.** A weak event leaves a tag that fades within an hour; a
strong event anywhere on the same neuron makes a shared resource that turns all tags on it
into lasting change.
Source: Frey and Morris 1997, "Synaptic tagging and long-term potentiation", Nature 385:533.
https://doi.org/10.1038/385533a0
Aim: keeping, gap. Here: tag bit per connection, or per slot; a neuron-wide "capture" event
(a big reward, a burst) converts all its tags. Cost: awkward per connection, fits per slot.
Ideas: new (34 and 35 are near). Matters: medium; it gives a principled "make it permanent".

**11. Behavioral-timescale plasticity (BTSP).** A rare dendritic plateau in a hippocampal
cell potentiates every input active within several seconds before or after, in one trial;
place fields form this way. Not Hebbian: the inputs need not have fired the cell.
Source: Bittner, Milstein, Grienberger, Romani and Magee 2017, "Behavioral time scale
synaptic plasticity underlies CA1 place fields", Science 357:1033.
https://doi.org/10.1126/science.aan3846 ; follow-up Milstein et al. 2021, eLife 10:e73046.
Aim: inside, gap, capacity (one-shot). Here: plateau = a per-neuron event drawn by chance
when strong feedback (from motor cells or a "good" line) meets local drive; on a plateau,
the neuron's evidence for every source active in the whole ring (16 maps) moves up. Cost:
fits (rare, per neuron). Ideas: new. Matters: medium to high; one-shot wiring from a
seconds-wide window is exactly the kind of credit the inside lacks.

**12. Cascade synapse.** Each visible state of a binary synapse has hidden depths; deeper
depths flip with geometrically smaller chance. Storage stays high and forgetting becomes a
power law.
Source: Fusi, Drew and Abbott 2005, "Cascade models of synaptically stored memories",
Neuron 45:599. https://doi.org/10.1016/j.neuron.2005.02.001
Aim: keeping, capacity. Here: 2 or 3 hidden bits per connection; on an attempted flip, AND
with a sparse random word whose density halves per depth. Cost: awkward (per-connection
depth, touched rarely). Ideas: 7, 35. Matters: medium; capacity past eight cues is a wall.

**13. Coupled multi-timescale synapse.** A chain of variables per synapse, each slower than
the last, passing change along; the coupling raises capacity well past cascades and explains
spacing effects.
Source: Benna and Fusi 2016, "Computational principles of synaptic memory consolidation",
Nat Neurosci 19:1697. https://doi.org/10.1038/nn.4401
Aim: keeping, capacity. Here: two counters per connection (fast, slow) with chance transfer
toward each other. Cost: awkward. Ideas: 7. Matters: medium.

**14. The hidden weight as consolidation (metaplasticity).** Plasticity depends on the
synapse's history. In binary networks the hidden real weight behind each binary weight can
be read as inertia (how hard the bit is to flip) and used deliberately: the further from
the flip point, the smaller further steps away from it, so old tasks are protected.
Sources: Abraham and Bear 1996, "Metaplasticity: the plasticity of synaptic plasticity",
Trends Neurosci 19:126 (m), https://doi.org/10.1016/S0166-2236(96)80018-X ; Helwegen et al.
2019, "Latent weights do not exist: rethinking binarized neural network optimization",
NeurIPS, https://arxiv.org/abs/1906.02107 ; Laborieux, Ernoult, Hirtzlin and Querlioz 2021,
"Synaptic metaplasticity in binarized neural networks", Nat Commun 12:2549,
https://doi.org/10.1038/s41467-021-22768-y
Aim: keeping. Here: on our evidence counters, steps toward zero (toward unwiring) succeed
with a chance that falls with distance from the wiring threshold. Cost: fits (counters exist
at the readout; a random gate per step). Ideas: new on top of 59. Matters: high for tier 8,
cheap.

**15. Binary synapses with hidden states and a "barely correct" rule.** Clipped perceptron:
each synapse has a hidden integer and a visible sign. Error: every hidden value steps toward
the right answer. Correct with margin: nothing. Correct by the thinnest margin: only the
synapses that helped step deeper, hidden only. First online rule to reach a finite capacity
per synapse with binary synapses (about 0.65 to 0.7 patterns per synapse).
Source: Baldassi, Braunstein, Brunel and Zecchina 2007, "Efficient supervised learning in
networks with binary synapses", PNAS 104:11079. https://arxiv.org/abs/0707.1295 (read in full)
Aim: capacity, inside, keeping. Here: at the motor cells, the margin is the vote gap between
the pressed button and the runner-up; on good with a gap below a small mark, add one to the
evidence of the pressed group's active, connected sources. Applied inward with 98 or 89.
Cost: fits (counters exist; touched on reward when close). Ideas: new; corrects finding 17.
Matters: high; it is our rule with the missing case filled in by theory.

**16. Bistable stochastic synapses with stop-learning.** Binary synapses drift to one of two
states between events; changes happen by chance and only while the neuron's total input is
not already on the right side ("stop-learning" via a calcium variable). Built in VLSI.
Sources: Amit and Fusi 1994, "Learning in neural networks with material synapses", Neural
Comput 6:957 (m), https://doi.org/10.1162/neco.1994.6.5.957 ; Brader, Senn and Fusi 2007,
"Learning real-world stimuli in a neural network with spike-driven synaptic dynamics",
Neural Comput 19:2881, https://doi.org/10.1162/neco.2007.19.11.2881
Aim: capacity, keeping. Here: our chance flips plus a gate: skip learning for a motor cell
whose potential is already far past threshold in the right direction. Cost: fits. Ideas: 7
partly; the gate is new. Matters: medium to high; same lesson as 15, from hardware.

**17. Synaptic scaling.** A neuron scales all its input strengths up when it has been too
quiet and down when too busy, over hours to days.
Source: Turrigiano, Leslie, Desai, Rutherford and Nelson 1998, "Activity-dependent scaling
of quantal amplitude in neocortical neurons", Nature 391:892. https://doi.org/10.1038/36103
Aim: alive. Here: what per-neuron threshold regulation already does (threshold, not weights).
Cost: fits. Ideas: design mechanism 15. Matters: low; present in another form.

**18. Intrinsic plasticity.** A neuron tunes its own excitability so its output distribution
is sparse (exponential); combined with Hebb it finds heavy-tailed, sparse directions, and it
improves reservoirs.
Sources: Triesch 2007, "Synergies between intrinsic and synaptic plasticity mechanisms",
Neural Comput 19:885, https://doi.org/10.1162/neco.2007.19.4.885 ; Desai, Rutherford and
Turrigiano 1999, Nat Neurosci 2:515 (m), https://doi.org/10.1038/9165 ; Schrauwen et al. 2008,
"Improving reservoirs using intrinsic plasticity", Neurocomputing 71:1159 (m),
https://doi.org/10.1016/j.neucom.2007.12.020
Aim: alive, inside. Here: our threshold regulator targets a mean; intrinsic plasticity
targets a shape: rare high bursts and many silences. Add a second chance rule that raises the
threshold more steeply when a neuron fires on consecutive ticks. Cost: fits. Ideas: design
mechanism 15 (mean only). Matters: medium; sparse firing is what makes features readable.

**19. Heterosynaptic plasticity and fast compensation.** When some inputs strengthen, others
on the same neuron weaken, without having been active; this must act fast to keep Hebbian
learning stable, and slow homeostasis cannot do it alone.
Sources: Chistiakova, Bannon, Bazhenov and Volgushev 2014, "Heterosynaptic plasticity:
multiple mechanisms and multiple roles", Neuroscientist 20:483,
https://doi.org/10.1177/1073858414529829 ; Zenke and Gerstner 2017, "Hebbian plasticity
requires compensatory processes on multiple timescales", Phil Trans R Soc B 372:20160259,
https://doi.org/10.1098/rstb.2016.0259
Aim: alive, room. Here: the budget, applied at the moment of strengthening (same tick).
Cost: fits. Ideas: 2. Matters: high as confirmation; it is finding 1.

**20. Short-term depression.** A synapse that just transmitted transmits less for a while;
how fast it depresses decides whether the target reads rate or timing.
Source: Tsodyks and Markram 1997, "The neural code between neocortical pyramidal neurons
depends on neurotransmitter release probability", PNAS 94:719.
https://doi.org/10.1073/pnas.94.2.719
Aim: invariance (rate versus onset), alive. Here: per-source "tired" bit word per tile, set
on spike, cleared with chance each tick; popcount uses `s & ~tired` or counts tired sources
at weight 1. Cost: fits (one word per tile, per tick). Ideas: 66. Matters: medium; makes
cells respond to onsets, which helps 99's on/off cells and loudness invariance.

**21. Short-term facilitation as working memory.** Recurrent synapses that facilitate for
about a second hold an item with no persistent firing; brief population spikes refresh it.
Source: Mongillo, Barak and Tsodyks 2008, "Synaptic theory of working memory", Science
319:1543. https://doi.org/10.1126/science.1150769
Aim: gap. Here: per-source "facilitated" word per tile, set on spike, cleared with chance
2^-k per tick (k gives hundreds of ticks); popcount adds `popcount(e & fac & w0)` again.
Cost: fits (one word per tile and one extra popcount per slot). Ideas: 67. Matters: high;
the cheapest silent hold.

**22. Structural plasticity: wiring as storage.** Synapses form and vanish continually;
which pairs are connected can store more than strengths alone in a sparse network, at the
cost of slower learning.
Sources: Chklovskii, Mel and Svoboda 2004, "Cortical rewiring and information storage",
Nature 431:782, https://doi.org/10.1038/nature03012 ; Holtmaat and Svoboda 2009,
"Experience-dependent structural synaptic plasticity in the mammalian brain", Nat Rev
Neurosci 10:647 (m), https://doi.org/10.1038/nrn2699 ; Poirazi and Mel 2001, "Impact of
active dendrites and structural plasticity on the memory capacity of neural tissue", Neuron
29:779 (m), https://doi.org/10.1016/S0896-6273(01)00252-5
Aim: capacity. Here: growth and pruning already exist; the point is to treat which slot a
connection sits in (which branch) as information. Cost: fits. Ideas: 19, 20 (growth,
pruning), 24. Matters: medium.

**23. Synaptic sampling (noisy rewiring under a prior).** Weights and wiring wander under
noise and a prior while plasticity pulls them; the network samples good configurations
instead of settling, and copes with damage. The same idea with a hard connection budget
trains very sparse deep nets (Deep R).
Sources: Kappel, Habenschuss, Legenstein and Maass 2015, "Network plasticity as Bayesian
inference", PLoS Comput Biol 11:e1004485, https://doi.org/10.1371/journal.pcbi.1004485 ;
Bellec, Kappel, Maass and Legenstein 2018, "Deep rewiring", ICLR,
https://arxiv.org/abs/1711.05136
Aim: keeping, inside, alive. Here: a small chance per tick that a weak connection drops and a
random candidate in the same slot grows, at a fixed count. Cost: fits. Ideas: 23 to 25
partly; the "sample, do not settle" framing is new. Matters: medium.

**24. Silent synapses.** Many synapses exist but pass nothing until coincident activity
unsilences them; a reserve of potential connections.
Sources: Isaac, Nicoll and Malenka 1995, "Evidence for silent synapses", Neuron 15:427 (m),
https://doi.org/10.1016/0896-6273(95)90046-2 ; Kerchner and Nicoll 2008, "Silent synapses
and the emergence of a postsynaptic mechanism for LTP", Nat Rev Neurosci 9:813 (m),
https://doi.org/10.1038/nrn2501
Aim: capacity. Here: an evidence counter below the wiring threshold is a silent synapse.
Cost: fits. Ideas: 59. Matters: low; already built.

**25. Inhibitory plasticity.** Inhibitory synapses onto a cell strengthen when the cell fires
above a target rate and weaken below, balancing excitation input by input; stored patterns
end up silent but recallable.
Source: Vogels, Sprekeler, Zenke, Clopath and Gerstner 2011, "Inhibitory plasticity balances
excitation and inhibition in sensory pathways and memory networks", Science 334:1569.
https://doi.org/10.1126/science.1211095
Aim: alive, capacity. Here: for inhibitory sources only: on a target spike, active inhibitory
sources gain; with a small chance per tick, active ones lose. Cost: fits. Ideas: new
(inhibition is currently fixed by type, its weights learn like excitation). Matters: medium;
balance per input is finer than our per-neuron threshold.

**26. Disinhibition gates learning.** During fear learning, acetylcholine drives layer-1
interneurons that inhibit PV cells, which releases pyramidal cells for a moment; blocking
this blocks learning.
Source: Letzkus et al. 2011, "A disinhibitory microcircuit for associative fear learning in
the auditory cortex", Nature 480:331. https://doi.org/10.1038/nature10674
Aim: inside. Here: on good or bad, drop inhibition gain in the sectors that were active for
a few ticks, so their cells fire more and their learning chances rise. Cost: fits (per
sector). Ideas: 18, 85. Matters: medium.

**27. Branch-specific plasticity.** Each dendritic branch can strengthen as a unit, so
different branches learn different input combinations and one neuron joins several
ensembles.
Sources: Legenstein and Maass 2011, "Branch-specific plasticity enables self-organization of
nonlinear computation in single neurons", J Neurosci 31:10787,
https://doi.org/10.1523/JNEUROSCI.5684-10.2011 ; Losonczy, Makara and Magee 2008,
"Compartmentalized dendritic plasticity and input feature storage in neurons", Nature 452:436
(m), https://doi.org/10.1038/nature06725
Aim: capacity, inside. Here: a slot is a branch; a per-slot gain bit doubles that slot's
branch bonus; set when the branch crossed its threshold just before good. Cost: fits (one bit
per slot). Ideas: 72 (any-branch), branch bonus. Matters: medium; finding 20 says the branch
bonus carries exclusive-or.

**28. Non-monotonic dendritic spikes.** Human layer 2/3 dendrites make calcium spikes that are
largest at threshold and shrink with stronger input, so one branch computes exclusive-or.
Source: Gidon et al. 2020, "Dendritic action potentials and computation in human layer 2/3
cortical neurons", Science 367:83. https://doi.org/10.1126/science.aax6239
Aim: inside (combination). Here: branch contributes bonus only if excitation is in a band
[a, b]: two compares on the popcount. Cost: fits. Ideas: new (73 veto is near). Matters:
medium; parity and exclusive-or in one cell.

**29. Dendritic prediction of somatic spiking.** A neuron's dendrite learns to predict its own
firing; the mismatch is the learning signal. Covers unsupervised, supervised and reinforcement
cases with one rule.
Source: Urbanczik and Senn 2014, "Learning by the dendritic prediction of somatic spiking",
Neuron 81:521. https://doi.org/10.1016/j.neuron.2013.11.030
Aim: inside. Here: split slots into "basal" (local) and "apical" (far, or a feedback slot);
each tick, `predicted = apical drive >= mark`, `actual = spike`; on mismatch, move the apical
connections of active sources toward actual. Cost: fits (only on mismatch, like sensor
prediction today). Ideas: 11, design mechanism 16. Matters: medium to high; it gives every
inner neuron a local target, and a teacher can be fed in through the apical slot.

**30. Burst-dependent plasticity (burstprop).** Top-down feedback changes a neuron's chance of
bursting; a burst means "strengthen", a single spike "weaken". With short-term plasticity
separating burst rate from event rate, higher layers steer lower ones' learning.
Source: Payeur, Guerguiev, Zenke, Richards and Naud 2021, "Burst-dependent synaptic plasticity
can coordinate learning in hierarchical circuits", Nat Neurosci 24:1010.
https://doi.org/10.1038/s41593-021-00970-x
Aim: inside. Here: a burst is two spikes in two ticks (cooldown permitting); feedback from the
pressed button's cells lowers the threshold of their sources for a tick on good (more
bursts). Learning: burst means active sources gain, single spike means lose a little. Cost:
fits. Ideas: 29. Matters: medium to high; a second symbol is the cheapest channel for a
teaching signal.

**31. Apical coincidence (BAC firing).** A back-propagating spike plus input to the distal
tuft at the same time triggers a burst; the cell signals "my input matches the context".
Source: Larkum, Zhu and Sakmann 1999, "A new cellular mechanism for coupling inputs arriving at
different cortical layers", Nature 398:338 (m). https://doi.org/10.1038/18686
Aim: inside, capacity (context). Here: a neuron bursts only when local and far slots both
pass their marks in one tick. Cost: fits. Ideas: 28. Matters: medium.

**32. Hedonistic synapses.** Transmission is random; reward raises the release chance of
synapses that released before a spike that preceded reward, and lowers it for those that
failed. A stochastic gradient of average reward.
Source: Seung 2003, "Learning in spiking neural networks by reinforcement of stochastic
synaptic transmission", Neuron 40:1063. https://doi.org/10.1016/S0896-6273(03)00761-X
Aim: inside. Here: AND each slot's source word with a sparse random mask (release) and keep
the release mask as the eligibility. Cost: awkward (a random word per slot per tick). Ideas:
1 (noise at the neuron); this is noise at the synapse. Matters: low; finding 12 says
per-unit noise is too rare to matter.

## B. Neuromodulators and global signals

**33. Dopamine as reward prediction error.** Dopamine cells fire for reward better than
expected, pause for worse, and move their response to the earliest predicting cue.
Source: Schultz, Dayan and Montague 1997, "A neural substrate of prediction and reward",
Science 275:1593. https://doi.org/10.1126/science.275.5306.1593
Aim: acting, gap (credit to earlier cues). Here: idea 90's value cell; the broadcast is
"good minus expected" from a group of cells that learn to predict good. Cost: fits (a group
of neurons and the existing broadcast). Ideas: 4, 90. Matters: high for acting; cartpole
needs a signal more frequent than its reward.

**34. Different rules in different compartments.** In the fly mushroom body each dopamine
cell type writes memory with its own timing rule; the same cell can write or erase depending
on when it fires relative to the odor. Parallel memories with different rules.
Source: Aso and Rubin 2016, "Dopaminergic neurons write and update memories with
cell-type-specific rules", eLife 5:e16135. https://doi.org/10.7554/eLife.16135
Aim: keeping, capacity. Here: per sector, a rule choice in the genome (timing sign, trace
length, learning chance). Cost: fits. Ideas: 112. Matters: medium; regions with their own
rules have a clear biological precedent.

**35. Opponent actors (go and no-go).** Two striatal populations learn action values from
positive and negative errors respectively, with a Hebbian (activity times error) update; they
specialize, and dopamine level sets which one drives choice.
Source: Collins and Frank 2014, "Opponent actor learning (OpAL)", Psychol Rev 121:337.
https://doi.org/10.1037/a0037015
Aim: acting. Here: two motor groups per button (go, no-go); good trains go, bad trains no-go;
the press is go minus no-go. Cost: fits. Ideas: new. Matters: medium; a clean way for bad to
be heard without fighting good, aimed at locking.

**36. Distributional dopamine.** Different dopamine cells are optimistic or pessimistic
(different gains for positive and negative errors), so the population codes the whole
distribution of rewards.
Source: Dabney et al. 2020, "A distributional code for value in dopamine-based reinforcement
learning", Nature 577:671 (m). https://doi.org/10.1038/s41586-019-1924-6
Aim: acting. Here: value cells with different good/bad chance ratios. Cost: fits. Ideas: new.
Matters: low; refinement once 33 works.

**37. Acetylcholine switches encoding and retrieval.** High acetylcholine strengthens
feed-forward input relative to recurrent feedback and favors writing new memories; low
acetylcholine favors recall through recurrent links.
Source: Hasselmo 2006, "The role of acetylcholine in learning and memory", Curr Opin Neurobiol
16:710. https://doi.org/10.1016/j.conb.2006.09.002
Aim: keeping, gap. Here: one global bit: "encode" raises the gain of sensor slots and enables
co-activity wiring; "hold or recall" lowers it, disables wiring, and lowers the leak of
memory cells. Cost: fits. Ideas: 56 (the gate on input is a crude form), 84. Matters: medium.

**38. Expected and unexpected uncertainty.** Acetylcholine tracks known unreliability of cues
within a context; noradrenaline tracks surprise that says the context changed.
Source: Yu and Dayan 2005, "Uncertainty, neuromodulation, and attention", Neuron 46:681.
https://doi.org/10.1016/j.neuron.2005.04.026
Aim: keeping, acting. Here: two global counters: running error rate (expected) and a jump
detector on it (unexpected); a jump raises learning chances and noise; steady error lowers
reliance on the cue. Cost: fits. Ideas: 84, 85. Matters: medium; it is what to do when a rule
changes (tier 8).

**39. Noradrenaline gain: exploit or explore.** Phasic locus coeruleus bursts after
task-relevant outcomes sharpen the current task; high tonic firing when utility falls drives
disengagement and search.
Source: Aston-Jones and Cohen 2005, "An integrative theory of locus coeruleus-norepinephrine
function: adaptive gain and optimal performance", Annu Rev Neurosci 28:403.
https://doi.org/10.1146/annurev.neuro.28.061604.135709
Aim: acting. Here: a slow average of good; when it falls, raise noise on motor cells and lower
thresholds (explore); when it is high, lower noise (exploit). Cost: fits. Ideas: 85, 36.
Matters: medium; aimed at the lock.

**40. Serotonin and patience.** Activating dorsal raphe serotonin cells makes mice wait longer
for delayed rewards; Doya's mapping assigns serotonin the time horizon of reward prediction.
Sources: Miyazaki, Miyazaki and Doya 2014, "Optogenetic activation of dorsal raphe serotonin
neurons enhances patience for future rewards", Curr Biol 24:2033,
https://doi.org/10.1016/j.cub.2014.07.041 ; Doya 2002 (entry 41).
Aim: gap, acting. Here: a global value that sets the trace length of entry 8 (longer when
rewards have been late). Cost: fits. Ideas: new. Matters: low; one knob.

**41. Modulators as metaparameters.** Dopamine as the reward error, serotonin as the discount
horizon, noradrenaline as exploration noise, acetylcholine as learning speed; learning these
settings online is "metalearning".
Source: Doya 2002, "Metalearning and neuromodulation", Neural Networks 15:495.
https://doi.org/10.1016/S0893-6080(02)00044-8
Aim: all. Here: four global shift values moved slowly by simple rules (raise noise when good
stalls). Cost: fits. Ideas: 85, 88. Matters: medium; a frame for the genome's chances.

**42. Glia count futility.** In zebrafish, noradrenergic cells respond to failed swim
attempts and radial astrocytes add these up; when the sum crosses a level the fish stops
trying.
Source: Mu et al. 2019, "Glia accumulate evidence that actions are futile and suppress
unsuccessful behavior", Cell 178:27. https://doi.org/10.1016/j.cell.2019.05.050
Aim: acting, alive. Here: a slow per-sector counter of "pressed and nothing good"; past a mark
it inhibits the motor group for a while or switches to the explore mode of 39. Cost: fits.
Ideas: 53 (lose-shift) is a fast cousin. Matters: low to medium.

**43. Astrocytes as many-neuron synapses.** A theory in which astrocytes couple many synapses
at once, giving dense associative memory with better storage per neuron.
Source: Kozachkov, Slotine and Krotov, "Neuron-astrocyte associative memory", arXiv 2023,
PNAS 2025. https://arxiv.org/abs/2311.08135
Aim: capacity. Here: a per-tile "astrocyte" word that ANDs several sources before they count.
Cost: awkward. Ideas: new. Matters: low; theory only so far.

**44. Critical periods set by inhibition.** Plasticity of visual cortex opens when PV basket
inhibition matures and closes later; drugs that boost inhibition open it early.
Source: Hensch 2005, "Critical period plasticity in local cortical circuits", Nat Rev Neurosci
6:877. https://doi.org/10.1038/nrn1787
Aim: keeping. Here: per-region learning chance that falls as the region's inhibition gain
rises with age. Cost: fits. Ideas: 88. Matters: low; at odds with "learns everywhere, all the
time" unless it reopens on surprise.

## C. Offline phases, keeping, growth

**45. Sleep downscales.** Waking learning raises net synaptic strength; slow-wave sleep scales
it back, saving energy and improving signal to noise. Contested in parts, but widely tested.
Source: Tononi and Cirelli 2014, "Sleep and the price of plasticity", Neuron 81:12.
https://doi.org/10.1016/j.neuron.2013.12.025
Aim: keeping, alive. Here: when unplugged, every connection below top strength drops a level
with a small chance; strong ones rarely. Cost: fits. Ideas: 34, design mechanism 21.
Matters: medium; the half of finding 4 we lack.

**46. Sleep-like replay with local Hebb.** After normal training, an offline phase of noisy
input and local Hebbian changes restores old tasks in artificial networks.
Source: Tadros, Krishnan, Ramyaa and Bazhenov 2022, "Sleep-like unsupervised replay reduces
catastrophic forgetting in artificial neural networks", Nat Commun 13:7742.
https://doi.org/10.1038/s41467-022-34938-7
Aim: keeping. Here: when unplugged, raise noise, run co-activity wiring with a slight
depression bias. Cost: fits. Ideas: 34, 87. Matters: medium.

**47. Replay, reverse replay, reward.** Hippocampal place cells replay paths during rest,
sometimes in reverse right after reward, and reward raises how often reverse replay happens.
Sources: Foster and Wilson 2006, "Reverse replay of behavioural sequences in hippocampal place
cells during the awake state", Nature 440:680 (m), https://doi.org/10.1038/nature04587 ;
Ambrose, Pfeiffer and Foster 2016, "Reverse replay of hippocampal place cells is uniquely
modulated by changing reward", Neuron 91:1124 (m), https://doi.org/10.1016/j.neuron.2016.07.047
Aim: gap (credit), acting. Here: idea 62: walk the ring backward on reward. Cost: fits (on
reward, a few map reads). Ideas: 62, 87. Matters: high; it is the biological case for 62.

**48. Prioritized replay.** Replay the experiences whose re-evaluation would most improve
choices; explains forward and reverse replay patterns; in RL, replay large-error transitions
first.
Sources: Mattar and Daw 2018, "Prioritized memory access explains planning and hippocampal
replay", Nat Neurosci 21:1609, https://doi.org/10.1038/s41593-018-0232-z ; Schaul et al. 2015,
"Prioritized experience replay" (m), https://arxiv.org/abs/1511.05952
Aim: keeping, acting. Here: keep a few saved stretches of the ring (16 maps each) taken at
bad outcomes and replay those first. Cost: awkward (a store of saved maps). Ideas: 87.
Matters: medium.

**49. Complementary learning systems.** A fast learner (hippocampus) stores episodes without
disturbing a slow learner (cortex) that extracts structure by interleaved replay.
Sources: McClelland, McNaughton and O'Reilly 1995, "Why there are complementary learning
systems in the hippocampus and neocortex", Psychol Rev 102:419,
https://doi.org/10.1037/0033-295X.102.3.419 ; Kumaran, Hassabis and McClelland 2016, "What
learning systems do intelligent agents need?", Trends Cogn Sci 20:512 (m),
https://doi.org/10.1016/j.tics.2016.05.004
Aim: keeping, capacity. Here: two readouts (idea 94) or two regions with different learning
chances; the fast one's stored stretches replayed into the slow one when idle. Cost: fits.
Ideas: 94, 34. Matters: medium.

**50. Wake-sleep.** Recognition weights learn from data in a wake phase, generative weights
learn from dreams in a sleep phase; each trains the other with local rules.
Source: Hinton, Dayan, Frey and Neal 1995, "The wake-sleep algorithm for unsupervised neural
networks", Science 268:1158. https://doi.org/10.1126/science.7761831
Aim: inside (no teacher). Here: feedback slots learn to reproduce lower activity in "wake";
in "sleep" the top generates and forward slots learn to recover it. Cost: awkward (two modes,
two directions of wiring). Ideas: new. Matters: low to medium; tier 10 (no teacher).

**51. Adult neurogenesis.** New dentate granule cells are added through life and are
thought to help separate similar memories, or to bind events close in time while young.
Whether adult humans make many new neurons is disputed (Sorrells et al. 2018 against;
Boldrini et al. 2018 for). In deep nets, adding units when a new class arrives helps.
Sources: Aimone, Deng and Gage 2011, "Resolving new memories", Neuron 70:589,
https://doi.org/10.1016/j.neuron.2011.05.010 ; Sorrells et al. 2018, Nature 555:377 (m),
https://doi.org/10.1038/nature25975 ; Boldrini et al. 2018, Cell Stem Cell 22:589 (m),
https://doi.org/10.1016/j.stem.2018.03.015 ; Draelos et al. 2017, "Neurogenesis deep
learning" (m), https://arxiv.org/abs/1612.03770
Aim: capacity, keeping. Here: rebirth of idle neurons (idea 25) with high learning chance
for a while. Cost: fits. Ideas: 25, 96. Matters: medium.

**52. Reset the least-used units.** Networks trained forever lose the ability to learn; giving
a tiny fraction of the least-used units fresh weights each step keeps plasticity
indefinitely.
Source: Dohare et al. 2024, "Loss of plasticity in deep continual learning", Nature 632:768.
https://doi.org/10.1038/s41586-024-07711-7
Aim: keeping (plasticity), alive. Here: track a per-neuron "used" count (how often it was a
counted source of a firing neuron); now and then rebirth the lowest. Cost: fits. Ideas: 25.
Matters: medium; finding 13 notes rules that freeze when every reward equals the average.

**53. Context-dependent gating.** For each task or context, a fixed random half (or more) of
hidden units is switched off; tasks then use mostly separate units. Alone it helps partly;
with synaptic stabilization, a network keeps 100 sequential tasks.
Source: Masse, Grant and Freedman 2018, "Alleviating catastrophic forgetting using
context-dependent gating and synaptic stabilization", PNAS 115:E10467.
https://doi.org/10.1073/pnas.1803839115
Aim: capacity, keeping. Here: a context line (or a hash of the cue set) selects a random mask
word per tile that is ANDed with spikes; combine with 14. Cost: fits (one AND per tile).
Ideas: new. Matters: high for tier 8 and capacity, very cheap.

**54. Importance-weighted consolidation (EWC, synaptic intelligence).** Each weight carries an
importance estimate; changes to important weights are penalized.
Sources: Kirkpatrick et al. 2017, "Overcoming catastrophic forgetting in neural networks",
PNAS 114:3521, https://doi.org/10.1073/pnas.1611835114 ; Zenke, Poole and Ganguli 2017,
"Continual learning through synaptic intelligence" (m), https://arxiv.org/abs/1703.04200
Aim: keeping. Here: an importance count per connection (how often it was credited on good);
flip chance falls with it. Cost: awkward (per connection); 14 is the cheap form. Ideas: 35.
Matters: low beyond 14.

**55. Spontaneous waves before input.** Before eyes open, retinal waves drive correlated
activity that wires maps and eye-specific layers.
Source: Meister, Wong, Baylor and Shatz 1991, "Synchronous bursts of action potentials in
ganglion cells of the developing mammalian retina", Science 252:939 (m).
https://doi.org/10.1126/science.2035024
Aim: invariance (topography), inside. Here: before tasks, sweep traveling waves across the
sensor lattice while co-activity wiring runs. Cost: fits. Ideas: 97. Matters: medium; also a
source of moving stimuli for the trace rule.

## D. Circuits that solve particular problems

**56. Mushroom body: expand, sparsen, learn at the output.** About 50 input channels project
randomly to about 2,000 Kenyon cells; feedback inhibition from one neuron (APL) keeps about 5%
active; dopamine changes only the Kenyon-to-output synapses. Breaking the inhibitory loop
makes codes denser and more correlated, and flies then fail to tell similar odors apart.
Sources: Dasgupta, Stevens and Navlakha 2017, "A neural algorithm for a fundamental computing
problem", Science 358:793, https://doi.org/10.1126/science.aam9868 ; Lin, Bygrave, de Calignon,
Lee and Miesenböck 2014, "Sparse, decorrelated odor coding in the mushroom body enhances
learned odor discrimination", Nat Neurosci 17:559, https://doi.org/10.1038/nn.3660
Aim: capacity, inside (readability). Here: per tile, an inhibitory feedback that keeps the
top k by potential (k-winners; a popcount-and-threshold search over potentials, or a
per-tile threshold nudged each tick toward k spikes). Cost: fits. Ideas: 75, 9. Matters:
high; it is our headline's architecture in an animal, with the sparsening we lack.

**57. Optimal in-degree for an expansion.** For a random expansion, dimension peaks at few
inputs per cell (cerebellar granule cells have 4, Kenyon cells about 7, matching the
optimum); when the input synapses are trained by error, dense wiring wins instead.
Source: Litwin-Kumar, Harris, Axel, Sompolinsky and Abbott 2017, "Optimal degrees of synaptic
connectivity", Neuron 93:1153. https://doi.org/10.1016/j.neuron.2017.01.030
Aim: capacity. Here: a genome number: connections per neuron in the sheet (small) versus at
the readout (large). Cost: fits. Ideas: 20, 98. Matters: medium; a guide for the richness
search.

**58. Cerebellum (Marr-Albus).** Mossy fibers expand into a huge number of granule cells;
each Purkinje cell reads them through parallel fibers; one climbing fiber per Purkinje cell
carries an error that depresses the parallel-fiber synapses active with it (Albus's sign
was confirmed; Marr's was not).
Sources: Marr 1969, "A theory of cerebellar cortex", J Physiol 202:437,
https://doi.org/10.1113/jphysiol.1969.sp008820 ; Albus 1971, "A theory of cerebellar function",
Math Biosci 10:25 (m), https://doi.org/10.1016/0025-5564(71)90051-4
Aim: inside (readout), acting. Here: our readout already is this: error depresses active
inputs of the wrong cells. Cost: fits. Ideas: 58. Matters: high as confirmation; motor
control (cartpole) is the cerebellum's job, with a dedicated error line per output.

**59. Cerebellar learning by perturbation.** Spontaneous climbing-fiber spikes perturb
movement and leave eligibility; a later climbing-fiber signal that reports the change in
error decides the sign. An average-cancelling error gives a gradient estimate.
Source: Bouvier et al. 2018, "Cerebellar learning using perturbations", eLife 7:e31599.
https://doi.org/10.7554/eLife.31599
Aim: acting, inside. Here: decision-sized perturbation (idea 60) with a per-output-group
eligibility, graded by change in error. Cost: fits. Ideas: 60. Matters: medium to high for
acting.

**60. Pattern separation and completion.** The dentate gyrus makes overlapping inputs
sparser and more distinct; CA3's recurrent collaterals complete partial cues to stored
patterns.
Source: Rolls 2013, "The mechanisms for pattern completion and pattern separation in the
hippocampus", Front Syst Neurosci 7:74 (m). https://doi.org/10.3389/fnsys.2013.00074
Aim: capacity. Here: a sparse expansion region (56) feeding a recurrent region with strong
co-activity wiring. Cost: fits. Ideas: 80, 112. Matters: medium.

**61. Phase coding: theta precession and theta-gamma.** Place cells fire at earlier phases of
theta as the animal crosses the field, compressing sequences into one cycle; several items
can be held as gamma subcycles within a theta cycle.
Sources: O'Keefe and Recce 1993, "Phase relationship between hippocampal place units and the
EEG theta rhythm", Hippocampus 3:317 (m), https://doi.org/10.1002/hipo.450030307 ; Lisman and
Jensen 2013, "The theta-gamma neural code", Neuron 77:1002 (m),
https://doi.org/10.1016/j.neuron.2013.03.007
Aim: gap, capacity (several items held at once). Here: a slow pulse (idea 86) with neurons
whose threshold is lowest at a preferred phase. Cost: fits. Ideas: 86. Matters: low to
medium; finding 2 says the pulse we had hurt.

**62. Time cells and a compressed timeline.** Hippocampal "time cells" fire in sequence
through a delay, with wider fields later. A bank of leaky integrators with time constants
spaced by ratio holds the Laplace transform of the past; cells reading differences of
neighbors fire in sequence with scale-invariant widths.
Sources: MacDonald, Lepage, Eden and Eichenbaum 2011, "Hippocampal 'time cells' bridge the gap
in memory for discontiguous events", Neuron 71:737 (m),
https://doi.org/10.1016/j.neuron.2011.07.012 ; Shankar and Howard 2012, "A scale-invariant
internal representation of time", Neural Comput 24:134 (m),
https://doi.org/10.1162/NECO_a_00212 ; Liu, Tiganj, Hasselmo and Howard 2019, "A neural
microcircuit model for a scalable scale-invariant representation of time", Hippocampus 29:260
(m), https://doi.org/10.1002/hipo.22994
Aim: gap, invariance (tempo). Here: a few cells per sensor line with leak shifts 1..8 (already
per-neuron); a second cell type that fires when cell k is up and cell k+1 is not. Cost: fits.
Ideas: 104, 69. Matters: high; one mechanism for both the gap and tempo.

**63. Basal ganglia action selection.** Channels compete through selective disinhibition: the
output nuclei hold all actions inhibited and release the one with the strongest striatal
support.
Source: Gurney, Prescott and Redgrave 2001, "A computational model of action selection in the
basal ganglia", Biol Cybern 84:401 (m). https://doi.org/10.1007/PL00007984
Aim: acting. Here: buttons as channels with a winner-take-all on the motor counts. Cost: fits.
Ideas: 75. Matters: low; the plug already picks a winner.

**64. Actor and critic.** Two neuron-like elements balanced a pole from failure signals alone:
the actor associates states with pushes; the critic learns to predict failure and gives the
actor a frequent internal reinforcement. The actor alone did not reach that performance.
Source: Barto, Sutton and Anderson 1983, "Neuronlike adaptive elements that can solve difficult
learning control problems", IEEE Trans SMC 13:834.
https://doi.org/10.1109/TSMC.1983.6313077
Aim: acting. Here: idea 90's value cell feeding the existing good/bad broadcast with its
change. Cost: fits. Ideas: 90, 5. Matters: high; it is cartpole's original solution, with
neuron-like parts and eligibility traces.

**65. Gating what enters working memory (PBWM).** Prefrontal stripes hold items; basal ganglia
learn by reward when to open each stripe's gate; this solves tasks where what to hold depends
on context (1-2-AX).
Source: O'Reilly and Frank 2006, "Making working memory work", Neural Comput 18:283.
https://doi.org/10.1162/089976606775093909
Aim: gap. Here: memory sectors (99 or 81) with a gate bit set by a small learned group;
reward trains the gate group like a button. Cost: fits. Ideas: new. Matters: high once any
hold exists; without a gate a hold is overwritten by the next input.

**66. Ring attractor.** The fly heading system is a ring with local excitation and global
inhibition that keeps a single bump; an artificially placed bump persists.
Source: Kim, Rouault, Druckmann and Jayaraman 2017, "Ring attractor dynamics in the Drosophila
central brain", Science 356:849. https://doi.org/10.1126/science.aal4835
Aim: gap, acting (heading). Here: a region whose genome sets local excitatory wiring dense
and inhibitory reach wide. Cost: fits. Ideas: 111, 112. Matters: medium.

**67. Bump attractor with slow excitation.** A spiking ring model of prefrontal spatial
working memory: tuned excitation, broad inhibition, and NMDA-like slow excitation; a bump
holds a location through the delay and drifts slowly. With fast excitation it fails.
Source: Compte, Brunel, Goldman-Rakic and Wang 2000, "Synaptic mechanisms and network dynamics
underlying spatial working memory in a cortical network model", Cereb Cortex 10:910.
https://doi.org/10.1093/cercor/10.9.910
Aim: gap. Here: in a memory region, excitatory slots use long delays and slow leak; inhibitory
slots short delays. Cost: fits. Ideas: 111. Matters: high; it names the parameter (slow
excitation) the hold search should look at.

**68. Grid cells by path integration.** A continuous attractor sheet shifted by velocity
inputs integrates position over tens of meters without resets.
Source: Burak and Fiete 2009, "Accurate path integration in continuous attractor network models
of grid cells", PLoS Comput Biol 5:e1000291. https://doi.org/10.1371/journal.pcbi.1000291
Aim: invariance (location), gap. Here: a region with a bump whose wiring is shifted by motor
lines. Cost: awkward (needs the shift wiring). Ideas: 112. Matters: low until there is a world
to move in.

**69. Grid codes as residue codes.** Several grid modules with different periods code
position like a residue number system: capacity grows exponentially with modules and errors
can be corrected.
Sources: Fiete, Burak and Brookings 2008, "What grid cells convey about rat location", J
Neurosci 28:6858 (m), https://doi.org/10.1523/JNEUROSCI.5684-07.2008 ; Sreenivasan and Fiete
2011, "Grid cells generate an analog error-correcting code for singularly precise neural
computation", Nat Neurosci 14:1330, https://doi.org/10.1038/nn.2901
Aim: capacity, gap (counting time). Here: pacemaker groups with coprime periods (say 5, 7, 9,
11 ticks); the joint phase codes elapsed time up to 3,465 ticks. Cost: fits (pacemakers
exist). Ideas: design mechanism 12. Matters: medium; a cheap clock for long gaps.

**70. Canonical microcircuit and layers.** Cortex repeats one circuit: input to layer 4,
processing in 2/3, output from 5/6, with feedforward and feedback paths in different
layers; predictive-coding versions assign errors to superficial and predictions to deep cells.
Sources: Douglas and Martin 2004, "Neuronal circuits of the neocortex", Annu Rev Neurosci
27:419 (m), https://doi.org/10.1146/annurev.neuro.27.070203.144152 ; Bastos et al. 2012,
"Canonical microcircuits for predictive coding", Neuron 76:695 (m),
https://doi.org/10.1016/j.neuron.2012.10.038
Aim: inside. Here: bit-position groups within a tile (idea 19). Cost: fits. Ideas: 19.
Matters: medium.

**71. Thalamic gating.** Thalamus does not only relay; it sets which cortical circuits talk,
switching effective connectivity by context.
Source: Halassa and Kastner 2017, "Thalamic functions in distributed cognitive control", Nat
Neurosci 20:1669 (m). https://doi.org/10.1038/s41593-017-0020-1
Aim: capacity, keeping. Here: like 53, but the gate words come from a small learned region.
Cost: fits. Ideas: 28. Matters: medium.

**72. Winner-take-all as a computing primitive.** A single (k-)winner-take-all stage over
weighted sums can compute anything a two-layer threshold network computes.
Source: Maass 2000, "On the computational power of winner-take-all", Neural Comput 12:2519
(m). https://doi.org/10.1162/089976600300014827
Aim: inside, capacity. Here: k winners per tile (56). Cost: fits. Ideas: 75, 9. Matters:
medium; argues 75 is not just a regulator.

**73. Divisive normalization.** A cell's response divided by the pooled activity of its
neighbors; found from retina to cortex; gives contrast and loudness invariance.
Source: Carandini and Heeger 2012, "Normalization as a canonical neural computation", Nat Rev
Neurosci 13:51 (m). https://doi.org/10.1038/nrn3136
Aim: invariance (intensity, thickness). Here: per-neuron threshold raised by the popcount of
its 9 local tiles last tick (a shift of the sum). Cost: fits (one sum per tile). Ideas: 108.
Matters: medium.

**74. Synfire chains.** Groups of neurons fire in sequence with precise timing, the volley
passing stably from group to group.
Source: Diesmann, Gewaltig and Aertsen 1999, "Stable propagation of synchronous spiking in
cortical neural networks", Nature 402:529 (m). https://doi.org/10.1038/990101
Aim: gap. Here: idea 16 (chains). Cost: fits. Ideas: 16. Matters: medium; 75 is how to get
them without designing them in.

**75. Chains grown by timing plus competition.** STDP alone does not organize a network into
long sequences; with heterosynaptic competition (a per-neuron budget) it does, producing many
chains with lengths spread as 1/L, matching songbird premotor data.
Source: Fiete, Senn, Wang and Hahnloser 2010, "Spike-time-dependent plasticity and
heterosynaptic competition organize networks to produce long scale-free sequences of neural
activity", Neuron 65:563. https://doi.org/10.1016/j.neuron.2010.02.003
Aim: gap. Here: our budget plus entry 1 (timing-based growth). Cost: fits. Ideas: 16, 81, 2.
Matters: high; we have the budget already and lack the timing rule.

**76. Polychronization.** With conduction delays and STDP, groups of neurons fire in
time-locked but not synchronous patterns; there are far more such groups than neurons.
Source: Izhikevich 2006, "Polychronization: computation with spikes", Neural Comput 18:245
(m). https://doi.org/10.1162/089976606775093882
Aim: gap, capacity. Here: delays (exist) plus wiring by exact delay (entry 1, idea 3). Cost:
fits. Ideas: 3, 15, 81. Matters: medium to high; explains why delays measured as nothing.

**77. Memory without feedback.** A purely feedforward chain of states can hold or integrate a
value as long as the chain is long; no attractor needed.
Source: Goldman 2009, "Memory without feedback in a neural network", Neuron 61:621.
https://doi.org/10.1016/j.neuron.2009.02.006
Aim: gap. Here: relays and highways (ideas 13, 14) laid along a long path, with delays.
Cost: fits. Ideas: 13, 14. Matters: medium.

**78. Predictive coding circuits.** Higher areas predict lower activity; lower areas send up
only the error. In cortex, mismatch neurons respond to the difference between expected and
actual sensory flow.
Sources: Rao and Ballard 1999, "Predictive coding in the visual cortex", Nat Neurosci 2:79
(m), https://doi.org/10.1038/4580 ; Keller and Mrsic-Flogel 2018, "Predictive processing: a
canonical cortical computation", Neuron 100:424 (m), https://doi.org/10.1016/j.neuron.2018.10.003
Aim: inside (no teacher), capacity. Here: design mechanism 16 extended to every neuron
(idea 11); error cells as XOR of predicted and actual words. Cost: fits (XOR and popcount).
Ideas: 11, 36. Matters: medium.

**79. Active sensing.** People choose fixations that reveal the most about the category
given what they have seen (about 70% efficient). Learned glimpse policies (trained by
REINFORCE) classify from a few small views.
Sources: Yang, Lengyel and Wolpert 2016, "Active sensing in the categorization of visual
patterns", eLife 5:e12215, https://doi.org/10.7554/eLife.12215 ; Mnih, Heess, Graves and
Kavukcuoglu 2014, "Recurrent models of visual attention" (m), https://arxiv.org/abs/1406.6247
Aim: invariance (position), acting. Here: motor lines that shift the window the world writes
onto the sensor lattice. Cost: fits (world side). Ideas: 105. Matters: medium; it moves the
problem to acting, which is also a wall.

**80. Songbird variability as the experimenter.** A premotor area (LMAN) injects variability
into the motor pathway; plasticity at the main pathway combines its own activity, the
variability input, and a delayed, coarse, binary reinforcement; learning time barely depends
on network size.
Source: Fiete, Fee and Seung 2007, "Model of birdsong learning based on gradient estimation by
dynamic perturbation of neural conductances", J Neurophysiol 98:2038.
https://doi.org/10.1152/jn.01311.2006
Aim: acting, inside. Here: a small "experimenter" group that nudges one button group for a
step; the eligibility is "was nudged and was active"; reward graded against its average.
Cost: fits. Ideas: 60, 38. Matters: high; it is idea 60 with a model that already works.

**81. Graded persistent firing in single cells.** Entorhinal layer V neurons keep firing at a
level set by their last inputs for minutes after input stops, through a calcium-activated
cation current switched on by acetylcholine.
Source: Egorov, Hamam, Fransén, Hasselmo and Alonso 2002, "Graded persistent activity in
entorhinal cortex neurons", Nature 420:173. https://doi.org/10.1038/nature01171
Aim: gap. Here: a latch type of neuron: no leak, exempt from rate regulation, potential
steps up on input bursts and down only on strong inhibition. Cost: fits. Ideas: 71, 69,
111. Matters: high; the simplest hold, and its exemption from regulation is the point.

**82. Activity-silent working memory.** Prefrontal delay activity comes and goes; content may
be held in short-term synaptic changes. Trained networks use this only for short delays and
need persistent firing to manipulate what they hold.
Sources: Stokes 2015, "'Activity-silent' working memory in prefrontal cortex", Trends Cogn Sci
19:394, https://doi.org/10.1016/j.tics.2015.05.004 ; Masse, Yang, Song, Wang and Freedman
2019, "Circuit mechanisms for the maintenance and manipulation of information in working
memory", Nat Neurosci 22:1159, https://doi.org/10.1038/s41593-019-0414-3
Aim: gap. Here: entries 21 and 143. Cost: fits. Ideas: 67. Matters: medium; it sets
expectations: silent holds are short.

**83. Active dendrites and sequence memory (HTM).** Each neuron has many dendritic segments;
a segment that matches a pattern depolarizes the cell (a prediction); fast inhibition lets
predicted cells fire first. Capacity grows with synapses per neuron; hundreds of patterns per
neuron. Synapses have a hidden "permanence" and are connected above a threshold.
Source: Hawkins and Ahmad 2016, "Why neurons have thousands of synapses, a theory of sequence
memory in neocortex", Front Neural Circuits 10:23. https://doi.org/10.3389/fncir.2016.00023
Aim: gap (sequence), capacity. Here: slots as segments (idea 72); a segment match lowers
threshold next tick; k-winners prefer predicted cells. Cost: fits. Ideas: 72, 26, 59.
Matters: medium to high; the closest prior design, and its permanence is our counter.

**84. Locations on objects (Thousand Brains).** Cortical columns hold grid-like location
codes in the object's own frame; a column learns which feature is at which location, and
recognizes objects by moving the sensor.
Source: Lewis, Purdy, Ahmad and Hawkins 2019, "Locations in the neocortex: a theory of
sensorimotor object recognition using cortical grid cells", Front Neural Circuits 13:22.
https://doi.org/10.3389/fncir.2019.00022
Aim: invariance. Here: a location code (69) updated by motor lines, ANDed with features in a
table neuron (idea 26). Cost: awkward (needs path integration). Ideas: 105, 26. Matters:
medium; needs a sensor that moves.

**85. Structure separated from content (TEM).** Entorhinal cells learn the structure of a task
(grid-like, abstract); hippocampal cells bind structure to the current sensory content; the
structure transfers to new environments.
Source: Whittington et al. 2020, "The Tolman-Eichenbaum machine", Cell 183:1249.
https://doi.org/10.1016/j.cell.2020.10.024
Aim: invariance (relational transfer). Here: a structure region driven by motor lines and a
binding region that ANDs it with sensor regions. Cost: awkward (trained by backprop in the
paper). Ideas: new. Matters: low now; tier 9.

**86. Successor representation.** Place cells may encode expected future occupancy of states
under the current policy, learned by a TD rule; it transfers when rewards move.
Source: Stachenfeld, Botvinick and Gershman 2017, "The hippocampus as a predictive map", Nat
Neurosci 20:1643 (m). https://doi.org/10.1038/nn.4650
Aim: acting. Here: idea 11 with a slow trace target. Cost: fits. Ideas: 11. Matters: low.

## E. Learning without backprop

### Sending an error inward without symmetric weights

**87. Feedback alignment.** Errors are sent back through fixed random weights instead of the
transpose of the forward weights; the forward weights come to align with them.
Source: Lillicrap, Cownden, Tweed and Akerman 2016, "Random synaptic feedback weights support
error backpropagation for deep learning", Nat Commun 7:13276.
https://doi.org/10.1038/ncomms13276
Aim: inside. Here: needs a real-valued error per layer. Cost: does not fit as stated; 88 and
89 are the forms that fit. Ideas: new. Matters: low directly.

**88. Direct feedback alignment.** The output error goes straight to every hidden layer
through fixed random weights; near backprop on MNIST.
Source: Nøkland 2016, "Direct feedback alignment provides learning in deep neural networks",
NeurIPS. https://arxiv.org/abs/1609.01596
Aim: inside. Here: each inner neuron keeps a fixed random sign per button (1 bit per button);
on reward, its learning direction is the sign-weighted error. Cost: fits with sign-only error
(a bit mask per neuron), awkward with magnitudes. Ideas: new. Matters: high; see 89.

**89. Direct random target projection (DRTP).** Even simpler: the one-hot target, not the
error, is projected through fixed random weights to each layer; the sign is enough to keep
alignment. Layers update during the forward pass with no backward pass. Built for edge
hardware; costs accuracy.
Source: Frenkel, Lefebvre and Bol 2021, "Learning without feedback: fixed random learning
signals allow for feedforward training of deep neural networks", Front Neurosci 15:629892.
https://doi.org/10.3389/fnins.2021.629892
Aim: inside. Here: each inner neuron gets a random "home button" (or a random sign per
button); when the right answer is known (any good; a bad with two buttons), a neuron whose
home button was right moves evidence up for its active sources, others down a little. One
small int per neuron, touched on reward. Cost: fits. Ideas: new (92 is near). Matters: high;
the cheapest inside rule with a published result.

**90. Difference target propagation.** Each layer gets a target computed by learned inverses
(autoencoders) of the layers above, corrected by the difference trick.
Source: Lee, Zhang, Fischer and Bengio 2015, "Difference target propagation", ECML PKDD.
https://arxiv.org/abs/1412.7525
Aim: inside. Here: needs learned inverse mappings per layer. Cost: does not fit. Ideas: new.
Matters: low.

**91. Contrastive Hebbian learning and equilibrium propagation.** Run the recurrent net to
rest freely, then with the output nudged toward the target; the difference of the two
Hebbian terms is the gradient. Only one kind of computation, local updates.
Sources: Scellier and Bengio 2017, "Equilibrium propagation", Front Comput Neurosci 11:24,
https://doi.org/10.3389/fncom.2017.00024 ; Xie and Seung 2003, "Equivalence of backpropagation
and contrastive Hebbian learning in a layered network", Neural Comput 15:441 (m),
https://doi.org/10.1162/089976603762552988
Aim: inside. Here: a few ticks free, a few ticks with the right button's cells forced on;
per connection, `coactive_clamped & ~coactive_free` gains, the reverse loses. Two co-activity
snapshots per neuron (bit masks). Cost: awkward (two phases per decision and settle time).
Ideas: 91, 39. Matters: medium to high; the sheet is recurrent, which this needs, and the
"show the answer" world (39) supplies the clamp.

**92. Prospective configuration.** Let activity settle to what it should be given the
outcome, then change weights to fix that activity in place; learns faster than backprop in
several biological settings and avoids interference.
Source: Song, Millidge, Salvatori, Lukasiewicz, Xu and Bogacz 2024, "Inferring neural activity
before plasticity as a foundation for learning beyond backpropagation", Nat Neurosci 27:348.
https://doi.org/10.1038/s41593-023-01514-1
Aim: inside. Here: same two-phase shape as 91 with energy-based settling. Cost: awkward.
Ideas: 91. Matters: medium.

**93. Forward-forward.** Each layer has its own goal: high "goodness" (sum of squared
activity) on real data, low on negative data; two forward passes, no backward pass. Slower
than backprop and generalizes a little worse on toy problems; hard to reproduce.
Source: Hinton 2022, "The forward-forward algorithm: some preliminary investigations".
https://arxiv.org/abs/2212.13345
Aim: inside. Here: per tile, goodness = popcount of spikes; on good stretches, connections
that raised it gain; on bad stretches, lose. Cost: fits. Ideas: new. Matters: medium; good
and bad are natural positive and negative data here.

**94. Local losses per layer.** Each layer trains against its own small classifier and a
similarity-matching loss; near state of the art without end-to-end backprop.
Source: Nøkland and Eidnes 2019, "Training neural networks with local error signals", ICML.
https://arxiv.org/abs/1901.06656
Aim: inside. Here: a mini-readout per sector (a few motor-like cells) trained by the readout
rule; the sector's neurons learn to make it right. Cost: awkward (more readouts). Ideas: 5,
92. Matters: medium.

**95. e-prop.** For recurrent spiking nets: each connection keeps an eligibility trace from
local quantities; a learning signal per neuron (from random feedback of the output error)
multiplies it. Online, no backprop through time, close to BPTT on many tasks.
Source: Bellec et al. 2020, "A solution to the learning dilemma for recurrent networks of
spiking neurons", Nat Commun 11:3625. https://doi.org/10.1038/s41467-020-17236-y
Aim: inside, gap. Here: eligibility = recent-before-spike bits (exist, 16 ticks) or a per-
neuron slow trace; learning signal = 88/89's sign bit per neuron. Cost: fits in the bit form,
awkward with real traces. Ideas: 52, 92. Matters: high.

**96. Dendritic cortical microcircuits.** Apical dendrites compute the mismatch between
top-down feedback and lateral interneuron predictions; learning on that mismatch
approximates backprop with no separate phases.
Source: Sacramento, Costa, Bengio and Senn 2018, "Dendritic cortical microcircuits approximate
the backpropagation algorithm", NeurIPS. https://arxiv.org/abs/1810.11393
Aim: inside. Here: apical slot (29) plus an inhibitory group that learns to cancel feedback.
Cost: awkward. Ideas: 29 here. Matters: medium.

**97. Predictive coding approximates backprop.** A predictive coding network with local
Hebbian updates converges to backprop's updates.
Source: Whittington and Bogacz 2017, "An approximation of the error backpropagation algorithm
in a predictive coding network with local Hebbian synaptic plasticity", Neural Comput 29:1229
(m). https://doi.org/10.1162/NECO_a_00949
Aim: inside. Here: entry 78 with an output clamp. Cost: awkward (settling). Ideas: 11.
Matters: low to medium.

### Reward-driven credit

**98. Attention-gated reinforcement learning (AGREL, Q-AGREL, BrainProp).** After a choice,
the selected action sends feedback down the network; the feedback tags the synapses that
supported that action; a global reward-prediction error converts tags into change. Proven
equal to backprop for one output at a time; matches it on MNIST to Tiny ImageNet at 1 to 3.5
times the training time.
Source: Pozzi, Bohté and Roelfsema 2020, "Attention-gated brain propagation: how the brain
can implement reward-based error backpropagation", NeurIPS.
https://proceedings.neurips.cc/paper/2020/file/1abb1e1ea5f481b589da52303b091cbb-Paper.pdf
Aim: inside. Here: on the decision tick, the pressed group's cells mark their active,
connected sources with a tag bit; optionally those mark theirs (one or two hops, along edges
that carried a spike); good or bad moves evidence of tagged neurons for their own active
inputs. Tag bit per neuron, a few map reads on reward. Cost: fits. Ideas: 62, 92. Matters:
high; it extends finding 15 inward.

**99. AuGMEnT: learning what to hold.** Sensory input arrives as instantaneous cells and as
on/off cells that fire on change. Memory units take only on/off input and integrate it with
no decay through a trial. Synaptic traces last the trial; feedback from the chosen action
turns traces into tags; a TD error converts them. Learned delayed saccade/antisaccade
(about 4,100 trials), delayed match-to-category, probabilistic classification. The authors
call perfect integration an idealization and reset everything between trials.
Source: Rombouts, Bohté and Roelfsema 2015, "How attention can create synaptic tags for the
learning of working memories in sequential tasks", PLoS Comput Biol 11:e1004060.
https://doi.org/10.1371/journal.pcbi.1004060 (read in full)
Aim: gap, inside. Here: on/off cells = sensor word XOR previous word, split by sign; memory
cells = latch neurons (81) fed only by those; reset by a rare global event (a good/bad, or a
long quiet). Tags as in 98. Cost: fits. Ideas: new (111 is the search for a hold). Matters:
high; the one published system that learns to hold from reward with local rules.

**100. Global error-vector broadcasting.** One vector-valued error is broadcast to all hidden
units; in networks built with nonnegative weights and vector units, local Hebbian-like
updates then match backprop in sign, and train convolutional layers where DFA cannot.
Source: Clark, Abbott and Chung 2021, "Credit assignment through broadcasting a global error
vector", NeurIPS. https://arxiv.org/abs/2106.04089
Aim: inside. Here: the error vector is the button one-hot difference (a few bits); each neuron
is assigned to one component. Cost: fits in sign form. Ideas: new. Matters: medium; the
architecture constraint (nonnegative, vector units) is the price.

**101. Synthetic gradients.** A small learned model at each layer predicts the gradient that
backprop would bring, so layers update without waiting.
Source: Jaderberg et al. 2017, "Decoupled neural interfaces using synthetic gradients", ICML.
https://arxiv.org/abs/1608.05343
Aim: inside, speed. Here: a predictor per sector of "will this sector's activity be
rewarded". Cost: does not fit (real-valued predictors trained by regression). Ideas: 90
(as a critic per sector, a cousin). Matters: low.

**102. REINFORCE with a baseline.** For stochastic units, reward minus a baseline times the
eligibility is an unbiased gradient; the baseline only cuts variance.
Source: Williams 1992, "Simple statistical gradient-following algorithms for connectionist
reinforcement learning", Mach Learn 8:229 (m). https://doi.org/10.1007/BF00992696
Aim: inside, acting. Here: idea 1, done correctly (finding 13). Cost: fits. Ideas: 1, 4.
Matters: low as is (finding 12); the baseline theory is what 107 sharpens.

**103. Associative reward-penalty units.** Stochastic binary units that learn input-to-action
maps from scalar reward; on penalty they move toward the opposite of what they did, at a
smaller rate. Networks of them learn jointly.
Source: Barto and Anandan 1985, "Pattern-recognizing stochastic learning automata", IEEE
Trans SMC 15:360. https://doi.org/10.1109/TSMC.1985.6313371
Aim: inside, acting. Here: our good/bad rule with an explicit penalty rate (lambda) below
the reward rate. Cost: fits. Ideas: 53 (lose-shift is near). Matters: medium; it has a
convergence result for exactly our kind of unit.

**104. Node perturbation.** Perturb each neuron's input slightly, correlate the change in
reward with the perturbation, update. Scales weakly with size in linear nets but is unstable
under mismatch: weights diffuse and grow without bound unless normalized.
Sources: Fiete and Seung 2006, "Gradient learning in spiking neural networks by dynamic
perturbation of conductances", Phys Rev Lett 97:048104, https://arxiv.org/abs/q-bio/0601028 ;
Hiratani, Mehta, Lillicrap and Latham 2022, "On the stability and scalability of node
perturbation learning", NeurIPS,
https://discovery.ucl.ac.uk/10166318/1/10935_on_the_stability_and_scalabili.pdf
Aim: inside. Here: idea 1. Cost: fits. Ideas: 1, 57. Matters: low per neuron (finding 12);
the budget is our weight normalization.

**105. Weight perturbation in time-extended tasks.** Perturbing each connection instead of
each neuron performs as well or better when tasks unfold over time and are low-dimensional.
Source: Züge, Klos and Memmesheimer, "Weight versus node perturbation learning in temporally
extended tasks: weight perturbation often performs similarly or better", preprint 2021, later
published in a journal not re-checked here. https://doi.org/10.1101/2021.10.04.463055
Aim: inside, acting. Here: AND a slot's weights with a sparse random flip mask for a window;
keep if reward beat its average. Cost: fits. Ideas: 31. Matters: medium; reopens idea 31 on
long tasks like cartpole.

**106. Evolution strategies with mirrored noise.** Perturb all parameters with noise and its
negative, compare the two returns; scales across thousands of workers.
Source: Salimans, Ho, Chen, Sidor and Sutskever 2017, "Evolution strategies as a scalable
alternative to reinforcement learning" (m). https://arxiv.org/abs/1703.03864
Aim: acting. Here: idea 32 (twin tournament) with the twin as the exact mirror flip set.
Cost: fits (two blobs). Ideas: 32. Matters: medium; the mirror halves the variance for free.

**107. Reward-modulated STDP needs a critic.** R-STDP learns only if the reward is centered
per stimulus (reward minus expected reward for that context); a global average fails when
several tasks or stimuli share the network.
Sources: Frémaux, Sprekeler and Gerstner 2010, "Functional requirements for reward-modulated
spike-timing-dependent plasticity", J Neurosci 30:13326 (m),
https://doi.org/10.1523/JNEUROSCI.6249-09.2010 ; Legenstein, Pecevski and Maass 2008, "A
learning theory for reward-modulated spike-timing-dependent plasticity with application to
biofeedback", PLoS Comput Biol 4:e1000180 (m), https://doi.org/10.1371/journal.pcbi.1000180
Aim: inside, acting. Here: a value cell per cue context (idea 5) instead of one running
average. Cost: fits. Ideas: 4, 5. Matters: high; it sharpens finding 13.

**108. Spiking actor-critic with TD.** A spiking critic learns a value with a TD rule; its
error modulates STDP in a spiking actor; solved navigation and cart-pole-like tasks.
Source: Frémaux, Sprekeler and Gerstner 2013, "Reinforcement learning using a continuous time
actor-critic framework with spiking neurons", PLoS Comput Biol 9:e1003024 (m).
https://doi.org/10.1371/journal.pcbi.1003024
Aim: acting. Here: 64 and 33 together. Cost: fits. Ideas: 90. Matters: high for cartpole.

**109. Aggregate-label learning.** A neuron learns to fire as many spikes as a delayed
feedback count says, with no timing given; it finds which features predicted the count, and
beats stochastic RL at finding predictive cues.
Source: Gütig 2016, "Spiking neurons can discover predictive features by aggregate-label
learning", Science 351:aab4113. https://doi.org/10.1126/science.aab4113
Aim: gap (credit), inside. Here: a motor group's spike count over a window compared with a
count from the world; evidence moves for the sources active before the spikes nearest
threshold. Cost: awkward (needs the near-threshold moments). Ideas: new. Matters: medium.

**110. Tempotron.** A neuron learns to fire or not for whole spatiotemporal spike patterns,
updating only at the moment of peak potential when wrong.
Source: Gütig and Sompolinsky 2006, "The tempotron: a neuron that learns spike timing-based
decisions", Nat Neurosci 9:420 (m). https://doi.org/10.1038/nn1643
Aim: inside, gap (patterns in time). Here: at a wrong decision, move evidence of the sources
active at the tick of highest potential in the window (store that tick's index per neuron).
Cost: fits (one index per neuron). Ideas: new. Matters: medium; a sharper eligibility than
"any recent".

### Unsupervised feature growth

**111. Self-organizing map.** The winning unit and its neighbors move toward the input;
nearby units come to code similar inputs.
Source: Kohonen 1982, "Self-organized formation of topologically correct feature maps", Biol
Cybern 43:59 (m). https://doi.org/10.1007/BF00337288
Aim: invariance (topography), capacity. Here: per tile, winner plus its 8 neighbors grow
toward the winner's active sources. Cost: fits. Ideas: 75. Matters: low to medium.

**112. Adaptive resonance (ART).** A category learns only if the input matches its template
above a vigilance threshold; otherwise a new category is recruited. Stable and plastic.
Source: Carpenter and Grossberg 1987, "A massively parallel architecture for a
self-organizing neural pattern recognition machine", Comput Vis Graph Image Process 37:54 (m).
https://doi.org/10.1016/S0734-189X(87)80014-2
Aim: keeping, capacity. Here: a neuron changes its wiring only if its match (popcount of
active and connected over connected) is above vigilance; else an idle neuron is recruited
(idea 25). Cost: fits. Ideas: new. Matters: medium; a direct answer to "learn B without
losing A".

**113. Anti-Hebbian decorrelation.** Lateral connections between outputs that fire together
weaken (become inhibitory), forcing outputs to code different things.
Source: Földiák 1990, "Forming sparse representations by local anti-Hebbian learning", Biol
Cybern 64:165 (m). https://doi.org/10.1007/BF02331346
Aim: capacity, inside. Here: inhibitory connections between co-active neurons in a tile
gain (25). Cost: fits. Ideas: 82. Matters: medium.

**114. SoftHebb.** Hebbian learning in soft winner-take-all layers, with no feedback or
error at all, trains deep nets to 99.4% on MNIST and 80% on CIFAR-10 with a linear readout.
Source: Journé, Garcia Rodriguez, Guo and Moraitis 2023, "Hebbian deep learning without
feedback", ICLR. https://arxiv.org/abs/2209.11883
Aim: inside. Here: k-winners per tile (56) plus growth toward the winners' active sources,
with a decay on the rest. Cost: fits. Ideas: 75. Matters: high; the strongest result for
"grow features, then select", which is what works here.

**115. Competing hidden units.** A local rule with global inhibition among hidden units
learns early features on CIFAR-10 close to backprop's.
Source: Krotov and Hopfield 2019, "Unsupervised learning by competing hidden units", PNAS
116:7723. https://doi.org/10.1073/pnas.1820458116
Aim: inside. Here: as 114 with anti-Hebbian push on the runners-up. Cost: fits. Ideas: 75.
Matters: medium.

**116. STDP with lateral inhibition and adaptive thresholds.** A spiking net with STDP,
lateral inhibition and a threshold that rises each time a neuron fires learns MNIST without
labels to 95% with 6,400 neurons; labels only assign neurons to classes afterward.
Source: Diehl and Cook 2015, "Unsupervised learning of digit recognition using
spike-timing-dependent plasticity", Front Comput Neurosci 9:99.
https://doi.org/10.3389/fncom.2015.00099
Aim: inside, capacity. Here: our co-activity growth plus k-winners plus a fast per-neuron
threshold rise on each spike (idea 68). Cost: fits. Ideas: 68, 75. Matters: high; MNIST is a
named milestone and this is the bit-friendly baseline for it.

**117. Bayesian winner-take-all STDP.** STDP in a WTA circuit performs expectation-
maximization: neurons come to represent hidden causes.
Source: Nessler, Pfeiffer, Buesing and Maass 2013, "Bayesian computation emerges in generic
cortical microcircuits through spike-timing-dependent plasticity", PLoS Comput Biol
9:e1003037 (m). https://doi.org/10.1371/journal.pcbi.1003037
Aim: inside. Here: as 116. Cost: fits. Ideas: 75. Matters: low beyond 116.

**118. Sparse coding.** Find a dictionary in which each input is a few active elements;
learned on natural images it gives V1-like edges. Locally competitive circuits (leaky
neurons, thresholds and lateral inhibition) compute the sparse codes.
Sources: Olshausen and Field 1996, "Emergence of simple-cell receptive field properties by
learning a sparse code for natural images", Nature 381:607, https://doi.org/10.1038/381607a0 ;
Rozell, Johnson, Baraniuk and Olshausen 2008, "Sparse coding via thresholding and local
competition in neural circuits", Neural Comput 20:2526, https://doi.org/10.1162/neco.2008.03-07-486
Aim: capacity, inside. Here: lateral inhibition weighted by overlap of two neurons' masks
(popcount of AND). Cost: awkward (overlap per pair). Ideas: new. Matters: low to medium.

**119. Slow feature analysis.** Find the features of the input that change most slowly;
gives invariant features; learning several invariances at once degrades performance.
Source: Wiskott and Sejnowski 2002, "Slow feature analysis: unsupervised learning of
invariances", Neural Comput 14:715. https://doi.org/10.1162/089976602317318938
Aim: invariance. Here: the exact method needs whitening and eigenvectors; the trace rule
(120) is its local form. Cost: does not fit as stated. Ideas: 102. Matters: medium as theory.

**120. The trace rule, and temporal contiguity in monkey IT.** Hebbian learning against a
slow trace of the cell's own activity wires successive views of a moving thing to the same
cell; with competition between cells, this gives position invariance. In monkeys, swapping an
object's identity as it moved across the retina changed IT neurons' position tolerance
within an hour of unsupervised exposure.
Sources: Földiák 1991, "Learning invariance from transformation sequences", Neural Comput
3:194, https://doi.org/10.1162/neco.1991.3.2.194 ; Li and DiCarlo 2008, "Unsupervised natural
experience rapidly alters invariant object representation in visual cortex", Science
321:1502, https://doi.org/10.1126/science.1160028
Aim: invariance. Here: co-activity growth reads the target's slow trace (a recent-bit kept
for 30 to 100 ticks) instead of its current spike; k-winners for competition; the world must
show drift. Cost: fits. Ideas: 102, 110. Matters: high.

### Fixed rich sheets and memories

**121. Reservoir computing.** A fixed random recurrent net with a trained linear readout;
spiking version: the liquid state machine.
Sources: Jaeger 2001, "The 'echo state' approach to analysing and training recurrent neural
networks", GMD Report 148, https://www.ai.rug.nl/minds/uploads/EchoStatesTechRep.pdf ; Maass,
Natschläger and Markram 2002, "Real-time computing without stable states", Neural Comput
14:2531 (m), https://doi.org/10.1162/089976602760407955
Aim: inside (readability). Here: our frozen-sheet diagnostic is this. Cost: fits. Ideas: 41,
49. Matters: high as prior art for the headline.

**122. A reservoir from one node and a delay line.** One nonlinear node with delayed
feedback, read at virtual nodes along the delay, works as a reservoir; built in electronics
and lasers.
Source: Appeltant et al. 2011, "Information processing using a single dynamical node as
complex system", Nat Commun 2:468. https://doi.org/10.1038/ncomms1476
Aim: gap. Here: a ring of relays (idea 13) longer than 16 ticks, read at taps. Cost: fits.
Ideas: 13. Matters: medium; a cheap long delay.

**123. Deep reservoirs.** Stacked reservoirs develop multiple timescales layer by layer,
richer than one big reservoir of the same size.
Source: Gallicchio, Micheli and Pedrelli 2017, "Deep reservoir computing: a critical
experimental analysis", Neurocomputing 268:87. https://doi.org/10.1016/j.neucom.2016.12.089
Aim: gap, inside. Here: two sheets (idea 80) with slower leak in the second. Cost: fits.
Ideas: 80. Matters: medium.

**124. Extreme learning machines and random features.** One fixed random hidden layer and a
trained linear output; a cheap universal approximator.
Source: Huang, Zhu and Siew 2006, "Extreme learning machine: theory and applications",
Neurocomputing 70:489 (m). https://doi.org/10.1016/j.neucom.2005.12.126
Aim: inside (readability). Here: 121 without recurrence. Cost: fits. Ideas: 41. Matters:
low; a baseline for MNIST.

**125. Hyperdimensional (vector-symbolic) computing.** Things are random high-dimensional
binary vectors; binding is XOR, bundling is majority, sequence and position are
permutation; similarity is Hamming distance. Built for bits.
Sources: Kanerva 2009, "Hyperdimensional computing: an introduction to computing in
distributed representation with high-dimensional random vectors", Cogn Comput 1:139,
https://doi.org/10.1007/s12559-009-9009-8 ; Plate 1995, "Holographic reduced
representations", IEEE Trans Neural Netw 6:623 (m), https://doi.org/10.1109/72.377968
Aim: capacity, invariance (binding). Here: a 64-bit tile word is already a small hypervector;
XOR of two tiles binds, majority over tiles bundles, rotate permutes. Cost: fits perfectly.
Ideas: new. Matters: medium to high for composition (tier 9) and as the source of 157.

**126. Resonator networks.** Factor a bound vector (for example shape XOR position) into its
parts by iterating guesses against codebooks; far higher capacity than optimization methods.
Source: Frady, Kent, Olshausen and Sommer 2020, "Resonator networks, 1: an efficient solution
for factoring high-dimensional, distributed representations of data structures", Neural
Comput 32:2311. https://doi.org/10.1162/neco_a_01331
Aim: invariance (what versus where). Here: codebooks as stored tile words, unbinding by XOR,
cleanup by nearest stored word (popcount). Cost: awkward (codebook search). Ideas: new.
Matters: medium.

**127. Sparse distributed memory and the Willshaw net.** Store sparse binary patterns in
binary synapses by OR-ing outer products; recall by thresholded popcount; capacity is high
when patterns are sparse.
Sources: Willshaw, Buneman and Longuet-Higgins 1969, "Non-holographic associative memory",
Nature 222:960 (m), https://doi.org/10.1038/222960a0 ; Kanerva 1988, Sparse Distributed
Memory, MIT Press (m), https://en.wikipedia.org/wiki/Sparse_distributed_memory
Aim: capacity. Here: binary connections set by co-activity (our growth) with sparse codes;
capacity analysis tells how sparse. Cost: fits. Ideas: new. Matters: medium; it gives a
formula for "how many things" a binary sheet can hold.

**128. Dense associative memory.** Energy functions with sharper-than-quadratic interactions
store far more patterns than a Hopfield net of the same size.
Source: Krotov and Hopfield 2016, "Dense associative memory for pattern recognition", NeurIPS
(m). https://arxiv.org/abs/1606.01164
Aim: capacity. Here: table neurons (idea 26) and branch thresholds give higher-order
interactions. Cost: fits in that form. Ideas: 26. Matters: low to medium.

**129. HTM spatial pooler.** Columns with potential synapses and hidden permanences; k
winners by overlap; winners move permanences toward active inputs; "boosting" raises
columns that are rarely active. Online, sparse, robust to noise and cell loss.
Source: Cui, Ahmad and Hawkins 2017, "The HTM spatial pooler: a neocortical algorithm for
online sparse distributed coding", Front Comput Neurosci 11:111.
https://doi.org/10.3389/fncom.2017.00111
Aim: inside, capacity. Here: evidence counters per potential source, k-winners per tile,
boost by per-neuron threshold regulation. Cost: fits. Ideas: 59, 75. Matters: high.

### Logic-based and table learners

**130. Tsetlin machine.** Teams of Tsetlin automata (finite-state counters) include or exclude
literals in AND clauses; clauses vote; feedback games reward useful clauses. Competitive with
standard classifiers; since extended to regression, convolution and contextual bandits.
Source: Granmo 2018, "The Tsetlin machine: a game theoretic bandit driven approach to optimal
pattern recognition with propositional logic". https://arxiv.org/abs/1804.01508
Aim: inside, capacity. Here: each slot's mask is a clause; each possible source has a small
include/exclude counter (our evidence counters); clause = AND of included inputs (idea 27).
Cost: fits (it was designed for bits). Ideas: 27, 59. Matters: medium to high; the most
developed learner already in our form.

**131. Learning automata.** A unit picks an action with probabilities nudged up after reward
(linear reward-inaction) or also down after penalty; proven convergence in games of many
automata.
Source: Narendra and Thathachar 1974, "Learning automata: a survey", IEEE Trans SMC 4:323 (m).
https://doi.org/10.1109/TSMC.1974.5408453
Aim: acting. Here: reward-inaction is "on good strengthen, on bad do nothing", the mirror of
our rule; worth knowing that games of such units converge. Cost: fits. Ideas: 33. Matters:
low.

**132. n-tuple and RAM networks (WiSARD).** Random n-tuples of input bits address RAM cells
that store a bit per class; classification sums matches. Bledsoe and Browning 1959 is the
origin; WiSARD built it in hardware.
Sources: Bledsoe and Browning 1959, "Pattern recognition and reading by machine", Proc
Eastern Joint Computer Conf (m), https://doi.org/10.1145/1460299.1460326 ; Aleksander, Thomas
and Bowden 1984, "WISARD a radical step forward in image recognition", Sensor Review 4:120 (m),
https://doi.org/10.1108/eb007637
Aim: capacity. Here: table neurons (idea 26) are RAM neurons with counters. Cost: fits.
Ideas: 26. Matters: medium as prior art for finding 5.

**133. Binary networks with latent weights.** Binary weights in the forward pass, real
hidden weights updated by gradients and binarized.
Sources: Courbariaux, Bengio and David 2015, "BinaryConnect" (m),
https://arxiv.org/abs/1511.00363 ; Hubara et al. 2016, "Binarized neural networks" (m),
https://arxiv.org/abs/1602.02830
Aim: speed. Here: our evidence counters are latent weights without gradients. Cost: the
gradient part does not fit. Ideas: 59. Matters: low (entry 14 is the useful lesson).

**134. Differentiable logic-gate networks.** Each node is one of 16 two-input logic gates,
chosen by a relaxed softmax during training and fixed after; over a million MNIST images per
second on one CPU core.
Source: Petersen, Borgelt, Kuehne and Deussen 2022, "Deep differentiable logic gate networks",
NeurIPS. https://arxiv.org/abs/2210.08277
Aim: speed. Here: the trained form is pure bit logic, like ours; the training is backprop.
Cost: does not fit for learning online. Ideas: new. Matters: low; a reference point for speed
on MNIST.

### Searching for the rule itself

**135. Evolved Hebbian rules.** Evolve per-synapse Hebbian coefficients, start each life from
random weights; agents learned to drive and a quadruped adapted to damage within 100 steps.
Reviewers noted fixed weights beat the Hebbian ones on two of three settings.
Source: Najarro and Risi 2020, "Meta-learning through Hebbian plasticity in random networks",
NeurIPS. https://arxiv.org/abs/2007.02686
Aim: all. Here: the genome search over rule parameters. Cost: fits (outer loop). Ideas: 98,
99. Matters: medium.

**136. Evolved or meta-learned interpretable rules.** Search the space of plasticity rules
(symbolic expressions, polynomial expansions) for ones that make a network do a task family;
so far they mostly rediscover known rules (Oja, inhibitory balance, reward-modulated STDP
variants), sometimes improved.
Sources: Confavreux, Zenke, Agnes, Lillicrap and Vogels 2020, "A meta-learning approach to
(re)discover plasticity rules that carve a desired function into a neural network", NeurIPS,
https://papers.neurips.cc/paper_files/paper/2020/file/bdbd5ebfde4934142c8a88e7a3796cd5-Paper.pdf ;
Jordan, Schmidt, Senn and Petrovici 2021, "Evolving interpretable plasticity for spiking
networks", eLife 10:e66273, https://doi.org/10.7554/eLife.66273
Aim: all. Here: a grammar of rule pieces over our bits (which maps, which masks, which
broadcast, which chance) and genetic programming over it, scored across the task map. Cost:
fits (outer loop; the inner rules are bit ops). Ideas: 99. Matters: high; this is the
owner's "set it off for a week" done with a search instead of by hand.

**137. Learning algorithms evolved from scratch.** AutoML-Zero evolves whole training
programs from basic math operations and rediscovers gradient descent; VSML learns a learning
rule as tiny shared recurrent cells at every synapse.
Sources: Real, Liang, So and Le 2020, "AutoML-Zero" (m), https://arxiv.org/abs/2003.03384 ;
Kirsch and Schmidhuber 2021, "Meta learning backpropagation and improving it" (m),
https://arxiv.org/abs/2012.14905
Aim: all. Here: as 136 with a lower-level grammar. Cost: fits but very expensive. Ideas: 99.
Matters: medium.

**138. Differentiable and neuromodulated plasticity.** Train the plasticity coefficients (and
a network that outputs its own modulator) by backprop across lifetimes; plastic nets beat
fixed ones on memory and maze tasks.
Source: Miconi, Clune and Stanley 2018, "Differentiable plasticity: training plastic neural
networks with backpropagation", ICML. https://arxiv.org/abs/1804.02464
Aim: all. Here: offline only. Cost: does not fit online; fits as an offline tuner of
genome numbers. Ideas: 98. Matters: low.

**139. Neuroevolution of wiring (NEAT, HyperNEAT).** Grow networks by adding nodes and links
under evolution (NEAT); or evolve a function of two positions that paints the whole
connectivity pattern (HyperNEAT), giving regular, symmetric, large networks.
Sources: Stanley and Miikkulainen 2002, "Evolving neural networks through augmenting
topologies", Evol Comput 10:99, https://doi.org/10.1162/106365602320169811 ; Stanley, D'Ambrosio
and Gauci 2009, "A hypercube-based encoding for evolving large-scale neural networks", Artif
Life 15:185 (m), https://doi.org/10.1162/artl.2009.15.2.15202
Aim: invariance (regular wiring), all. Here: HyperNEAT's idea fits our genome: birth wiring
as a function of (source offset, target position), so a pattern repeats across the sheet.
Cost: fits (at birth). Ideas: 98, 83. Matters: medium; birth-time regularity is one way to
get "the same detector everywhere".

**140. Weight-agnostic networks.** Search only the architecture, with one shared weight
value; some reach about 92% on MNIST and solve control tasks with no trained weights.
Source: Gaier and Ha 2019, "Weight agnostic neural networks", NeurIPS.
https://arxiv.org/abs/1906.04358
Aim: acting, inside. Here: evidence that wiring alone carries a lot; our birth wiring could
be searched for cartpole directly. Cost: fits. Ideas: 98. Matters: low to medium.

**141. Neural cellular automata.** One small local update rule, the same in every cell,
grows and repairs a pattern; a variant classifies MNIST digits by local agreement. Trained by
backprop through time.
Source: Mordvintsev, Randazzo, Niklasson and Levin 2020, "Growing neural cellular automata",
Distill. https://doi.org/10.23915/distill.00023
Aim: alive (repair), invariance (self-classifying). Here: the sheet is a cellular automaton;
the training method does not carry over. Cost: does not fit for learning. Ideas: new.
Matters: low.

**142. Boltzmann machines and contrastive divergence.** Stochastic binary units; learning is
the difference of co-activity statistics between data-clamped and free-running phases;
contrastive divergence shortens the free phase to one step.
Sources: Ackley, Hinton and Sejnowski 1985, "A learning algorithm for Boltzmann machines",
Cogn Sci 9:147 (m), https://doi.org/10.1207/s15516709cog0901_7 ; Hinton 2002, "Training
products of experts by minimizing contrastive divergence", Neural Comput 14:1771 (m),
https://doi.org/10.1162/089976602760128018
Aim: inside (no teacher). Here: our units are already stochastic binary; two phases like 91.
Cost: awkward. Ideas: 91. Matters: low to medium.

### Memory devices from machine learning

**143. Fast weights.** A second set of weights, written by recent co-activity and decaying
quickly, acts as an attention over the recent past.
Source: Ba, Hinton, Mnih, Leibo and Ionescu 2016, "Using fast weights to attend to the recent
past", NeurIPS (m). https://arxiv.org/abs/1610.06258
Aim: gap. Here: a third mask plane per slot, set by co-activity, cleared with chance per
tick; counted in the popcount. Cost: awkward (a third plane, per-tick decay). Ideas: 7.
Matters: medium.

**144. Legendre memory units.** A small linear system whose state is the sliding window of
the input expressed in Legendre polynomials; holds over 100,000 steps; buildable from spiking
neurons.
Source: Voelker, Kajić and Eliasmith 2019, "Legendre memory units: continuous-time
representation in recurrent neural networks", NeurIPS.
https://papers.nips.cc/paper/2019/hash/952285b9b7e7a1be5aa7849f32ffff05-Abstract.html
Aim: gap. Here: needs real-valued linear dynamics. Cost: does not fit as stated; 62 is the
spiking cousin. Ideas: new. Matters: low.

**145. Adaptation as memory (LSNN).** Adding neurons whose threshold rises with each spike and
decays over seconds gives recurrent spiking nets working memory like an LSTM's.
Source: Bellec, Salaj, Subramoney, Legenstein and Maass 2018, "Long short-term memory and
learning-to-learn in networks of spiking neurons", NeurIPS. https://arxiv.org/abs/1803.09574
Aim: gap. Here: a share of neurons with a 4-bit adaptation counter added to threshold, up one
per spike, down with chance 2^-k per tick (k near 6 to 8 for hundreds of ticks). Cost: fits.
Ideas: 68. Matters: high; one counter per neuron, and the paper shows it is enough memory for
hard tasks (trained by BPTT there; with e-prop in entry 95).

**146. Selection among replicating patterns.** Proposals that the brain runs Darwinian
selection over neural patterns (copying good ones over bad ones); reviewed with skepticism.
Source: Fernando, Szathmáry and Husbands 2012, "Selectionist and evolutionary approaches to
brain function: a critical appraisal", Front Comput Neurosci 6:24 (m).
https://doi.org/10.3389/fncom.2012.00024
Aim: inside. Here: idea 30. Cost: fits. Ideas: 30. Matters: low; little evidence either way.

## F. Invariance and binding

**147. Complex cells and max pooling.** A cell fires for its preferred feature anywhere in a
patch by taking the max (OR) over simple cells at shifted positions; stacked, this gives the
Neocognitron and HMAX.
Sources: Fukushima 1980, "Neocognitron", Biol Cybern 36:193 (m),
https://doi.org/10.1007/BF00344251 ; Riesenhuber and Poggio 1999, "Hierarchical models of
object recognition in cortex", Nat Neurosci 2:1019 (m), https://doi.org/10.1038/14819
Aim: invariance. Here: an any-slot neuron (idea 72) whose slots look at neighboring tiles with
the same mask. Learned: the trace rule (120) wires it. Cost: fits. Ideas: 72, 103, 106.
Matters: high.

**148. Stored templates and their orbits (i-theory).** Store a few templates as they are seen
transforming (from video); for a new image, the distribution of its similarities to each
template's stored versions is invariant to those transformations, exactly for group
transformations.
Source: Anselmi, Leibo, Rosasco, Mutch, Tacchetti and Poggio 2016, "Unsupervised learning of
invariant representations", Theor Comput Sci 633:112. https://doi.org/10.1016/j.tcs.2015.06.048
Aim: invariance. Here: a group of cells each wired (by 120) to one view of one template; a
pooling cell counts how many of a template's views fire. Cost: fits. Ideas: 107, 110.
Matters: medium to high; explains why "same rule, same world" (107) can work.

**149. Maps where the change is a slide.** The primate retina-to-cortex map is roughly
log-polar, so scaling and rotation about the fixation point become shifts; the cochlea is
roughly log-frequency, so a change of pitch is a shift.
Source: Schwartz 1977, "Spatial mapping in the primate sensory projection: analytic structure
and relevance to perception", Biol Cybern 25:181 (m). https://doi.org/10.1007/BF01885636
Aim: invariance. Here: how the world lays input on the sensor lattice (world side), plus 147
on the sheet. Cost: fits. Ideas: 104. Matters: high; it turns four invariances into one.

**150. Time-warp invariance by shunting.** Synaptic conductance shortens a neuron's effective
time constant in proportion to input rate, so a pattern played faster or slower drives it the
same way; good on spoken words.
Source: Gütig and Sompolinsky 2009, "Time-warp-invariant neuronal processing", PLoS Biol
7:e1000141. https://doi.org/10.1371/journal.pbio.1000141
Aim: invariance (tempo). Here: leak shift chosen per tick from the neuron's input count (more
input, faster leak). Cost: fits (a small table lookup per neuron per tick). Ideas: new.
Matters: medium.

**151. Transient synchrony.** Neurons driven by inputs through different decay rates fall into
step briefly only when the right pattern arrives; the synchrony is the recognition event and
spans half a second or more.
Source: Hopfield and Brody 2001, "What is a moment? Transient synchrony as a collective
mechanism for spatiotemporal integration", PNAS 98:1282. https://doi.org/10.1073/pnas.98.3.1282
Aim: invariance (tempo), gap. Here: sensor lines feeding cells with varied leaks (exists); a
coincidence cell (idea 74) that fires when many of them cross together. Cost: fits. Ideas: 74.
Matters: medium; needs carefully tuned weights in the original.

**152. First-spike order.** Code a stimulus by which cells fire first; the order is unchanged
when overall intensity scales, and the first wave carries enough for fast recognition.
Sources: Thorpe, Delorme and VanRullen 2001, "Spike-based strategies for rapid processing",
Neural Netw 14:715 (m), https://doi.org/10.1016/S0893-6080(01)00083-1 ; Hopfield 1995,
"Pattern recognition computation using action potential timing for stimulus representation",
Nature 376:33 (m), https://doi.org/10.1038/376033a0
Aim: invariance (intensity, scale of a number line). Here: sensor plug writes stronger lines
earlier; delays at the readout compare orders. Cost: fits. Ideas: new. Matters: low to medium.

**153. Routing by control cells (shifter circuits).** Control neurons gate which input
positions feed a central window, so an object is shifted and scaled into a standard frame
before recognition.
Source: Olshausen, Anderson and Van Essen 1993, "A neurobiological model of visual attention
and invariant pattern recognition based on dynamic routing of information", J Neurosci
13:4700 (m). https://doi.org/10.1523/JNEUROSCI.13-11-04700.1993
Aim: invariance. Here: a "window" region whose slots point to several source tiles, with one
slot enabled at a time by a control word. Cost: awkward (per-slot enable bits steered by a
learned control). Ideas: 89, 105. Matters: medium.

**154. Agreement between parts (capsules, GLOM).** Parts predict the pose of the whole; a
whole is present when predictions agree, whatever its pose.
Sources: Sabour, Frosst and Hinton 2017, "Dynamic routing between capsules" (m),
https://arxiv.org/abs/1710.09829 ; Hinton 2021, "How to represent part-whole hierarchies in a
neural network" (m), https://arxiv.org/abs/2102.12627
Aim: invariance, composition. Here: coincidence cells (idea 74) over several part detectors
with matching offsets. Cost: awkward. Ideas: new. Matters: low to medium.

**155. Binding by synchrony.** Features of one object are bound by firing in step; separate
objects fire out of step. Contested: Shadlen and Movshon argue it is a theory of how binding
is signaled, not computed, and that the needed precision is implausible; Gray defends it.
Sources: von der Malsburg 1981/1994, "The correlation theory of brain function" (m),
https://doi.org/10.1007/978-1-4612-4320-5_2 ; Shadlen and Movshon 1999, "Synchrony unbound",
Neuron 24:67, https://doi.org/10.1016/S0896-6273(00)80822-3
Aim: capacity (several objects at once), composition. Here: spikes are already in time; a
coincidence cell reads same-tick firing. Cost: fits. Ideas: 74. Matters: low.

**156. Tensor-product binding.** A role and a filler are bound by their outer product; a
structure is the sum of its bindings.
Source: Smolensky 1990, "Tensor product variable binding and the representation of symbolic
structures in connectionist systems", Artif Intell 46:159 (m).
https://doi.org/10.1016/0004-3702(90)90007-M
Aim: composition. Here: a table neuron over a role group and a filler group is a binary
outer product. Cost: awkward (size grows as a product). Ideas: 26. Matters: low.

**157. Position as a bit rotation.** In vector-symbolic computing, position and sequence
order are encoded by permuting a vector; a cyclic shift is a permutation. On our tile words a
rotate by 1 shifts a pattern one cell along a row (with wrap into the next row) and by 8 one
row down, so "the same pattern one step over" is one instruction.
Sources: Kanerva 2009 (entry 125); Frady et al. 2020 (entry 126).
Aim: invariance (position). Here: a readout cell reads `rotl(word, 8k + j)` for a few small
shifts and ORs the matches; or evidence counters indexed by offset shared across shifts (idea
100) use rotation as the sharing map. Cost: fits (rotate is free on a CPU; a mux on an FPGA).
Ideas: 100 (engineered sharing). Matters: medium to high; it is the cheapest engineered
convolution on this substrate, and the honest control for anything biological.

## G. Learning hardware

**158. Loihi.** Intel's chip has a programmable learning engine per core: rules are sums of
products of pre and post spike traces and reward traces, applied at each synapse.
Source: Davies et al. 2018, "Loihi: a neuromorphic manycore processor with on-chip learning",
IEEE Micro 38:82. https://doi.org/10.1109/MM.2018.112130359
Aim: speed. Here: confirms three-factor rules with a few traces per synapse are what
hardware people chose to support. Cost: fits as design guidance. Ideas: new. Matters: low.

**159. TrueNorth.** A million neurons, binary synapses in a crossbar, deterministic or
stochastic neurons, no on-chip learning.
Source: Merolla et al. 2014, "A million spiking-neuron integrated circuit with a scalable
communication network and interface", Science 345:668 (m).
https://doi.org/10.1126/science.1254642
Aim: speed. Here: proof that binary crossbars at scale are cheap; nothing on learning. Cost:
fits. Ideas: new. Matters: low.

**160. SpiNNaker.** Many small ARM cores passing spike packets; plasticity in software.
Source: Furber, Galluppi, Temple and Plana 2014, "The SpiNNaker project", Proc IEEE 102:652
(m). https://doi.org/10.1109/JPROC.2014.2304638
Aim: speed. Here: the closest to our CPU path. Cost: fits. Ideas: new. Matters: low.

**161. BrainScaleS-2.** Analog neurons run 1,000 times faster than biology; a digital
plasticity processor next to the synapse array reads correlations and applies any rule
written in software (hybrid plasticity).
Source: Pehle et al. 2022, "The BrainScaleS-2 accelerated neuromorphic system with hybrid
plasticity", Front Neurosci 16:795876. https://doi.org/10.3389/fnins.2022.795876
Aim: speed. Here: confirms "rare rule runs on correlations the array measured" is the
hardware-friendly shape, as our design says. Cost: fits. Ideas: new. Matters: low.

**162. ODIN and MorphIC.** Tiny digital chips with on-chip spike-driven synaptic plasticity
(4-bit synapses on ODIN; binary weights with stochastic learning on MorphIC).
Source: Frenkel, Lefebvre, Legat and Bol 2019, "A 0.086-mm2 12.7-pJ/SOP 64k-synapse
256-neuron online-learning digital spiking neuromorphic processor in 28-nm CMOS", IEEE Trans
Biomed Circuits Syst 13:145. https://arxiv.org/abs/1804.07858
Aim: speed. Here: the nearest silicon to our design: few-bit synapses, chance updates, stop-
learning. Cost: fits. Ideas: new. Matters: low to medium (an FPGA reference).

**163. Binary stochastic synapses in memory devices.** CBRAM devices used as binary synapses
switched by chance, trained by a probabilistic STDP rule, learn auditory and visual patterns.
Source: Suri et al. 2013, "Bio-inspired stochastic computing using binary CBRAM synapses",
IEEE Trans Electron Devices 60:2402. https://doi.org/10.1109/TED.2013.2263000
Aim: speed. Here: confirms chance flips of binary synapses learn; the chance is the learning
rate (design mechanism 10). Cost: fits. Ideas: design mechanism 10. Matters: low.

**164. Compound synapses.** A synapse made of several bistable devices in parallel acts as a
small counter; four parts were enough for reliable digit classification under device noise.
Source: Bill and Legenstein 2014, "A compound memristive synapse model for statistical
learning through STDP in spiking neural networks", Front Neurosci 8:412.
https://doi.org/10.3389/fnins.2014.00412
Aim: capacity. Here: 2 bits of evidence per connection, as four bistable parts, is a
hardware-backed counter width. Cost: fits. Ideas: 59. Matters: low to medium (counter width).

**165. Stochastic computing.** Numbers as the density of 1s in random bit streams;
multiplication is AND, addition is a mux; cheap and noise tolerant, slow.
Source: Alaghi and Hayes 2013, "Survey of stochastic computing", ACM Trans Embed Comput Syst
12:92. https://doi.org/10.1145/2465787.2465794
Aim: speed. Here: our sparse random words are stochastic numbers; a learning chance of 3/8 is
an OR of sparse words. Cost: fits. Ideas: design mechanism 13. Matters: low.

**166. Probabilistic bits (p-bits).** Binary units that flip randomly with a tunable bias;
networks of them sample Boltzmann distributions and run logic backward (factor by running a
multiplier in reverse).
Source: Camsari, Faria, Sutton and Datta 2017, "Stochastic p-bits for invertible logic", Phys
Rev X 7:031014. https://doi.org/10.1103/PhysRevX.7.031014
Aim: inside (sampling). Here: our noisy neurons are p-bits; a clamped-output run is the
"inverted" mode, which is 91's second phase. Cost: fits. Ideas: 91. Matters: low.

## H. From the owner's books on disk

These come from the owner's copy of Arbib (ed.) 2003, *The Handbook of Brain Theory and
Neural Networks*, 2nd ed., MIT Press, ISBN 0-262-01197-2 (cited below as "Arbib 2003" with
the article's first page), and from Hawkins 2021. Several are hard to find on the open web
as mechanisms in their own right. Read from the converted text in
`thonksheet-library/text/`.

**167. Cancelling the predictable by anti-Hebbian learning (electric fish).** Cells in a
cerebellum-like structure get raw sensor input plus parallel fibers carrying motor commands
and body posture. The parallel-fiber synapses learn a *negative image* of the predictable
part of the input: a synapse active while the cell fires weakens; one active while the cell
is silent strengthens. Self-caused input is cancelled; anything new (prey) stands out. Found
in four groups of fishes.
Source: Bastian, "Electrolocation", in Arbib 2003, p. 391.
Aim: inside (no teacher), acting (telling self-caused input from the world). Here: sensor-
area cells get a slot from motor cells and a slot of slow context; the rule is Hebb with
the sign flipped: `active_pre & spiked` loses, `active_pre & ~spiked` gains. Cost: fits.
Ideas: new (11 and design mechanism 16 predict the input; this subtracts it). Matters: medium
to high; it makes "surprise" mean the world, not the blob's own presses, which the acting
worlds need.

**168. Competitive queuing.** A sequence is held all at once as a gradient of activity over
plan cells (first item most active). A choice layer picks the most active, acts, and deletes
that plan; repeat. Errors are swaps of neighbors, as in human serial recall; prefrontal
recordings show the gradient.
Source: Bullock and Rhodes, "Competitive queuing for planning and serial performance", in
Arbib 2003, p. 241.
Aim: gap (order, sequences), acting (a sequence of presses). Here: latch cells (81) whose
level is set by order of arrival (earlier = more input before the decay), a winner-take-all
over motor groups, and an inhibitory link from each motor group back to its plan latch.
Cost: fits. Ideas: new. Matters: medium; the task map asks for sequences of presses.

**169. Many small thresholded branches, wired by structure.** If thin dendritic branches are
separately thresholded subunits, a neuron's capacity depends on which inputs share a branch.
With binary weights the optimum is about 1,000 branches of about 10 synapses (about 25 with
4-level weights); such a cell learned 27,400 patterns where a same-size linear cell learned
600 (46 times more), and a structural rule (keep a synapse where its branch neighbors are
co-active, move it otherwise) could reach about 100 times.
Source: Mel, "Dendritic learning", in Arbib 2003, p. 320; Poirazi and Mel 2001 (entry 22).
Aim: capacity. Here: our slot is one linear branch over 64 sources with a single bonus
threshold. Split each slot's mask into small branches (say 4 groups of 16 bits, each with
its own small threshold) and let growth place a new connection in the branch whose other
connections were co-active with it. Cost: fits (more popcounts of narrower masks). Ideas: 72,
27 here. Matters: high; it is the most direct published lever on "more than about eight
things".

**170. Hebb at the delay: storing sequences.** Hebb's rule applied across a connection with
transmission delay d pairs the source's state d steps ago with the target's state now; a
network with a spread of delays stores and replays sequences and cycles, and recognizes them
relative to their end.
Source: Herz, "Temporal sequences: learning and global analysis", in Arbib 2003, p. 1167.
Aim: gap. Here: the timing rule of entry 1, using each slot's own delay. Cost: fits. Ideas:
3, 81. Matters: medium to high; a near-duplicate of 1 and 76, kept because it states the
theory for our exact layout (delay per connection).

**171. Gain fields and basis functions.** Parietal cells multiply a retinal position response
by a gain set by eye position; a population of such products is a basis from which a linear
readout gets position in head, body or world coordinates, and remapping across saccades
falls out.
Source: Pouget and Sejnowski, "Dynamic remapping", in Arbib 2003, p. 368.
Aim: invariance (frame of reference). Here: AND a sensor tile with a few "where the sensor
points" lines: cells that fire for feature-and-posture. The readout picks the frame. Cost:
fits. Ideas: 105. Matters: medium once the sensor moves.

**172. Delay-and-coincide motion detectors, and the veto.** The fly's correlation detector
multiplies one photoreceptor's signal with its delayed neighbor; the rabbit retina's
direction-selective cells instead veto responses to the null direction with delayed
inhibition (possibly shunting).
Sources: Borst and Dickinson, "Visual course control in flies", in Arbib 2003, p. 1205;
Grzywacz and Merwine, "Directional selectivity", in Arbib 2003, p. 353.
Aim: invariance (motion as a feature), gap. Here: `rotl(ring[t-d], 1) & ring[t]` gives
rightward motion per tile in two instructions; the veto form is `x & ~rotl(ring[t-d], -1)`.
Cost: fits. Ideas: 74, 73. Matters: medium; motion detectors feed the trace rule (120) and
give velocity to cartpole.

**173. Delay lines to a place code, calibrated by another sense.** The barn owl turns
interaural time difference into a map by axonal delay lines and coincidence cells (Jeffress);
when prisms shift its vision, the auditory map shifts to match, taught by the visual map.
Sources: Albeck, "Sound localization and binaural processing", in Arbib 2003, p. 1061;
Knudsen 2002, "Instructed learning in the auditory localization pathway of the barn owl",
Nature 417:322 (m), https://doi.org/10.1038/417322a
Aim: invariance (time to place), inside (one region teaching another). Here: delays plus
coincidence cells (74); a second sense fed through an apical slot (29) as teacher. Cost: fits.
Ideas: 74. Matters: medium.

**174. A clock from a changing population (eyeblink timing).** Conditioned eyelid responses
are timed to the cue-to-puff interval. Tapped delay lines would need hundreds of
milliseconds of delay; the alternative is a granule-cell population whose pattern keeps
changing after the cue (recurrent inhibition through Golgi cells), with Purkinje cells
learning which moment to respond to.
Sources: Grethe and Thompson, "Cerebellum and conditioning", in Arbib 2003, p. 187;
Buonomano and Mauk 1994, "Neural network model of the cerebellum: temporal discrimination
and the timing of motor responses", Neural Comput 6:38 (m),
https://doi.org/10.1162/neco.1994.6.1.38
Aim: gap (timing). Here: a region tuned to keep changing without repeating or dying for
hundreds of ticks after a cue (inhibition-dominated, varied delays); the existing readout
learns the moment. Cost: fits. Ideas: 111, 62. Matters: medium to high; for "press at the
right time" a moving pattern may be easier to get than a held one.

**175. Attention that sets each cue's learning rate.** In conditioning theory a cue's
learning rate is not fixed: it rises if the cue predicted the outcome better than other cues
(Mackintosh) or if outcomes after it were recently surprising (Pearce and Hall). These
explain blocking, overshadowing and latent inhibition, which a fixed rate cannot.
Source: Schmajuk, "Conditioning", in Arbib 2003, p. 256.
Aim: inside, capacity. Here: a small "associability" counter per sensor line or per source
tile, scaling the learning chance at the readout; raised after a surprising good or bad that
followed it. Cost: fits. Ideas: 6, 84. Matters: medium.

**176. One circuit, many configurations.** In crustacean and mollusk motor circuits,
modulators switch plateau properties and synapse strengths so that the same wiring produces
different rhythms, or merges and splits networks.
Source: Dickinson, "Neuromodulation in invertebrate nervous systems", in Arbib 2003, p. 757.
Aim: capacity, acting. Here: a modulator line that swaps a region's genome numbers
(thresholds, delays, inhibition gain) between two or three stored sets. Cost: fits. Ideas:
112, 53 here. Matters: medium.

**177. Neurons that tune themselves toward a firing pattern.** Model neurons sense calcium
with several sensors and adjust conductances until they reach a target pattern (for example
bursting) from any start; a circuit of such cells self-assembles its rhythm, and recovers it
a day after its modulators are removed.
Source: Abbott and Marder, "Activity-dependent regulation of neuronal conductances", in
Arbib 2003, p. 85.
Aim: alive. Here: regulate burstiness as well as rate: a per-neuron "fired twice" sensor that
nudges cooldown length. Cost: fits. Ideas: design mechanism 15, entry 18. Matters: low to
medium.

**178. Neuron-wide regulation keeps memories through synaptic loss; additive versus
multiplicative.** Rescaling all of a neuron's synapses to restore its input level maintains
stored memories while synapses decay or are deleted, and raises associative-memory capacity.
Additive constraints (fixed total) give sharp fields that keep only the most correlated
inputs; multiplicative ones keep graded fields.
Source: Chechik, Horn and Ruppin, "Hebbian learning and neuronal regulation", in Arbib 2003,
p. 511.
Aim: keeping, alive. Here: our budget is additive (one gained, one lost), so it should give
sharp, saturated fields; a multiplicative variant would weaken all of a neuron's
connections a little by chance in proportion to strength. Cost: fits. Ideas: 2. Matters: low
to medium; it says what kind of features finding 1's budget produces.

**179. Delay activity learned by repetition.** In monkey IT and prefrontal cortex, about 1 to
2% of cells in a column hold elevated firing for up to 30 s for a learned stimulus; new
stimuli show none until trained. Modeled as a local module where slow, stochastic Hebbian
synapses carve an attractor per repeated stimulus.
Source: Amit, "Cortical Hebbian modules", in Arbib 2003, p. 285.
Aim: gap. Here: a dense local region with co-activity wiring and a low learning chance; holds
should appear for frequent cues only. Cost: fits. Ideas: 111. Matters: medium; a hold can be
learned by plain Hebb, given repetition.

**180. The tuned integrator, and why bits make it free.** Recurrent feedback tuned exactly to
balance leak makes a network integrate its input (the oculomotor integrator); any mistuning
makes it drift or run away, which is the standard objection to integrator models.
Source: Seung, "Amplification, attenuation, and integration", in Arbib 2003, p. 94.
Aim: gap. Here: an integer counter with no leak is a perfect integrator with no tuning; the
biological objection does not apply. This is the case for latch and memory cells (81, 99)
being cheaper here than in a brain. Cost: fits. Ideas: 69, 111. Matters: medium as an
argument; the mechanism is 81.

**181. Dynamic links.** Connections whose strength changes as fast as activity bind features
into objects and match a stored graph onto an image, which gives recognition across position
and distortion (elastic graph matching, used for faces).
Source: von der Malsburg, "Dynamic link architecture", in Arbib 2003, p. 365.
Aim: invariance, binding. Here: a fast weight plane (143) between a model region and the
input, set within a few ticks by co-activity. Cost: awkward. Ideas: new. Matters: low to
medium.

**182. Habituation, dishabituation and sensitization.** Repeated stimuli lose effect
(homosynaptic depression); a strong different stimulus restores the response; spaced
repetition gives longer memory than massed; in Aplysia a modulator (serotonin) drives
presynaptic facilitation through several kinase systems with short, intermediate and
long-lasting phases.
Sources: Bristol, Purcell and Carew, "Habituation", in Arbib 2003, p. 504; Byrne and Crow,
"Invertebrate models of learning: Aplysia and Hermissenda", in Arbib 2003, p. 581.
Aim: keeping, inside (noticing the new). Here: a slow per-source depression (entry 20 with a
long recovery) reset by a strong global event; a presynaptic gain bit for sources active
when a modulator fires. Cost: fits. Ideas: 66. Matters: medium; it is how a sheet stops
reacting to the constant.

**183. Stochastic resonance.** The right amount of noise lets a subthreshold signal cross
threshold in proportion to its strength, so noise improves detection.
Source: Moss, "Stochastic resonance", in Arbib 2003, p. 1112.
Aim: alive (readability). Here: the noise rate as a tuned number, not off. Cost: fits.
Ideas: design mechanism 13. Matters: low (finding 6 says the sheet makes its own).

**184. Maps that follow use.** Body-surface representations of used or spared inputs expand
into less-used or deprived cortex.
Source: Florence and Kaas, "Somatotopy: plasticity of sensory maps", in Arbib 2003, p. 1057.
Aim: capacity. Here: co-activity growth already does this. Cost: fits. Ideas: 19 (growth).
Matters: low.

**185. Rhythms reset by sensing.** The locust flight pattern generator (delayed excitation
with feedback inhibition) has its phase transitions and frequency set by wing stretch
receptors and tegulae.
Source: Robertson, "Locust flight: components and mechanisms in the motor", in Arbib 2003,
p. 654.
Aim: acting, gap (beat). Here: a pacemaker group whose phase is reset by a sensor line, with
motor groups phase-locked to it. Cost: fits. Ideas: 86. Matters: low to medium (beat tasks).

**186. Columns that vote.** Each cortical column learns whole models of objects in its own
reference frame; columns reach a consensus through long-range links, so one glance with many
columns or many glances with one column both work.
Source: Hawkins 2021, *A Thousand Brains: A New Theory of Intelligence*, Basic Books, ch. 7
(owner's copy on disk).
Aim: invariance, capacity. Here: many small readouts (one per sector), each learning the same
task on its own patch; far links carry their votes to the motor groups. Learning the same
thing in every place is then expected, not a failure of sharing. Cost: fits. Ideas: 94, 5,
107. Matters: medium.

---

## Veins that turned out thin

- **Astrocytes.** One strong behavioral result (Mu et al. 2019, entry 42) and theory
  (Kozachkov et al., entry 43). Nothing yet on astrocytes as a general learning rule.
- **Adult neurogenesis.** Its existence in adult humans is disputed; the computational
  claims are hypotheses (entry 51).
- **Serotonin.** One clean effect (patience, entry 40); the rest is theory.
- **Thalamus and superior colliculus.** Mostly reviews and frameworks (entries 71, 79); few
  rules concrete enough to build.
- **Cellular automata that learn.** The working results train by backprop (entry 141).
- **Stochastic computing and p-bits.** Relevant to hardware, not to learning (165, 166).
- **Learning hardware generally.** It confirms the design's shape (few-bit synapses, chance
  updates, three-factor rules) but adds no new rule.
