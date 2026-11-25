# Outside catalogue

Mechanisms by which systems that were never built as learning machines adapt, remember,
search, coordinate and hand out credit, each turned into something a pile of bits with local
rules could do. Written 2026-11-25 as the outside source the owner asked for ("we dont want
you just trying all the standard stuff tried in non backprop nn science right?"). Neuroscience
and non-backprop machine learning are in `docs/mechanism-catalogue.md` and are not repeated
here; where an entry turns out to be one of those under another name, it says so.

Sources: papers, reviews and encyclopaedia entries on the web, plus the owner's books in
`thonksheet-library/text/` (Arbib's Handbook of Brain Theory, 2nd ed.; Kennedy, Eberhart and
Shi, Swarm Intelligence; Koza, Genetic Programming; Prusinkiewicz and Lindenmayer, The
Algorithmic Beauty of Plants; Hofstadter, Gödel, Escher, Bach; Sutton and Barto, Reinforcement
Learning, 1st ed.; Packard and Wolfram, Two-Dimensional Cellular Automata; Braitenberg,
Vehicles, read in full, section I), and the owner's two earlier designs (section J). How much
was read against recalled is at the end.

How to read an entry. **What**: what the system does and why it works there. **Source**.
**Bits**: the same move for a pile of bits with local rules. **Aim**: the wall or property it
is pointed at. Walls are: *inside* (deep units learning from outcomes), *gap* (holding
something across time), *carry* (learned in one place, used in another: invariance), *acting*
(many steps in a world), *many* (holding many things at once); plus *lock*, *alive*, *room*,
*no teacher*. **Cost**: fits, awkward, or does not fit, on a CPU. **Here**: where it already
appears (ideas.md numbers, or "cat N" for the mechanism catalogue). **Surprise**: ordinary, a
twist, or genuinely different, against what the project has tried.

---

## 1. The twenty to bet on

Most unlike anything in `docs/ideas.md`, still buildable, in order of how much I would bet.

1. **Tagged tables at geometric history lengths (89, TAGE).** Branch predictors already solve "act on a pattern seen many steps back" with nothing but hashed tables of 3-bit counters; it is the most direct attack on the gap wall in this whole catalogue.
2. **Credit by payment under conservation (62 bucket brigade, 63 Hayek).** Units pay the units that set them up, the world pays the last one, and money is conserved; Baum's finding that shared reward among simultaneously active units is a tragedy of the commons describes our current inner credit exactly.
3. **Credit by code (101, SPSA and dither).** Every unit perturbs at once with its own random sign code, so the experiment is decision-sized (finding 12) while each unit still gets its own credit from one correlation.
4. **A pacemaker and an accumulator (120).** Count ticks from cue onset, store the count at reward, respond when the current count is near a stored one; a gap of any length becomes a stored number instead of a held activity.
5. **Top-k evidence slots (93, Space-Saving).** Evidence counters for a few candidate sources per unit, the weakest slot handed to a newcomer with its count; finding 19's six-times memory cost goes away.
6. **Clonal selection with misfit-scaled mutation (1, 2).** Learn by copying units that matched before good and mutating copies more the worse they fit; a learning move the sheet has never had.
7. **Reverberation after a strong outcome (140, Braitenberg's Vehicle 13).** On a rare, strong good or bad, loop the last two or three recorded spike maps through the learning rule many times at once; a one-off event becomes frequent inside, and the project already keeps the maps.
8. **Gene duplication (42).** When a relied-on unit is pulled two ways, copy it; the original keeps the old job and the copy takes the new one, so adding does not overwrite.
9. **Extinction as new, context-tagged learning (117).** Never erase evidence on a reversed rule; add a separate, context-bound, faster-fading inhibitory counter; the animal data say this is how learners keep both.
10. **Virtual points (67, minority game).** Each unit holds a few candidate tables and scores all of them every step by what they would have earned; counterfactual credit with no extra trials.
11. **Kinetic proofreading (33).** A candidate wire must survive k checks in a row, any of which resets it; discrimination goes to the k-th power, which is different from adding evidence.
12. **Kill the winner (47).** Tax every answer in proportion to how often it is given, not to how often it is punished; the ecological fix for monopoly is aimed at the lock.
13. **Optimism (141, Vehicle 14).** Keep transition and value counters; at each choice prefer the action whose likely next state is best; the project has no lookahead at all, and this is the cheapest kind.
14. **Run and tumble (31).** Repeat the last action while a fast average of reward beats a slow one, switch at random when it falls behind; a bacterium's one-number policy for acting in a world.
15. **Parity checks with bit-flip recall (100, LDPC).** Store patterns as codewords of sparse XOR checks; recall by flipping any bit most of whose checks fail; cleanup memory in pure bit operations.
16. **Clock and wavefront (16).** Every unit runs a phase counter; a sweeping front freezes the phase where it passes; time becomes a spatial pattern that just sits there.
17. **Recruitment, stop signals and quorum (56, honeybees).** Button groups recruit, cross-inhibit with stop bits and commit at a quorum; breaks ties and sets a speed-accuracy dial at the readout.
18. **Context-tree weighting (91).** Counts of the next bit for every suffix of recent history, mixed by stored local weights; provably near-optimal prediction for sources with bounded memory, in counters and lookups.
19. **Stigmergy (55, and Vehicle 5's beach).** Units write marks into a shared scratch grid that is not the wiring and act on what they read there; memory held in a world the sheet makes for itself.
20. **Copy by similarity (68, Axelrod).** Neighbouring tiles copy one differing bit from each other with probability equal to how alike they already are; features spread across the sheet while regions stay distinct.

Just outside the twenty: hysterons (78), the less loaded of two (97), negation lines (142, cheap
enough to try at once), the owner's state-addressed machine (149), and threshold control as the
pump of thoughts (138), which says the pulse the project removed may be an engine.

## 2. Whole systems that are not a sheet

**A repertoire.** The units are detectors, each a bit mask over the input (and over a short
history register), a threshold, an answer and a small wealth counter; there are a fixed number
of them, say 64k. Every tick the detectors that match bid on the answer; after an outcome the
ones that were right are cloned into the slots of the poorest detectors, copies mutated by a
number of bits that grows with how badly the parent fit (entries 1, 2); detectors that stay
poor die and their slots are refilled at random (9); a detector that wins often spawns a
frozen low-threshold memory copy that is outside the competition (7); a detector that fires
on the reward-free stream is deleted before it can join (4), which leaves a novelty sensor for
free. It learns one-shot (one clone is already a memory) and holds as many things as it has
slots, with no wiring to saturate. Cost: one popcount per detector per tick, cut to a few
percent by hashing the input into buckets of detectors. Walls: *many* and *lock* directly; *carry*
if detectors match on relative offsets; the *inside* wall disappears because there is no
inside, only a population.

**A market.** The units are small programs (a table neuron is enough) with wallets. Each step
the right to act (press a button, or write to a shared register) is auctioned: every unit
that wants it bids its estimate of what the next state is worth, capped by its wallet; the
winner pays its bid to the previous owner, acts, takes any reward from the world, and is paid
by the next winner (62, 63). Money is conserved between units; a unit that is rich spawns a
mutated child and takes a share of its profit; a unit poorer than its starting capital is
removed. Credit reaches a unit five steps from the reward because each owner was paid by the
next, which is the *inside* and *acting* walls together. Baum and Durdanovic report that this
learned 200-block Blocks World stacks where genetic programming and TD learning managed at
most 8 (entry 63), and that breaking conservation or ownership made it learn to exploit the
bookkeeping instead. Cost: one max over bids per step, plus wallet updates for two units;
cheap. Risk: one owner at a time makes it serial; a sheet could supply the features the
bidders read.

**A predictor bank.** No neurons: a set of hashed tables, each indexed by a different length
of recent history (2, 4, 8, ... 64 ticks of cue and action bits), each entry a 3-bit counter,
a partial tag and a usefulness bit (89), plus a pacemaker count since the last cue (120), plus
a chunk dictionary that turns repeated sequences into new symbols (92, 126). It predicts
either the reward of each button or the next input; the longest table that has a matching
entry answers; on a miss a new entry is allocated in a longer table; usefulness bits decide
what is overwritten. A context-tree mixture (91) is the principled version. Cost: a handful of
hash lookups per tick, the kind of thing CPUs do billions of times a second. Walls: *gap*,
*many*, *no teacher* (predicting the next input is the same machinery). It has nothing for
*carry*; a sheet in front of it, frozen or not, would supply features and the bank would supply
time.

**An ecology of small engines.** The unit is a whole small learner (a minimal engine of a few
hundred neurons, or a repertoire). A population of them sees the same stream; they compete for
a fixed resource (the right to vote on the answer, weighted by recent payoff), with the share
of any one taxed in proportion to how dominant it is (47), credit split among engines that
behave alike (48), a successful engine copied into the slot of a failing one (42), useful
fragments passed between engines (43), and each engine switching at random between two
settings at a rate tuned to how often the world changes (34). Within each engine nothing new
is needed. Cost: N copies of a small engine, run in turn; N of 16 to 64 is affordable. Walls:
*lock* (a dominant wrong answer is taxed away), *changed rule* (a minority is always ready),
*carry* by transfer between engines that saw different places. It is evolution inside one life,
with the owner's "nothing frozen" rule kept, and with Braitenberg's rule for copying (132):
swap whole parts, never splice inside one.

**A state that addresses its next step (after the owner's vvm, entry 149).** The unit is an
operation word in a bank of a few thousand; the only activity is one state bit vector (a few
hundred bits, sensor lines included). Each tick the state is matched against the bank by
popcount, the best match (sampled among the top few) is XORed or ORed into the state, the state
is thresholded, and that is the next query; a just-used operation sits out a few ticks. On good,
the used operations move a step toward the queries that called them; on bad, away. Memory and
program are the same thing: a cue that leads to the right chain of operations is held by
running, not by standing activity, and a chain learned in one context runs wherever the state
comes to look the same (*carry* through similarity of state, not of place). Cost: one hashed
nearest-match per tick, cheap. Walls: *gap* and *acting* by construction; *many* by bank size.
Risk: loops and collapse onto a few operations, which his refractory lockout and rejection were
written to stop.

## 3. Behaviour checks the task map lacks

Small worlds from the conditioning literature and elsewhere. Each has a control group (the same
design, with the first phase left out) and a plain pass rule.

1. **Habituation.** A harmless cue repeated every 5 ticks, no reward. Pass: response falls;
   after a rest it recovers; a second series falls faster (savings); a different strong cue
   restores it (dishabituation); a similar cue is partly habituated (generalisation). Rankin
   et al. 2009 list ten such properties; Arbib, "Habituation".
2. **Blocking.** Phase 1: cue A, press 1 is good. Phase 2: A and B together, press 1 is good.
   Test B alone. Pass: B drives press 1 much less than in a control that skipped phase 1.
3. **Overshadowing.** A strong cue (many lines) and a weak cue (few lines) together predict
   the answer. Pass: the weak cue alone is learned less than when trained alone.
4. **Conditioned inhibition.** A means press 1 is good; A with X means no press is good. Test
   X with another trained cue C. Pass: C with X drives press 1 less than C alone.
5. **Latent inhibition.** Show cue A many times with nothing following. Then train A to a
   button. Pass: slower than a fresh cue.
6. **Extinction, renewal, recovery, reacquisition.** Train A to press 1 with context lines in
   pattern X. Then make nothing good (extinction) in context Y. Test A in X (renewal), after a
   long idle (spontaneous recovery), and retrain (savings). Pass: the old answer comes back
   in X and after idle, and relearning is faster than first learning.
7. **Partial reinforcement.** Two lives: reward on every correct press, or on half. Then
   extinction. Pass: the half-rewarded life keeps answering longer.
8. **Contingency, not contiguity.** Reward after the cue at rate p, and reward without the cue
   at the same rate p. Pass: no learning. Then drop the cue-less rate to zero. Pass: learning.
   (Rescorla 1968.)
9. **Second-order conditioning and sensory preconditioning.** A then B paired without reward;
   B then trained to press 1. Test A. Pass: A drives press 1 above a control where A and B
   were never paired.
10. **Overexpectation.** A good alone and B good alone; then A and B together still good once.
    Pass: A alone then drives the answer less (the summed prediction was too high).
11. **Occasion setting.** A then B: press 1 is good; B alone: nothing is good. Pass: B is
    answered only when A came first. (Close to the order task; the difference is that A
    itself is never paired with the answer.)
12. **Serial reversal.** The good button swaps every block. Pass: each reversal is learned
    faster than the one before.
13. **Learning set.** A new pair of cues every 6 trials (Harlow 1949). Pass: trial-2 accuracy
    in a block climbs across blocks toward win-stay-lose-shift, though trial 1 stays at chance.
14. **Matching on concurrent schedules.** Two buttons whose rewards bank up at different rates
    and are collected by the next press. Pass: the share of presses tracks the share of
    rewards (Herrnstein 1961), and shifts when the rates are swapped.
15. **Interval timing (peak procedure).** Press 1 pays only after T ticks from cue onset; some
    trials never pay. Pass: press rate peaks near T and the width of the peak grows in
    proportion to T (the scalar property).
16. **Latent learning.** A small maze explored with no reward, then food placed. Pass: faster
    to the food than a design that never explored (Tolman 1948). Its mirror: a shortcut opens
    or a path closes mid-life, and the time to use it is measured (Sutton and Barto, ch. 9).
17. **Statistical segmentation.** A continuous stream of tokens from four three-token words,
    no reward; then test whether words are more familiar than part-words (Saffran et al. 1996).
    Needs a familiarity readout, which is a no-teacher output the map does not have yet.
18. **Spacing.** The same number of training trials, massed or spaced by idle stretches. Pass:
    after a long idle the spaced life remembers more.
19. **Noncontingent reward.** Good arrives on a fixed clock whatever is pressed. Not pass/fail:
    record whether the design locks onto an arbitrary press (Skinner's superstition) and how
    hard; it is the lock wall seen from the other side.
20. **Damage.** After learning, silence 10% then 30% of units at random. Pass: graceful loss
    and relearning faster than first learning (a hole already named in the task map).
21. **Occlusion.** A target moves at constant speed and passes behind a screen for a few ticks.
    Pass: the design's catcher (or gaze, idea 105) is at the exit point when the target
    reappears, ahead of a design that only follows what it sees (Braitenberg's Vehicle 13).
22. **Rare decisive event.** One cue is harmless 99 times and very bad once. Pass: after the one
    bad meeting, the design avoids it far more than the average outcome warrants (Vehicle 13's
    one-shot learning; entry 140). Not a pass/fail on its own: the right amount is a judgement.

## 4. What this material says about the findings

- **Shared credit is a commons.** Baum and Durdanovic: when many agents are active together
  and reward is shared among them, any agent profits by being active when reward is paid even
  if it harms the result; such systems "evolve complex behavior but not accurate credit
  assignment". Finding 14's best inner credit is "every neuron that spiked". Entries 62, 63,
  17 and 37 are ways to make credit conserved or owned.
- **Locks are frequency problems.** Ecology keeps diversity with taxes proportional to
  abundance (47) and with random switching (34), not with punishment. Lose-shift (idea 53)
  reacts to bad; the lock is a monopoly and might need a tax on winning.
- **Plasticity has a ceiling.** The error threshold (41): stored information survives only
  while changes per selection cycle stay under about one per stored item. Finding 4 (idle
  rewiring wipes everything) is that threshold being crossed with no selection at all.
- **Adding and multiplying evidence differ.** Evidence counters add (finding 14); proofreading
  (33) multiplies, and nucleation (77) needs a quorum before anything commits. Width results
  ("6 bits best, 8 worse") may be saying that a sum is the wrong shape, not the wrong size.
- **The pulse may be an engine.** Finding 2 removed sector regulation because it made the
  sheet pulse. Braitenberg's Vehicle 12 (entry 138) uses exactly that oscillation, with
  strong groups and one-way timing wires, to step from one held assembly to its successor.
  Worth a region of its own (idea 112) once timing wiring exists, rather than gone everywhere.
- **Holding plus co-activity wiring makes mush.** Braitenberg (entry 136): an idea held
  active wires all its members to each other and loses their order. Idea 111 (a place that
  holds) and finding 3 together will do this unless held activity circulates or wiring pauses.
- **Decision-sized experiments need not mean coarse credit.** Finding 12 says single-neuron
  noise almost never changes the answer. Simultaneous perturbation (101) perturbs everything
  at once with known codes and still credits each unit separately.

---

## A. The immune system

**1. Clonal selection and expansion.** *What*: a huge random repertoire of lymphocytes, each
with one receptor; the few that bind an invader divide fast, so the response is made by
copying the units that already fit. It works because generating variety is cheap and
selection by binding needs no teacher. *Source*: Farmer, Packard and Perelson 1986, "The
immune system, adaptation, and machine learning", Physica D 22
([record](https://www.inet.ox.ac.uk/publications/the-immune-system-adaptation-and-machine-learning)).
*Bits*: units are detectors (mask, threshold, answer); a detector that fired before good is
copied into the slot of the least successful detector; copies count as evidence. *Aim*: many,
lock. *Cost*: fits (a slot copy is a memcpy, done only on outcomes). *Here*: ideas 30, 31;
cat 146 is related. *Surprise*: genuinely different; learning by copying units, not by
changing wires.

**2. Hypermutation scaled by misfit (affinity maturation).** *What*: copies of a B cell
mutate their receptor gene at a very high rate, and in algorithms built on it the mutation
rate falls as fit improves, so good fits are refined and poor ones explore. *Source*: de
Castro and Von Zuben 2002, CLONALG, IEEE TEC 6(3)
([pdf](https://www.dca.fee.unicamp.br/~vonzuben/research/lnunes_dout/artigos/ieee_tec01.pdf)).
*Bits*: when a detector is cloned, flip k bits of the copy's mask with k = small when its match
margin was large and larger when small; keep whichever of parent and copies scores best over
the next few outcomes. *Aim*: inside (each detector hill-climbs on its own). *Cost*: fits.
*Here*: idea 31 is the flat-rate version. *Surprise*: a twist; step size tied to fit.

**3. Selection by competing for a few helpers.** *What*: in the germinal centre, B cells
compete for a limited number of helper T cells, and division and mutation scale with how much
antigen each captured and showed to them; cells cycle between a mutate zone and a test zone.
*Source*: Gitlin, Shulman, Nussenzweig 2014, Nature
([PMC4037377](https://pmc.ncbi.nlm.nih.gov/articles/PMC4037377)). *Bits*: per round, a fixed
small number of "help" tokens; detectors that matched rank by margin and only the top token
holders may be cloned. A fixed budget turns absolute evidence into rank. *Aim*: room, many.
*Cost*: fits. *Here*: finding 1 (a budget) by another route. *Surprise*: a twist; the budget is
on reproduction, not on wires.

**4. Negative selection.** *What*: young T cells that bind anything in the thymus (self) are
destroyed, so the survivors react only to non-self. *Source*: Forrest, Perelson, Allen,
Cherukuri 1994, "Self-nonself discrimination in a computer"
([record](https://ideas.repec.org/p/wop/safiwp/94-06-038.html)). *Bits*: generate random
detectors; run them on the ordinary stream for a while; delete any that fire; the survivors
fire only on what is new. *Aim*: no teacher (the map's "notice the odd one"), novelty gating
for ideas 84 and 36. *Cost*: fits. *Here*: not present. *Surprise*: genuinely different;
novelty detection by deleting, not by learning.

**5. Danger signals and the dendritic-cell window.** *What*: in Matzinger's view the immune
system answers damage, not foreignness; the dendritic cell algorithm samples everything it
meets over a window, sums "safe" and "danger" signals, and labels all of it by the majority
when the window closes. *Source*: Greensmith, Aickelin, Cayzer, "Detecting danger: the
dendritic cell algorithm"
([record](https://researchportal.bath.ac.uk/en/publications/detecting-danger-the-dendritic-cell-algorithm/)).
*Bits*: collector units log the hashes of active features as they go; they add good and bad
into two counters; when the total passes a threshold (not a fixed time) the collector closes
and every logged feature gets the majority label. *Aim*: gap (credit across delays),
cartpole's sparse reward. *Cost*: fits. *Here*: idea 52 is the fixed-window version.
*Surprise*: a twist; windows closed by signal amount, credit given in batches.

**6. Tolerance by tuning.** *What*: a lymphocyte stimulated without the second, confirming
signal does not ignore the stimulus but becomes harder to activate, and thresholds track the
background stimulation each cell gets. *Source*: Grossman and Paul 1992, PNAS
([PMC50339](https://pmc.ncbi.nlm.nih.gov/articles/PMC50339)). *Bits*: a unit strengthens only
when its input match coincides with a confirm line (reward, or a second region agreeing);
match without confirm raises its threshold one step. *Aim*: lock, room. *Cost*: fits. *Here*:
finding 2 and cat 17-18 (rate regulation); cat 8 (three factors). *Surprise*: ordinary, with
one twist: unconfirmed activity actively tolerises.

**7. Memory cells set aside.** *What*: after a response some cells become long-lived memory
cells with lower thresholds that answer faster the second time and are not in the race any
more. *Source*: Farmer et al. 1986 (entry 1); Freitas and Rocha 2000 (entry 9). *Bits*: a
detector that won often spawns a frozen copy with a lower threshold, kept out of replacement
and out of the clone competition. *Aim*: many, keeping. *Cost*: fits. *Here*: idea 35; cat 14,
54. *Surprise*: ordinary.

**8. The idiotypic network.** *What*: antibodies recognise each other as well as antigens;
stimulation and suppression among them keep a repertoire and its memory alive without the
antigen. *Source*: Farmer et al. 1986 (entry 1); Jerne's network theory. *Bits*: detectors
also match each other's masks as bit strings; a detector matched by many is suppressed, one
matched by few and by the input is stimulated; the repertoire holds itself without input.
*Aim*: gap, alive. *Cost*: awkward (pairwise matching; hashing helps). *Here*: not present.
*Surprise*: genuinely different, but hard to make useful.

**9. A repertoire of fixed size with niches.** *What*: lymphocyte numbers are held constant;
new cells must displace residents, survival needs continual receptor engagement, and
different survival signals let different kinds coexist. *Source*: Freitas and Rocha 2000,
"Population biology of lymphocytes: the flight for survival", Annu Rev Immunol
([pdf](https://hal.archives-ouvertes.fr/hal-00108283/file/ARI.pdf)). *Bits*: a detector needs
a match every N ticks or it dies and its slot is refilled at random; slots are split into
quotas by input region, so no region can take all. *Aim*: room. *Cost*: fits. *Here*: idea 25,
cat 52. *Surprise*: ordinary.

**10. A log of past invaders, newest first (CRISPR).** *What*: bacteria store short pieces of
invaders' DNA in an array, inserting new ones at the leader end; newer spacers are expressed
more and protect better; old ones drift away. *Source*: review of polarised spacer
acquisition ([PMC8047081](https://pmc.ncbi.nlm.nih.gov/articles/PMC8047081)). *Bits*: each
motor group keeps a short ring of hashes of the contexts that preceded bad; a new one goes at
the front; a match vetoes the same answer, with the front of the ring vetoing hardest.
*Aim*: lock, one-shot avoidance. *Cost*: fits. *Here*: idea 8 is undo; nothing like a log.
*Surprise*: genuinely different; memory as an ordered list of failures.

## B. Development and bodies

**11. Positional information.** *What*: cells read the level of a gradient from a source and
choose a fate by thresholds on it (the French flag). *Source*: Wolpert 1969;
[French flag model](https://en.wikipedia.org/wiki/French_flag_model). *Bits*: a few gradient
counters spread from the edges at birth give each unit coordinates; one rule everywhere reads
the coordinates to pick its parameters (threshold, delay, region role). *Aim*: lets idea 112
("different regions, different rules") be one rule. *Cost*: fits. *Here*: idea 112.
*Surprise*: a twist.

**12. Gradients that scale with size.** *What*: a pattern keeps its proportions when the
tissue grows, because a diffusible "expander" widens the gradient and is itself repressed by
the morphogen: an integral controller. *Source*: Ben-Zvi and Barkai 2010, PNAS 107
([record](https://cris.huji.ac.il/en/publications/scaling-of-morphogen-gradients/)). *Bits*: a
global expander count rises while the far edge sees no signal and falls when it does; the
decay length of every gradient (or every pooling radius) is set by it, so the pattern
fills whatever size is there. *Aim*: carry (scale). *Cost*: fits. *Here*: idea 104 is the
log-polar route. *Surprise*: genuinely different; scale invariance by feedback, not by a map.

**13. Local activation, long-range inhibition.** *What*: a self-enhancing activator with a
faster-spreading inhibitor makes spots and stripes at a fixed spacing from noise. *Source*:
Gierer and Meinhardt 1972, Kybernetik 12
([MPI page](https://bio.mpg.de/255219/gierer-and-meinhardt-1972)). *Bits*: units excite
within radius r1 and inhibit within r2 > r1; detectors end up spread evenly and refill a gap
after loss. *Aim*: alive, room. *Cost*: fits. *Here*: the sheet's inhibition; idea 75.
*Surprise*: ordinary.

**14. One-bit leader election (Notch-Delta).** *What*: neighbouring cells inhibit each other's
ability to signal, so small differences grow and one cell per cluster becomes the precursor;
the fly does this with one-bit messages and rising firing probability, which solved the
distributed maximal-independent-set problem. *Source*: Collier et al. 1996, J Theor Biol
([record](https://ora.ox.ac.uk/objects/uuid:d18e2570-90fd-47d5-94ad-3a4df1c0fe4c)); Afek et al.
2011, Science ([pdf](https://web.math.princeton.edu/~nalon/PDFS/misbios.pdf)). *Bits*: when a
new cue needs a unit, candidates in each patch raise a "mine" bit with a probability that
doubles each round; hearing a neighbour's bit drops you out. One committed unit per patch, no
collisions. *Aim*: many (allocating fresh units). *Cost*: fits. *Here*: idea 75, cat 72.
*Surprise*: a twist; allocation, not activity competition.

**15. Fate switches and canalisation.** *What*: two mutually repressing genes make a latch
with hysteresis that a pulse can flip; development runs down valleys (canalisation), so small
disturbances do not change the outcome. *Source*: Gardner, Cantor, Collins 2000, Nature 403
([doi](https://doi.org/10.1038/35002131));
[canalisation](https://en.wikipedia.org/wiki/Canalisation_(genetics)). *Bits*: two counters
per unit that subtract from each other; a pulse flips the winner and it stays. *Aim*: gap.
*Cost*: fits. *Here*: idea 71; cat 81. *Surprise*: ordinary.

**16. Clock and wavefront.** *What*: cells in the growing tail oscillate; a slowly moving
front freezes each cell's phase as it passes, which turns the clock's time into a repeating
pattern in space (the vertebrae). *Source*: Cooke and Zeeman 1976;
[overview](https://en.wikipedia.org/wiki/Clock_and_wavefront_model); tutorial review
([arXiv 2403.00457](https://arxiv.org/abs/2403.00457)). *Bits*: every unit in a strip keeps a
phase counter mod P; a front moves one column per k ticks; where it passes, the unit latches
its phase plus whatever input is present. The strip then holds a record of the last few
hundred ticks laid out in space, readable by the readout. *Aim*: gap. *Cost*: fits (a counter
and a latch per unit). *Here*: not present. *Surprise*: genuinely different; time stored as
place.

**17. Overproduce, then keep what earns a share (neurotrophic competition).** *What*:
developing neurons are made in excess and compete for a survival factor that their targets
make in limited amounts; those that get too little die. *Source*: reviews of the neurotrophic
hypothesis ([PMC156751](https://pmc.ncbi.nlm.nih.gov/articles/PMC156751/)). *Bits*: on good,
each motor cell hands out a fixed number of tokens to the units that drove it, each of those
passes part of its take to its own drivers, and so on for a few hops; a unit that collects no
tokens over a window is reborn at random. Credit flows backward as a conserved quantity.
*Aim*: inside. *Cost*: fits (a few hops over active edges, on outcomes only). *Here*: ideas 25,
62, 92. *Surprise*: a twist; credit conserved and spent, so it cannot be claimed by everyone.

**18. Guidance by matching gradients, then refinement by activity.** *What*: axons find
roughly the right place by matching gradient labels (chemoaffinity), then activity sharpens
the map. *Source*:
[chemoaffinity hypothesis](https://en.wikipedia.org/wiki/Chemoaffinity_hypothesis). *Bits*:
far links at birth go where the target's coordinate code matches the source's (entry 11), so
neighbours send to neighbours; co-activity wiring refines. *Aim*: carry. *Cost*: fits.
*Here*: idea 83. *Surprise*: ordinary.

**19. Flux reinforcement (leaf veins, slime-mould tubes).** *What*: in a leaf, cells that pass
auxin on get better at passing it, so diffuse flow narrows into veins; in Physarum, tubes with
more flow thicken and idle ones vanish, giving networks as efficient as the Tokyo rail system.
*Source*: Tero et al. 2010, Science 327
([pdf](https://pdodds.w3.uvm.edu/files/papers/others/2010/tero2010a.pdf)); Sachs and Mitchison
on canalisation ([review](https://www.algorithmicbotany.org/papers/canalization.pj2005.pdf)).
*Bits*: each wire has a capacity counter raised when a spike crosses it and the target passes
a spike on within its delay (flow, not co-firing), and lowered slowly otherwise; between a cue
and a rewarded button, used channels deepen. *Aim*: carry, reach. *Cost*: fits. *Here*: idea
14 (highways at birth), 62. *Surprise*: a twist; it rewards relaying, not firing together.

**20. Put the new one where inhibition is least (phyllotaxis).** *What*: each new leaf forms
where the inhibitory field of the existing ones is weakest, which yields the spiral packing.
*Source*: Reinhardt et al. 2003 and the inhibitory-field models
([Smith et al. 2006](https://www.algorithmicbotany.org/papers/phyllo.pnas2006.pdf)). *Bits*:
when a new detector or memory slot is made, choose its code (mask) by sampling a few candidates
and taking the one with the least overlap with existing ones. *Aim*: many, room. *Cost*: fits.
*Here*: cat 112 (ART) is near. *Surprise*: a twist.

**21. Units that move instead of rewiring (cell sorting).** *What*: mixed cells sort into
domains because each tends to stay touching cells it adheres to best. *Source*:
[differential adhesion](https://en.wikipedia.org/wiki/Differential_adhesion_hypothesis).
*Bits*: neighbouring units swap places when that increases how many alike neighbours each has
(alike = overlapping masks); similar detectors cluster, so a pooling cell over a patch pools
one kind. *Aim*: carry (pooling, idea 103). *Cost*: awkward (wiring that names positions has
to be rewritten on a swap; cheap if wiring is relative and the content moves). *Here*: not
present. *Surprise*: genuinely different.

**22. An uncommitted reserve (stem cells).** *What*: stem cells divide asymmetrically: one
daughter commits, the other stays a stem cell, so there is always capacity left. *Source*:
[asymmetric cell division](https://en.wikipedia.org/wiki/Asymmetric_cell_division). *Bits*:
keep a pool of units with no committed wiring and high plasticity; when a region needs a new
unit, a pool unit commits and the pool is topped up from the least used committed units.
*Aim*: many, second task. *Cost*: fits. *Here*: cat 51, idea 95. *Surprise*: a twist.

**23. Repair to a stored target.** *What*: planarian fragments regrow the body plan stored in
a bioelectric pattern; rewriting the pattern makes two-headed worms that stay two-headed
through further cuts. *Source*: Levin lab review
([PMC6234102](https://pmc.ncbi.nlm.nih.gov/articles/PMC6234102)). *Bits*: a slow, coarse layer
holds a target of what each region should be doing (its rate, its role bits); after damage,
units read the target and regrow toward it. *Aim*: damage (a map hole). *Cost*: fits. *Here*:
not present. *Surprise*: genuinely different, with a narrow use.

**24. Growth by parallel local rewriting (L-systems).** *What*: plant form grows by applying
string-rewriting rules to every module at once; context-sensitive rules let signals travel
along the plant. *Source*: Prusinkiewicz and Lindenmayer, The Algorithmic Beauty of Plants,
ch. 1 (owner's library; [online](http://algorithmicbotany.org/papers/abop/abop.pdf)). *Bits*: a
design is a small grammar that grows the wiring at birth; the search (idea 98) changes rules,
not wires. *Aim*: the search. *Cost*: fits. *Here*: cat 139 (indirect encodings).
*Surprise*: ordinary.

**25. Learning guides evolution (the Baldwin effect).** *What*: in Hinton and Nowlan's model
a genome of 0, 1 and "learn" bits finds a needle-in-a-haystack target only because lifetime
learning makes near misses partly fit; over generations, learned bits become fixed.
*Source*: Hinton and Nowlan 1987, Complex Systems 1
([page](https://www.cs.toronto.edu/~hinton/absps/evolution.htm)). *Bits*: score each genome
in the design search by how fast it learns, not how well it is born, and fix what is always
learned. *Aim*: the search (ideas 98-99). *Cost*: fits. *Here*: not stated. *Surprise*: a
twist for the search, not for the engine.

## C. Gene regulation and cells

**26. Random Boolean networks at the edge.** *What*: networks of genes each a random Boolean
function of K others settle into few attractors (cell types); at K = 2 they are critical,
stable yet responsive. *Source*: Kauffman 1969, J Theor Biol 22
([doi](https://doi.org/10.1016/0022-5193(69)90015-0));
[Boolean network](https://en.wikipedia.org/wiki/Boolean_network). *Bits*: table neurons with
two inputs each, randomly wired, as a time-holding reservoir; tune K and bias to the critical
line and screen with the richness probe. *Aim*: gap. *Cost*: fits. *Here*: idea 26; cat 121.
*Surprise*: ordinary.

**27. A persistence detector (coherent feed-forward loop).** *What*: X turns on Y and both
are needed to turn on Z, so Z ignores brief pulses of X and turns off at once: a
sign-sensitive delay. *Source*: Mangan and Alon 2003, PNAS
([record](https://cris.haifa.ac.il/en/publications/structure-and-function-of-the-feed-forward-loop-network-motif/)).
*Bits*: Z = X now AND X d ticks ago (through Y); chains of these with different d measure how
long something has lasted. *Aim*: gap (gap length). *Cost*: fits. *Here*: idea 74. *Surprise*:
ordinary.

**28. Fold-change detection (incoherent feed-forward loop).** *What*: X turns on Z and also
a slow repressor of Z, so Z responds to relative change in X, not its level. *Source*:
Goentoro et al. 2009, Mol Cell 36
([PMC2896310](https://pmc.ncbi.nlm.nih.gov/articles/PMC2896310)). *Bits*: a unit excited by a
line and inhibited by a slowly averaged copy of the same line fires on change in proportion
to the old level. *Aim*: carry (intensity, thickness). *Cost*: fits. *Here*: idea 108, cat 73.
*Surprise*: ordinary.

**29. Perfect adaptation by integral feedback.** *What*: bacterial chemotaxis returns to the
same tumble rate after any step in attractant because receptor methylation integrates the
error; that is integral control. *Source*: Yi, Huang, Simon, Doyle 2000, PNAS
([PMC18287](https://pmc.ncbi.nlm.nih.gov/articles/PMC18287)). *Bits*: each unit's threshold is
a counter that integrates (fired minus target). *Aim*: alive. *Cost*: fits. *Here*: finding 2
already does this. *Surprise*: ordinary; it confirms the design.

**30. Antithetic integral feedback.** *What*: two molecules made at rates set by the reference
and by the output annihilate each other; this gives exact set points even with few molecules
and heavy noise. *Source*: Briat, Gupta, Khammash 2016, Cell Systems
([arXiv 1410.6064](https://arxiv.org/abs/1410.6064v5)). *Bits*: two small saturating
counters per unit, one up on each target tick, one up on each spike; whenever both are
non-zero, decrement both; the threshold follows their difference. *Aim*: alive with tiny
counters. *Cost*: fits. *Here*: finding 2. *Surprise*: a twist; useful if 2-bit counters
drift.

**31. Run and tumble.** *What*: E. coli swims straight while things get better and tumbles to
a random new direction when they get worse, comparing now with a few seconds ago; that alone
climbs gradients. *Source*: Yi et al. 2000 (entry 29); Berg's work summarised there. *Bits*:
the motor group repeats its last action while a fast average of reward exceeds a slow one,
and switches to a random action with a probability that rises as the fast average falls
below the slow one. *Aim*: acting (track, forage, catch). *Cost*: fits. *Here*: idea 53
reacts to one bad; idea 60. *Surprise*: a twist; it compares trends, not single outcomes.

**32. Marks that keep themselves (epigenetic memory).** *What*: modified nucleosomes recruit
enzymes that modify other nucleosomes, including distant ones; with cooperativity this gives a
bistable state that survives noise and being split at cell division. *Source*: Dodd et al.
2007, Cell 129 ([PMC record](https://pmc.ncbi.nlm.nih.gov/articles/PMC2387233)). *Bits*: a unit
holds a row of mark bits; each step one random bit is set to the state of another random
bit (recruitment) with a small bias from the unit's recent use, plus noise; the majority is a
slow memory that survives copying (entries 1, 42). *Aim*: gap, keeping. *Cost*: fits. *Here*:
cat 12-13 (hidden synapse states). *Surprise*: a twist; memory kept by mutual copying, not by
a stored number.

**33. Kinetic proofreading.** *What*: an enzyme or T-cell receptor must pass several
energy-driven intermediate steps, any of which it can fall back out of, before it commits; the
error rate becomes the single-step error to the power of the number of steps. *Source*:
Hopfield 1974, PNAS ([PMC434344](https://pmc.ncbi.nlm.nih.gov/articles/PMC434344)); McKeithan
1995 for T cells ([PMC41844](https://pmc.ncbi.nlm.nih.gov/articles/PMC41844)). *Bits*: a
candidate wire carries a small step counter; each outcome that supports it moves it up one,
any outcome against it resets it to zero (or with a probability), and it is wired only at
step k. Unlike an evidence sum, one contradiction undoes it. *Aim*: inside, lock (no
commitment on lucky streaks). *Cost*: fits. *Here*: idea 59 is the additive cousin.
*Surprise*: genuinely different in shape.

**34. Bet hedging by random switching.** *What*: clonal bacteria switch phenotype at random;
when the environment changes rarely, random switching beats sensing, and the best switching
rates match the rates of environmental change. *Source*: Kussell and Leibler 2005, Science 309
([pdf](https://mimuw.edu.pl/~miekisz/leiblerfluct.pdf)). *Bits*: each unit (or engine) keeps
two settings, an active and a spare; it switches at a small rate; the rate itself moves
toward the observed rate of rule changes (how often reward flipped meaning). *Aim*: lock,
changed rule. *Cost*: fits. *Here*: not present. *Surprise*: a twist; deliberate minority
behaviour as insurance.

**35. Quorum sensing.** *What*: each bacterium releases a signal; when its concentration says
"many of us", all switch behaviour together. *Source*: Miller and Bassler 2001, Annu Rev
Microbiol 55 ([doi](https://doi.org/10.1146/annurev.micro.55.1.165)). *Bits*: units add to a
shared count when in some state; crossing a threshold switches a mode for all (learning on,
plasticity up, commit). *Aim*: coordinating phases (idea 85). *Cost*: fits. *Here*: idea 85.
*Surprise*: ordinary.

**36. Time-sharing by electrical waves (biofilms).** *What*: starving inner cells of a biofilm
release potassium; a relayed wave reaches the edge and makes outer cells pause eating, so the
colony takes turns. *Source*: Prindle et al. 2015, Nature 527
([PMC4890463](https://pmc.ncbi.nlm.nih.gov/articles/PMC4890463)). *Bits*: units under a
resource shortfall (too little reward, too few spikes) emit a relayed wave that lowers the
gain of distant units for a while; long-range negative feedback through an excitable relay.
*Aim*: alive, carry. *Cost*: fits. *Here*: not present. *Surprise*: a twist.

**37. Control shared out, summing to one (metabolic control).** *What*: control over a
pathway's flux is spread over all its enzymes, and the control coefficients sum to one; no
single step is "the" rate limit. *Source*:
[metabolic control analysis](https://en.wikipedia.org/wiki/Metabolic_control_analysis).
*Bits*: on each outcome, a fixed amount of credit is divided among contributing units in
proportion to an estimate of how much each changed the outcome; the shares must sum to the
outcome. *Aim*: inside (credit that cannot be over-claimed). *Cost*: fits. *Here*: finding 13 is
the slip that this rule would forbid. *Surprise*: a twist; pairs with entries 17 and 63.

## D. Evolution and ecology

**38. Plain selection.** *What*: variation plus differential copying. *Source*: Kennedy et
al., Swarm Intelligence, ch. 4 (owner's library). *Bits*: ideas 30-33. *Aim*: search. *Cost*:
fits. *Here*: ideas 30-33. *Surprise*: ordinary. Listed only to mark the ground.

**39. Sex as multiplicative weights (mixability).** *What*: under weak selection, gene
frequencies at each locus evolve exactly as the multiplicative-weights learning rule in a
game between loci; selection then favours alleles that do well across many combinations, not
the single best combination. *Source*: Chastain, Livnat, Papadimitriou, Vazirani 2014, PNAS
([PMC4115542](https://pmc.ncbi.nlm.nih.gov/articles/PMC4115542)). *Bits*: each unit keeps a
weight per candidate input; each trial it samples its inputs from those weights independently
of the other units (recombination); after the outcome each sampled weight is multiplied up
or down by the result. Units learn what works in many contexts. *Aim*: inside, carry. *Cost*:
fits. *Here*: not present (idea 59 is the additive version without sampling). *Surprise*: a
twist; a population-genetics engine inside one sheet.

**40. Neutral networks.** *What*: many sequences fold to the same shape, connected by single
mutations; populations drift along these at no cost and reach new shapes from anywhere.
*Source*: Schuster et al. 1994, Proc R Soc B
([preprint](https://sites.santafe.edu/~walter/Papers/shapes.html)). *Bits*: changes that do
not alter the output are accepted freely (wires not on any path to the motor cells drift at
random); score plateaus are walked, not frozen. *Aim*: lock (a rule graded against its average
freezes on a plateau: findings, "Locking on"). *Cost*: fits. *Here*: not present. *Surprise*:
genuinely different for this project, which freezes when nothing changes.

**41. The error threshold.** *What*: information survives selection only while the mutation
rate per genome stays below about the log of the fitness advantage; above it the population
melts into a cloud ("survival of the flattest"). *Source*: Eigen 1971;
[error threshold](https://en.wikipedia.org/wiki/Error_threshold_(evolution)). *Bits*: a design
rule: changes per unit per selection window, times the bits that matter, must stay under one.
Set noise and plasticity from it. *Aim*: keeping. *Cost*: fits. *Here*: finding 4 is the case
with no selection. *Surprise*: a twist; a rule for setting rates.

**42. Gene duplication and divergence.** *What*: a copied gene can drift into a new job
while the original keeps the old one; much new function came this way. *Source*:
[gene duplication](https://en.wikipedia.org/wiki/Gene_duplication) (Ohno 1970). *Bits*: when a
unit with high evidence keeps getting credit pulling in two directions (good with input set A
for one answer, B for another), copy it into a free unit; the original freezes its current
wiring, the copy takes the new demand. *Aim*: many, second task without forgetting. *Cost*:
fits. *Here*: not present. *Surprise*: genuinely different here.

**43. Horizontal transfer.** *What*: bacteria hand useful genes (antibiotic resistance) to
unrelated strains on plasmids. *Source*:
[horizontal gene transfer](https://en.wikipedia.org/wiki/Horizontal_gene_transfer). *Bits*: a
tile that did well posts a compact fragment (a mask in relative offsets) on a board; another
tile may splice it in and keeps it only if its own score does not fall. *Aim*: carry. *Cost*:
fits. *Here*: ideas 30, 100, 125. *Surprise*: a twist; transfer by trial, not by tying weights.

**44. Symbiotic encapsulation.** *What*: two partners that do well together become one
heritable unit (mitochondria); in Watson and Pollack's model this composes solutions that
mutation and sex cannot reach. *Source*: Watson's thesis, "Compositional Evolution"
([pdf](https://eprints.soton.ac.uk/262006/1/watson_thesis_2002.pdf)). *Bits*: when units A and
B co-fire before good far more than chance, create unit C = A AND B and treat it as a new
primitive with its own wiring. *Aim*: combination tier, parts and wholes. *Cost*: fits. *Here*:
idea 27 (all-of neurons) at birth. *Surprise*: a twist; new units made from proven pairs.

**45. Selfish parts that build an associative memory.** *What*: when each node of a network
changes its own connections to satisfy itself, over many restarts from random states the
changes amount to Hebbian learning and the network builds a memory of its good configurations,
which then improves the whole. *Source*: Watson, Mills, Buckley 2011, Artificial Life 17
([record](https://eprints.soton.ac.uk/271443)). *Bits*: a constraint network (units with
pairwise wants) is repeatedly reset at random and relaxed; after each settle, each unit
strengthens slightly its wires to neighbours it agrees with. *Aim*: search, many. *Cost*: fits.
*Here*: cat 128 (attractor memories). *Surprise*: a twist; learning that improves the
system's own search.

**46. Parasites that coevolve with the learner.** *What*: Hillis evolved sorting networks
against a population of test cases that evolved to break them, and got better networks
faster. *Source*: Hillis 1990, Physica D 42
([pdf](https://www.cse.unr.edu/~sushil/class/gas/papers/DannyHillisCoevolution.pdf)). *Bits*:
a world generator that keeps cases the design gets wrong and mutates them. *Aim*: the method.
*Cost*: fits. *Here*: ideas 126-127. *Surprise*: ordinary (hard-example mining in ML).

**47. Kill the winner.** *What*: viruses infect the most active bacterial type hardest, so
no type can take over and diversity stays high. *Source*: Winter, Bouvier, Weinbauer,
Thingstad 2010, MMBR ([PMC2832346](https://pmc.ncbi.nlm.nih.gov/articles/PMC2832346)). *Bits*:
each answer (or each strongly wired unit) has a "predator" counter that grows with how often
it wins and decays slowly; it subtracts from that answer's drive. A rewarded winner outruns
its predator; an unrewarded winner is pulled down whatever the punishments say. *Aim*: lock.
*Cost*: fits. *Here*: idea 53 reacts to bad, this to frequency. *Surprise*: a twist.

**48. Fitness sharing and niches.** *What*: if individuals that are alike split their payoff,
the population spreads over many niches instead of piling on one peak. *Source*: Sareni and
Krähenbühl 1998, IEEE TEC ([doi](https://doi.org/10.1109/4235.735432)). *Bits*: credit given
to a unit is divided by the number of units with similar masks (popcount of XOR under a
radius), so duplicates earn little and units spread over different features. *Aim*: many,
room. *Cost*: awkward (similarity counts; hash buckets help). *Here*: not present. *Surprise*:
ordinary in evolutionary computation, new here.

**49. Goals that vary in modules, and a price on wire.** *What*: networks evolved under goals
that switch between combinations of the same sub-goals become modular; a cost per connection
does the same. *Source*: Kashtan and Alon 2005, PNAS
([PMC record](https://pmc.ncbi.nlm.nih.gov/articles/PMC2605507)); Clune, Mouret, Lipson 2013
([doi](https://doi.org/10.1098/rspb.2012.2863)). *Bits*: the task stream for a life switches
among worlds that share parts (same cues, different combinations); a wire's budget cost grows
with its length. *Aim*: carry (reuse). *Cost*: fits. *Here*: not present (finding 1 is a flat
cost). *Surprise*: a twist; it says something about the curriculum.

**50. Stepping stones (Avida) and self-replicators (Tierra).** *What*: digital organisms
evolved the EQU logic function only when simpler functions were also rewarded, and some
harmful mutations were steps on the way; in Tierra, parasites and shorter code emerged by
themselves. *Source*: Lenski et al. 2003, Nature 423
([pdf](https://www.cs.princeton.edu/courses/archive/fall24/cos597C/bib/598-2017/adami03Nature.pdf));
Koza, Genetic Programming, ch. 28 (owner's library). *Bits*: worlds give partial credit for
sub-functions (each input pair right earns something). *Aim*: the method. *Cost*: fits. *Here*:
not stated. *Surprise*: ordinary.

**51. The one-fifth rule.** *What*: evolution strategies tune their own mutation size: if
more than one in five tries improves, step bigger, else smaller. *Source*:
[evolution strategy](https://en.wikipedia.org/wiki/Evolution_strategy) (Rechenberg). *Bits*:
each unit tracks the share of its recent changes that were followed by good; above a fifth
it changes more per outcome, below it changes less. *Aim*: lock, inside. *Cost*: fits. *Here*:
not present (cat 106 is the strategy itself). *Surprise*: a twist; per-unit self-tuned
plasticity from success alone.

**52. Costly signals.** *What*: signals stay honest when sending them costs the sender more
than a cheat could afford (the handicap principle). *Source*:
[handicap principle](https://en.wikipedia.org/wiki/Handicap_principle). *Bits*: a unit's vote
to the readout costs it budget (connections, or wealth), so only units with strong evidence
vote often. *Aim*: inside (thin signal). *Cost*: fits. *Here*: not present. *Surprise*: a
twist.

**53. Novelty search and quality-diversity.** *What*: searching for new behaviour rather than
for the goal avoids deception; MAP-Elites keeps the best of each behaviour type. *Source*:
Mouret and Clune ([arXiv 1504.04909](https://arxiv.org/abs/1504.04909)). *Bits*: the design
search keeps an archive indexed by behaviour (which worlds passed). *Aim*: the search. *Cost*:
fits. *Here*: idea 99 near. *Surprise*: ordinary (known evolutionary computation).

## E. Collectives

**54. Pheromone trails.** *What*: ants lay trail pheromone that evaporates; shorter paths are
reinforced faster and win. *Source*: Goss et al. 1989
([doi](https://link.springer.com/article/10.1007/BF00462870));
[ant colony optimisation](http://www.scholarpedia.org/article/Ant_colony_optimization). *Bits*:
each wire has a trail counter; on good, the chain of wires that carried the recent spikes to
the pressed button gets a deposit; all trails evaporate. *Aim*: carry, inside. *Cost*: fits.
*Here*: idea 62. *Surprise*: ordinary (known as ACO).

**55. Stigmergy.** *What*: termites and the TERMES robots coordinate by changing the shared
environment and reacting to the changes, with no messages. *Source*: Werfel, Petersen, Nagpal
2014, Science 343
([page](https://del.seas.harvard.edu/publication/designing-collective-behavior-termite-inspired-robot-construction-team));
[stigmergy](https://en.wikipedia.org/wiki/Stigmergy). *Bits*: a scratch grid of bits beside the
sheet; units may set or clear the cell under them and read nearby cells as input; marks decay
slowly. The sheet can then leave itself a note and read it later. *Aim*: gap, acting. *Cost*:
fits. *Here*: not present. *Surprise*: genuinely different.

**56. Recruitment, stop signals and quorum (honeybees).** *What*: scouts dance for sites in
proportion to quality, scouts for one site head-butt dancers for another (stop signals), and
the swarm commits when one site has a quorum; cross-inhibition breaks deadlocks between equal
sites. *Source*: Seeley et al. 2012, Science 335
([pdf](https://rangevoting.org/SeeleyVSHFMinhibit.pdf)). *Bits*: at the readout, each button
group recruits idle motor cells at a rate set by its evidence; committed cells send stop bits
to cells of other groups; a press happens when one group reaches a quorum count. Quorum size
is the speed-accuracy dial. *Aim*: readout decisions, lock. *Cost*: fits. *Here*: idea 75 and
cat 72 are plain winner-take-all. *Surprise*: a twist.

**57. Response thresholds and division of labour.** *What*: each insect has a threshold per
task; doing a task lowers its threshold for it, idleness raises it; specialists emerge from
identical workers, and others step in when specialists are removed. *Source*: Theraulaz,
Bonabeau, Deneubourg 1998, Proc R Soc B
([SFI](https://www.santafe.edu/research/results/working-papers/response-threshold-reinforcement-and-division-of-l)).
*Bits*: each unit keeps a threshold per input group (or per world); firing for a group lowers
that threshold, idleness raises it. *Aim*: many, damage. *Cost*: fits. *Here*: cat 18 is one
threshold per neuron. *Surprise*: a twist.

**58. Topological neighbours.** *What*: starlings align with their six or seven nearest
neighbours whatever the distance, which keeps the flock together through changes in density.
*Source*: Ballerini et al. 2008, PNAS ([arXiv 0709.1916](https://arxiv.org/pdf/0709.1916)).
*Bits*: a unit's inputs are the k most recently co-active units, re-chosen as density changes,
instead of all units in a radius. *Aim*: alive. *Cost*: awkward (finding the k nearest).
*Here*: not present. *Surprise*: a twist.

**59. Emergent sensing.** *What*: single golden shiners cannot track a moving shade gradient;
groups can, because each fish only slows in the dark and social following does the rest.
*Source*: Berdahl et al. 2013, Science 339 ([record](https://eprints.gla.ac.uk/133188)).
*Bits*: each unit senses one local scalar (its own recent credit) and only changes its speed
(firing or plasticity rate); coupling to neighbours turns many scalars into a group gradient
no unit could compute. *Aim*: inside (thin signal). *Cost*: fits. *Here*: not present.
*Surprise*: genuinely different.

**60. A few who know lead many who do not.** *What*: in moving groups a small informed
minority steers the whole without anyone knowing who is informed; the larger the group, the
smaller the fraction needed (about 5% of 200). *Source*: Couzin, Krause, Franks, Levin 2005,
Nature 433 ([pdf](https://www.cs.princeton.edu/courses/archive/fall24/cos597C/bib/nature03236.pdf)).
*Bits*: a region of units couples strongly to its neighbours' state; the cue reaches only a
few units; the region's consensus moves toward the cue. A cue can steer a large uninformed
patch instead of being relayed unit by unit. *Aim*: carry. *Cost*: fits. *Here*: not present.
*Surprise*: genuinely different.

**61. Group choice by staying where many are (cockroaches).** *What*: each cockroach leaves a
shelter with a probability that falls with how many are there; the group splits between
shelters in proportion to capacity. *Source*: Amé et al. 2006, PNAS
([PMC1458659](https://pmc.ncbi.nlm.nih.gov/articles/PMC1458659)). *Bits*: units join groups
(answers, or cues to serve); leaving probability falls with group size, up to a cap; units
spread over tasks in proportion to need. *Aim*: many. *Cost*: fits. *Here*: not present.
*Surprise*: a twist.

**62. The bucket brigade (classifier systems).** *What*: rules bid to post messages; a winning
rule pays its bid to the rules whose messages it used; the world pays the last rule; strength
flows backward along chains that lead to payoff. XCS later used accuracy as fitness. *Source*:
Holland 1985, "Properties of the bucket brigade"
([pdf](https://www.gwern.net/doc/reinforcement-learning/multi-agent/1985-holland.pdf));
Urbanowicz and Moore, history of LCS ([arXiv 1401.3607](https://arxiv.org/pdf/1401.3607)).
*Bits*: each unit has a wallet counter; when it fires because of its inputs, it pays a fixed
fraction of its wallet, split among the units that drove it; the world pays the motor cells;
broke units are reborn. *Aim*: inside, acting. *Cost*: fits (one pass over active edges at
firing, integer adds). *Here*: ideas 62, 92. *Surprise*: genuinely different for the
project; a known machine-learning idea (learning classifier systems) outside the catalogue.

**63. Hayek: an economy with property rights.** *What*: Baum's agents bid for ownership of the
whole world in auctions; the winner pays the previous owner, acts, takes the world's reward
and is paid by the next owner; money is conserved, rich agents spawn mutated children and
take a tenth of their profit, poor ones die. It learned 200-block Blocks World where
alternatives managed 8, and it failed when conservation or ownership was broken. *Source*:
Baum and Durdanovic 2000, Neural Computation 12
([pdf](https://gwern.net/doc/reinforcement-learning/model-free/2000-baum.pdf), read: section 2).
*Bits*: see section 2, "A market". *Aim*: inside, acting. *Cost*: fits. *Here*: not present.
*Surprise*: genuinely different; its diagnosis of shared credit applies to the sheet as it is.

**64. Market scoring rules.** *What*: a prediction market run by an automated market maker
with a logarithmic scoring rule; each trader is paid by how much they improved the shared
forecast. *Source*: Hanson 2003 and 2007
([pdf](https://healy.econ.ohio-state.edu/pubfiles/infomkts/litreview/Hanson%20-%20Combinatorial%20Information%20Market%20Design%2003.pdf)).
*Bits*: a shared forecast counter per button; a unit that fires nudges the forecast toward
its preferred answer; when the outcome comes, each unit is paid the change in log score its
nudge caused (a table lookup). Inner units are paid for information, not for being near the
end. *Aim*: inside. *Cost*: fits. *Here*: idea 90 (value cell). *Surprise*: a twist.

**65. Money is memory.** *What*: anything money can achieve, a record of everyone's history
could too; money is a cheap stand-in for that memory. *Source*: Kocherlakota 1998, J Econ
Theory 81 ([record](https://ideas.repec.org/a/eee/jetheo/v81y1998i2p232-251.html)). *Bits*:
instead of eligibility traces on every wire, a token travels with activity and carries the
history of contribution; credit is settled by what the token holder holds. *Aim*: gap
(credit across time). *Cost*: fits. *Here*: idea 52 is the trace route. *Surprise*: a twist;
it frames 62-63 as memory.

**66. Juries and weighted majority.** *What*: a majority of independent voters each a little
better than chance is very accurate, and correlation destroys that; weighting voters down
multiplicatively when wrong tracks the best expert. *Source*:
[Condorcet's jury theorem](https://en.wikipedia.org/wiki/Condorcet%27s_jury_theorem);
Littlestone and Warmuth 1994 ([doi](https://doi.org/10.1006/inco.1994.1009)). *Bits*: give
each motor cell a different random subset of the sheet (decorrelation) and halve the weight of
cells that voted wrong. *Aim*: readout. *Cost*: fits. *Here*: finding 16. *Surprise*:
ordinary (known machine learning).

**67. Virtual points (the minority game).** *What*: each agent holds a few fixed strategies
(tables from the recent history to a choice) and every round scores all of them by what they
would have won, then plays the current best; the population self-organises. *Source*: Challet
and Zhang 1997, Physica A ([arXiv adap-org/9708006](https://arxiv.org/abs/adap-org/9708006));
Arthur's El Farol problem in Kennedy et al., ch. 5 (owner's library). *Bits*: each motor cell
(or unit) keeps S small tables over its inputs; after each outcome, every table that would
have given the right answer gets a point, whether or not it was used; the cell plays its
top-scoring table. *Aim*: inside, lock (switching between candidates is built in). *Cost*:
fits (S lookups per outcome). *Here*: idea 33 is the two-variant, try-it-and-see version.
*Surprise*: genuinely different here; counterfactual credit is free whenever the world says
what the right answer was.

**68. Copy by similarity (Axelrod's culture model).** *What*: neighbours interact with
probability equal to how many traits they share and then one copies a differing trait;
local agreement and global diversity both persist. *Source*: Kennedy et al., Swarm
Intelligence, ch. 5-6 (owner's library); Axelrod 1997
([doi](https://doi.org/10.1177/0022002797041002001)). *Bits*: two neighbouring tiles compare
their masks in relative coordinates; with probability equal to their overlap, one copies a
differing bit from the other. Features learned in one tile spread to similar tiles, not to
all. *Aim*: carry. *Cost*: fits. *Here*: idea 100 leaks evidence to all places. *Surprise*: a
twist; transfer gated by similarity.

**69. Copy the successful.** *What*: in a tournament of social learning strategies, the
winner copied almost exclusively, weighting by recency, because others only show their best;
individual learning pays when the world is stable and sampled well. *Source*: Rendell et al.
2010, Science 328 ([PMC2989663](https://pmc.ncbi.nlm.nih.gov/articles/PMC2989663)); Boyd and
Richerson in Kennedy et al., ch. 5. *Bits*: a tile doing poorly copies the mask of a
neighbour that recently got credit, with small copy errors. *Aim*: carry, speed. *Cost*: fits.
*Here*: idea 30. *Surprise*: ordinary.

**70. Particle swarms.** *What*: each particle moves toward its own best position and its
neighbours' best. *Source*: Kennedy et al., Swarm Intelligence, ch. 7 (owner's library);
[particle swarm optimisation](https://en.wikipedia.org/wiki/Particle_swarm_optimization).
*Bits*: each tile keeps its best-so-far mask and pulls bits of its current mask toward it and
toward its neighbours' best, with probabilities. *Aim*: search. *Cost*: fits. *Here*: idea 32.
*Surprise*: ordinary (a known optimiser).

**71. Knowledge in the mix of types.** *What*: in Hofstadter's Ant Fugue the colony's
knowledge is in the proportions of its castes and in "signals", teams that form, move and
dissolve where local needs differ, not in any ant. *Source*: Hofstadter, Gödel, Escher, Bach,
"Ant Fugue" (owner's library). *Bits*: store a region's state as counts of unit types (how
many units are in mode A, B, C) rather than in wiring; a transient group forms to carry
something and dissolves where the local counts match it. *Aim*: unclear. *Cost*: fits. *Here*:
not present. *Surprise*: genuinely different, but a framing more than a recipe.

**72. Gossip averaging.** *What*: epidemic protocols spread updates by random pairwise
exchange; push-sum computes global averages with no centre. *Source*: Demers et al. 1987
([pdf](https://none.cs.umass.edu/~shenoy/courses/fall04/readings/demers.pdf)). *Bits*: each
unit holds a (sum, weight) pair and sends half to a random neighbour each tick; every unit
converges to the global mean (of reward, of rate) without a global counter. *Aim*: alive,
baselines for credit (idea 5). *Cost*: fits. *Here*: idea 5. *Surprise*: ordinary.

## F. Physics and chemistry

**73. Annealing.** *What*: cool slowly and a system finds low-energy states; reheat to escape.
*Source*: Kirkpatrick, Gelatt, Vecchi 1983, Science 220
([doi](https://doi.org/10.1126/science.220.4598.671)). *Bits*: noise and plasticity fall over a
life and rise again after a change in reward statistics. *Aim*: lock. *Cost*: fits. *Here*:
idea 88. *Surprise*: ordinary. (Copycat, entry 128, ties the temperature to coherence, which is
the better version.)

**74. Spin glasses, attractors, funnels.** *What*: frustrated magnets have many metastable
states; Hopfield used this for memory; proteins fold because evolution picked sequences whose
landscapes are funnels. *Source*: Hopfield 1982, PNAS
([doi](https://doi.org/10.1073/pnas.79.8.2554));
[folding funnel](https://en.wikipedia.org/wiki/Folding_funnel). *Bits*: shape attractors so each
cue's basin is wide. *Aim*: many. *Cost*: fits. *Here*: idea 123, cat 127-128. *Surprise*:
ordinary.

**75. Sandpiles.** *What*: grains added one at a time to a pile with a toppling threshold
drive it to a critical state with avalanches of all sizes, with no tuning. *Source*: Bak, Tang,
Wiesenfeld 1987, PRL ([doi](https://doi.org/10.1103/PhysRevLett.59.381)). *Bits*: units hold a
grain count; at threshold they topple and pass grains to neighbours, conserving the total; the
sheet sits at a branching ratio of one by construction. *Aim*: alive, carry. *Cost*: fits.
*Here*: idea 12. *Surprise*: a twist; criticality by conservation instead of a regulator.

**76. Percolation.** *What*: long-range connection appears suddenly at a critical density of
links. *Source*: [percolation theory](https://en.wikipedia.org/wiki/Percolation_theory).
*Bits*: measure the density of links that actually carry spikes; set far-link density just
above the threshold for the sheet's size. *Aim*: carry. *Cost*: fits. *Here*: idea 47.
*Surprise*: ordinary; a rule for idea 47's number.

**77. Nucleation and templated growth.** *What*: a new phase forms only once a nucleus larger
than a critical size appears; smaller ones dissolve; past it, growth is fast and copies the
template. *Source*:
[classical nucleation theory](https://en.wikipedia.org/wiki/Classical_nucleation_theory).
*Bits*: a new memory (a group of wires) commits only when k co-active units agree within a
window; below k the tentative changes dissolve; once committed, neighbours that match are
recruited quickly. *Aim*: inside (no commitment on noise), room. *Cost*: fits. *Here*: not
present. *Surprise*: a twist.

**78. Hysterons and return-point memory.** *What*: a disordered magnet driven by a field
remembers its past turning points; Preisach modelled this as a bank of relays with different
up and down thresholds. *Source*: Keim et al. 2019, "Memory formation in matter", Rev Mod Phys
91 ([arXiv 1810.08587](https://arxiv.org/abs/1810.08587));
[Preisach model](https://en.wikipedia.org/wiki/Preisach_model_of_hysteresis). *Bits*: give
each unit two thresholds drawn from a wide range; a line drives many of them; the set of
latched units stores the sequence of the line's recent maxima and minima, and is read out
directly. One bit per hysteron. *Aim*: gap, magnitude. *Cost*: fits. *Here*: idea 71 has one
pair of thresholds per neuron, not a spread bank. *Surprise*: genuinely different.

**79. Several memories at once, kept by noise.** *What*: a sheared suspension trained with
several amplitudes remembers all of them for a while but only the largest in the end, unless
noise is present, which keeps several. *Source*: Keim et al. 2019 (entry 78), section on
transient memories. *Bits*: units driven cyclically settle into states that no longer change
under that drive; each drive amplitude leaves a set of such units; a little noise stops one
memory absorbing the rest. *Aim*: many. *Cost*: fits in principle. *Here*: not present.
*Surprise*: genuinely different; noise as what keeps memories apart.

**80. Teach by holding the answer (directed aging).** *What*: a mechanical network held in the
desired deformation for a while ages so that it later produces it unaided; coupled learning
compares a free and a slightly nudged state using only local strains. *Source*: Stern,
Hexner, Rocks, Liu 2021, PRX 11 ([arXiv 2011.03861](https://arxiv.org/abs/2011.03861)), which
also cites Pashine et al. 2019 on directed aging. *Bits*: when the world shows the answer
(idea 39), clamp the motor cells to it and let wires under "strain" (source active, target
held opposite) weaken and the rest firm up; no reward needed. *Aim*: being shown the answer (a
map hole). *Cost*: fits. *Here*: idea 39; cat 91 is the contrastive version. *Surprise*: a
twist.

**81. Pulse-coupled oscillators (fireflies).** *What*: identical integrate-and-fire
oscillators that kick each other's phase forward on each flash always synchronise.
*Source*: Mirollo and Strogatz 1990, SIAM J Appl Math 50
([pdf](https://www.iasi.cnr.it/~vbonifaci/semcn/Mirollo1990.pdf)); Arbib, "Collective
behavior of coupled oscillators". *Bits*: each unit a phase counter that fires at wrap and
jumps ahead on a neighbour's spike; groups lock and the lock can tag what belongs together.
*Aim*: gap, binding. *Cost*: fits. *Here*: idea 86; cat 155. *Surprise*: ordinary.

**82. Waves from a gradient of frequencies.** *What*: a chain of coupled oscillators with
slightly different natural frequencies produces a travelling wave from fast to slow, as in the
lamprey spinal cord and the gut. *Source*: Kopell and Ermentrout, "Chains of oscillators in
motor and sensory systems", in Arbib's Handbook (owner's library). *Bits*: a strip of phase
counters whose natural periods step slightly along the strip; the phase lag along it is a
standing record of elapsed time and a carrier from one end to the other. *Aim*: carry, gap.
*Cost*: fits. *Here*: not present. *Surprise*: a twist.

**83. Half-centre oscillators.** *What*: two groups that inhibit each other and tire make a
rhythm, the core of many motor pattern generators; it can be entrained by input. *Source*:
Arbib's Handbook, "Half-center oscillators underlying rhythmic movements" and "Motor pattern
generation" (owner's library). *Bits*: two small groups with mutual inhibition and fatigue
(idea 68) generate a beat whose period is set by the fatigue time; a cue resets the phase.
*Aim*: the beat world, acting (cartpole needs alternation). *Cost*: fits. *Here*: ideas 68, 70,
86 as parts. *Surprise*: ordinary, but not yet a module here.

**84. Stochastic resonance.** *What*: a threshold, a sub-threshold signal and noise: a middle
amount of noise makes the signal detectable; too little or too much hides it. *Source*: Moss,
"Stochastic resonance", in Arbib's Handbook (owner's library). *Bits*: give each unit noise
tuned to its own distance below threshold for the inputs that matter, set by maximising how
often its firing agrees with outcomes. *Aim*: carry (thin signal past five tiles). *Cost*:
fits. *Here*: idea 1, finding 6 (noise did nothing at one fixed level). *Surprise*: ordinary;
the point is that the level matters.

**85. Excitable media.** *What*: in the Belousov-Zhabotinsky reaction and heart tissue a pulse
regenerates as it travels, so waves cross any distance without fading and annihilate on
collision; reaction-diffusion computers use this for shortest paths. *Source*:
[excitable medium](https://en.wikipedia.org/wiki/Excitable_medium). *Bits*: units with a
three-state cycle (rest, fire, refractory) carry a cue as a wave across the sheet; the arrival
time at each place encodes distance. *Aim*: carry. *Cost*: fits. *Here*: the sheet is close to
this already. *Surprise*: a twist; carry by regeneration, not by wiring.

**86. Autocatalytic sets.** *What*: a set of reactions each catalysed by products of the set,
fed by a food set, maintains itself; Kauffman proposed this for the origin of life and RAF
theory formalised it. *Source*: Hordijk and Steel 2018, Life
([PMC6315399](https://pmc.ncbi.nlm.nih.gov/articles/PMC6315399)). *Bits*: groups of units
whose outputs are each other's inputs keep firing once lit; memory as a self-sustaining loop
of groups, lit by a cue and put out by inhibition. *Aim*: gap. *Cost*: fits. *Here*: idea 77
(mirror links). *Surprise*: a twist.

**87. Self-assembly with proofreading.** *What*: DNA tiles that compute as they crystallise
make errors; spreading each logical bit over a k by k block of tiles stops single errors from
propagating. *Source*: Winfree and Bekbolatov 2003
([record](https://resolver.caltech.edu/CaltechAUTHORS:20110309-104203016)). *Bits*: any
structure the sheet grows (wiring, chunks) is grown in blocks; a block commits only if its
members agree. *Aim*: damage, alive. *Cost*: fits. *Here*: not present. *Surprise*: a twist.

**88. Keep options open.** *What*: a force toward states with more possible futures produces
tool use and cooperation in toy systems. *Source*: Wissner-Gross and Freer 2013, PRL 110
([record](https://dspace.mit.edu/handle/1721.1/79750)); disputed by later comments. *Bits*:
in an acting world without reward, prefer actions after which the set of sensor patterns seen
in the next few steps is largest (count distinct hashes). *Aim*: acting, no teacher. *Cost*:
awkward (needs either rollouts or a learned model). *Here*: idea 38. *Surprise*: a twist
(known in RL as empowerment).

## G. Computation and engineering

**89. TAGE: tagged tables at geometric history lengths.** *What*: modern branch predictors
keep a base table and several tables indexed by a hash of the branch address and the last L
outcomes, with L growing geometrically; each entry is a small saturating counter, a partial
tag and a usefulness bit; the longest matching table predicts; on a miss a new entry is
allocated in a longer table; usefulness decides what is overwritten. It holds patterns
hundreds of branches long in a few kilobytes. *Source*: Seznec and Michaud 2006, JILP
([author page](https://www.irisa.fr/caps/people/seznec/index_en.htm)); gem5's implementation
([tage.hh](https://gem5.googlesource.com/public/gem5/+/00ea41368d9364664ab6c9fc1e7a74bdffd8d41a/src/cpu/pred/tage.hh)).
*Bits*: the history is a shift register of the cue lines, actions and outcomes; tables keyed
by hashes of the last 2, 4, 8, ... 128 ticks; each entry holds a counter per button; the
longest hit answers; a wrong answer allocates in the next longer table. The sheet can feed
its spike bits in as extra history. *Aim*: gap, many. *Cost*: fits (a few hash lookups per
tick). *Here*: not present. *Surprise*: genuinely different; engineered for the exact wall.

**90. A readout over a history register.** *What*: the perceptron branch predictor trains one
tiny perceptron per branch over the global history bits, which reaches long histories with
linear cost. *Source*: [branch predictor](https://en.wikipedia.org/wiki/Branch_predictor)
(Jiménez and Lin 2001). *Bits*: give the evidence readout a shift register of the last N
ticks of selected sheet bits (or of its own inputs) as extra sources. Cheap time without
asking the sheet to hold anything. *Aim*: gap. *Cost*: fits. *Here*: idea 58 (the readout).
*Surprise*: a twist.

**91. Context-tree weighting.** *What*: keep counts of the next bit for every context (suffix
of recent history) up to depth D in a tree, and mix the predictions of all depths with weights
stored at each node; it is optimal for any bounded-memory source, with linear cost. *Source*:
Willems, Shtarkov, Tjalkens 1995, IEEE Trans IT 41
([pdf](https://webmath.univ-rennes1.fr/stat/GPE/COURBES/articles/willems95.pdf));
[PPM](https://en.wikipedia.org/wiki/Prediction_by_partial_matching). *Bits*: per predicted bit
(a reward, a sensor line), a hashed tree of small counter pairs; one walk down the tree per
tick. *Aim*: gap, no teacher. *Cost*: fits. *Here*: idea 11 ("every neuron predicts") in
spirit. *Surprise*: genuinely different.

**92. Phrases from phrases (LZ78 and Sequitur).** *What*: LZ78 adds to its dictionary a known
phrase plus one symbol; Sequitur keeps two rules online: no pair of symbols appears twice
(else make a rule), and every rule is used at least twice (else inline it), and builds a
hierarchy in linear time. *Source*: Nevill-Manning and Witten 1997, JAIR
([arXiv cs/9709102](https://arxiv.org/abs/cs/9709102));
[LZ78](https://en.wikipedia.org/wiki/LZ77_and_LZ78). *Bits*: a hash table of pairs of symbols
(cues, chunk ids); a pair seen twice gets a new chunk id, which is itself a symbol; chunks
used once are dropped. Sequences become nested chunks the readout can see as single inputs.
*Aim*: gap, parts and wholes, no teacher. *Cost*: fits. *Here*: idea 16 (chains). *Surprise*:
genuinely different.

**93. Top-k counters with bounded memory (Space-Saving).** *What*: to find the most frequent
items in a stream with k counters, keep k (item, count) slots; a new item takes the slot with
the smallest count and inherits that count plus one; the error is bounded. *Source*:
Metwally, Agrawal, El Abbadi 2005
([pdf](https://cse.hkust.edu.hk/~raywong/comp5331/References/EfficientComputationOfFrequentAndTop-kElementsInDataStreams.pdf)).
*Bits*: each unit keeps 8 to 16 slots of (source id, evidence); a source with no slot that
earns credit evicts the weakest slot and inherits its evidence plus one step; wires are the
top slots. *Aim*: many, room, speed. *Cost*: fits; it cuts finding 19's 82 to 493 bytes per
neuron to a few dozen. *Here*: idea 59 (counters for every source). *Surprise*: a twist; a
plain engineering fix to a measured cost.

**94. A cache that learns what to forget (ARC).** *What*: ARC keeps a list of pages seen once
and a list seen twice, plus ghost lists of what each just evicted; a hit on a ghost says
"that list was too small" and shifts the split between recency and frequency, with no tuning.
*Source*: Megiddo and Modha 2003, FAST
([pdf](https://static.usenix.org/event/fast03/tech/full_papers/megiddo/megiddo.pdf)). *Bits*:
a unit's (or a memory's) slots are split into "new" and "proven"; evicted entries leave a
hash in a ghost ring; if a later credit names a ghost, the split moves toward the side it came
from. *Aim*: many (forgetting the right things). *Cost*: fits. *Here*: not present. *Surprise*:
genuinely different.

**95. Most new things die young (generational collection).** *What*: garbage collectors put
new objects in a nursery collected often and promote survivors to an old generation
collected rarely, because most objects die young. *Source*:
[tracing garbage collection](https://en.wikipedia.org/wiki/Tracing_garbage_collection),
"Generational GC" (Ungar 1984). *Bits*: new wires go into a nursery with fast decay; wires that
survive N reward windows are promoted to a slow tier that decays only under budget pressure.
*Aim*: keeping, room. *Cost*: fits. *Here*: idea 7; cat 12-14. *Surprise*: ordinary.

**96. Mark from the outputs, sweep the rest.** *What*: a tracing collector marks everything
reachable from the roots and frees the rest. *Source*: as entry 95. *Bits*: every so often,
mark every unit with a path of live wires to a motor cell (a backward search); unmarked units
are reclaimed. Usefulness becomes reachability. *Aim*: room. *Cost*: fits (rarely run). *Here*:
idea 24. *Surprise*: a twist.

**97. The less loaded of two.** *What*: placing each ball in the less full of two random bins
cuts the fullest bin from about log n / log log n to log log n; it works even on stale load
data. *Source*: Mitzenmacher, Richa, Sitaraman 2001
([pdf](https://people.cs.umass.edu/~ramesh/Site/PUBLICATIONS_files/MRS01.pdf)). *Bits*: every
allocation (a new wire's source, the unit that takes a new cue, a memory slot) samples two
candidates and takes the one with less load (fewer outgoing wires, fewer stored items).
*Aim*: many, room. *Cost*: fits. *Here*: not present. *Surprise*: genuinely different.

**98. Bloom filters and count-min sketches.** *What*: k hash bits per item answer "seen
before?" with no false negatives; a count-min sketch gives approximate counts in fixed memory.
*Source*: [Bloom filter](https://en.wikipedia.org/wiki/Bloom_filter);
[count-min sketch](https://en.wikipedia.org/wiki/Count%E2%80%93min_sketch). *Bits*: a novelty
bit per context, and cheap counts of how often a context came before reward. *Aim*: no
teacher, novelty. *Cost*: fits. *Here*: idea 123. *Surprise*: ordinary.

**99. Reservoir sampling.** *What*: a fixed-size uniform sample of an unbounded stream.
*Source*: [reservoir sampling](https://en.wikipedia.org/wiki/Reservoir_sampling). *Bits*: the
replay store for idea 87 holds a fair sample of the whole life, not only the last stretch.
*Aim*: keeping. *Cost*: fits. *Here*: idea 87; cat 48. *Surprise*: ordinary.

**100. Parity checks and bit-flip recall (LDPC).** *What*: low-density parity-check codes
store information so that every bit takes part in a few XOR checks; Gallager's decoder flips
any bit most of whose checks fail, repeatedly, in parallel, and corrects many errors.
*Source*: [LDPC code](https://en.wikipedia.org/wiki/Low-density_parity-check_code); bit
flipping described in [arXiv 1801.04668](https://pdf.arxiv.org/pdf/1801.04668). *Bits*: a
memory region where check units each XOR a few memory bits; storing a pattern means adjusting
the checks' expected values (or adding checks the pattern satisfies); recall from a damaged
or partial cue is bit flipping. *Aim*: many, damage, no teacher (fill in a gap). *Cost*: fits
(XOR and popcount). *Here*: idea 123; cat 127. *Surprise*: a twist (an attractor memory made of
parity, not of weights).

**101. Credit by code (dither, SPSA, spread spectrum).** *What*: extremum-seeking control
wiggles a parameter with a known dither and correlates the output with it to get the
gradient; SPSA perturbs all parameters at once with random signs and needs two measurements
for any number of parameters; spread-spectrum radio separates many senders on one channel by
orthogonal codes. *Source*: Spall 1998, APL Technical Digest
([pdf](https://jhuapl.edu/SPSA/PDF-SPSA/Spall_An_Overview.PDF)); extremum-seeking tutorial
([pdf](https://orbi.umons.ac.be/bitstream/20.500.12907/53772/1/Dewasme2025.pdf)). *Bits*: each
unit (or tile) has a pseudo-random ±1 code; on each trial it nudges its threshold (or flips a
candidate wire) by its code bit; the reward minus its average is accumulated into each unit's
counter with the sign of its own code. Everything is perturbed at once, so the experiment is
big enough to change the answer, and the codes keep the credit apart. *Aim*: inside. *Cost*:
fits (one sign multiply-add per unit per outcome). *Here*: idea 1, 60; cat 104-106 are the
single-noise versions. *Surprise*: a twist that answers finding 12.

**102. Step size by uncertainty (Kalman gain).** *What*: a Kalman filter corrects its estimate
by an amount set by how uncertain it is, and the uncertainty grows when the world may have
changed. *Source*: [Kalman filter](https://en.wikipedia.org/wiki/Kalman_filter). *Bits*: each
evidence counter carries a small confidence count; the step is large when confidence is low,
small when high; a run of surprises lowers confidence. *Aim*: keeping and changing at once.
*Cost*: fits. *Here*: idea 93. *Surprise*: ordinary (known as adaptive learning rates).

**103. Phase-locked loops and entrainment.** *What*: a PLL compares the phase of an input
with its own oscillator and corrects the frequency until locked; the circadian clock is
reset by light according to its phase response curve. *Source*:
[phase-locked loop](https://en.wikipedia.org/wiki/Phase-locked_loop);
[phase response curve](https://en.wikipedia.org/wiki/Phase_response_curve). *Bits*: a phase
counter per unit; XOR of the input pulse and the unit's own pulse is the error; the period
moves by the sign of the error. A bank of these locks to a beat and fires just before the
next one. *Aim*: the beat world (fails today). *Cost*: fits. *Here*: idea 86. *Surprise*: a
twist.

**104. Amounts as bit streams (sigma-delta, stochastic computing).** *What*: a sigma-delta
modulator emits a bit stream whose density equals its input by accumulating the error;
stochastic computing multiplies two densities with one AND gate. *Source*:
[delta-sigma modulation](https://en.wikipedia.org/wiki/Delta-sigma_modulation); Alaghi and
Hayes 2013 ([pdf](https://homes.cs.washington.edu/~armin/ACM_TECS_2013.pdf)). *Bits*: an output
cell that accumulates (target amount minus its own output) and fires on overflow gives the map
its missing "output an amount". *Aim*: magnitude tier, amount output. *Cost*: fits. *Here*:
cat 165. *Surprise*: ordinary.

**105. Additive increase, multiplicative decrease.** *What*: TCP senders add one to their rate
each round and halve it on congestion; from any start this converges to a fair and full share
with one bit of shared feedback. *Source*: Chiu and Jain 1989
([pdf](https://sites.cs.ucsb.edu/~ravenben/classes/papers/cj89.pdf)). *Bits*: each unit's share
of a common resource (its wire count, its firing rate, its share of credit) rises by one per
window and halves when a global "too much" bit is set. *Aim*: room, alive. *Cost*: fits. *Here*:
finding 2 is per-unit, not shared. *Surprise*: a twist.

**106. Exponential backoff and watchdogs.** *What*: after a failed send, wait a random time
from a window that doubles each failure; a watchdog resets a component that stops checking in.
*Source*: [exponential backoff](https://en.wikipedia.org/wiki/Exponential_backoff). *Bits*: a
motor group that was wrong waits a random time from a doubling window before it may win again;
a group bad for N in a row with no good gets its recent changes rolled back. *Aim*: lock.
*Cost*: fits. *Here*: ideas 8, 25, 53. *Surprise*: a twist.

**107. Tabu lists.** *What*: tabu search forbids undoing recent moves for a while, which stops
cycling. *Source*: [tabu search](https://en.wikipedia.org/wiki/Tabu_search). *Bits*: a unit
remembers its last few changed wires and may not reverse them for T outcomes. *Aim*: lock.
*Cost*: fits. *Here*: idea 53. *Surprise*: ordinary.

**108. Bandit bonuses.** *What*: UCB adds a bonus that shrinks with how often an option was
tried; Thompson sampling plays each option with the probability it is best. *Source*: Auer et
al. 2002 ([doi](https://doi.org/10.1023/A:1013689704352)); Russo et al. tutorial
([arXiv 1707.02038](https://arxiv.org/abs/1707.02038)). *Bits*: each evidence counter has a
try count; choices use evidence plus a bonus, or evidence plus noise sized by uncertainty.
*Aim*: lock, acting. *Cost*: fits. *Here*: idea 33. *Surprise*: ordinary (known machine
learning).

**109. Encapsulation and defined functions (genetic programming).** *What*: Koza's
encapsulation operation picks a useful subtree, names it and makes it a new primitive;
automatically defined functions evolve subroutines that the main program calls many times,
which made parity problems tractable. *Source*: Koza, Genetic Programming (1992), ch. 6
(encapsulation) and ch. 20-21 (building blocks, hierarchies) (owner's library). *Bits*: when a
group of wires (a mask over sources at relative offsets) has earned a lot, store it as a named
feature that any unit anywhere may take as one input. *Aim*: carry, parts and wholes. *Cost*:
fits. *Here*: ideas 100, 125. *Surprise*: a twist; sharing by naming, not by leaking.

**110. Cellular automaton classes, gliders, Lenia.** *What*: CA rules fall into classes from
dead to chaotic, with complex structure between (gliders in Life carry information without
decay); Lenia's continuous rules host lifelike moving creatures. *Source*: Packard and Wolfram
1985, "Two-dimensional cellular automata" (owner's library); Chan 2019, Complex Systems 28
([arXiv 1812.05433](https://arxiv.org/abs/1812.05433v2)). *Bits*: screen sheet rules by class;
look for gliders that could carry a cue. *Aim*: carry. *Cost*: fits (Lenia is awkward:
floats). *Here*: idea 98; cat 141. *Surprise*: ordinary.

**111. Commit, branch, revert.** *What*: version control keeps snapshots, tries changes on
branches, and reverts. *Source*: [version control](https://en.wikipedia.org/wiki/Version_control).
*Bits*: snapshot the masks after a good run; on a run of bad, revert. *Aim*: lock, keeping.
*Cost*: fits. *Here*: ideas 8, 32. *Surprise*: ordinary.

**112. Counters that merge without double counting (CRDTs).** *What*: a grow-only counter keeps
one slot per replica and merges by taking the maximum per slot, so copies can be combined in
any order. *Source*:
[conflict-free replicated data type](https://en.wikipedia.org/wiki/Conflict-free_replicated_data_type).
*Bits*: tiles that share evidence for a feature at an offset (idea 100) keep a slot per tile
and merge by max, so passing evidence around never inflates it. *Aim*: carry. *Cost*: fits.
*Here*: idea 100. *Surprise*: a twist; a detail that makes idea 100 safe.

**113. Token buckets.** *What*: a rate limiter allows bursts but caps the average by spending
tokens that refill at a fixed rate. *Source*:
[token bucket](https://en.wikipedia.org/wiki/Token_bucket). *Bits*: each wire change costs a
token; a unit's bucket refills slowly; a burst of learning is allowed after a surprise, then
the unit is quiet. *Aim*: room, keeping. *Cost*: fits. *Here*: finding 1 budgets wires, not
changes. *Surprise*: a twist.

## H. Minds and behaviour

**114. Habituation and sensitisation.** *What*: response to a repeated harmless stimulus falls,
recovers with rest, falls faster next time, and comes back after a strong different stimulus;
Sokolov's comparator model says a stored template of the expected stimulus suppresses response
and any mismatch, even a weaker stimulus, restores it. *Source*: Bristol, Purcell, Carew,
"Habituation", Arbib's Handbook (owner's library, read); Rankin et al. 2009
([PMC2754195](https://pmc.ncbi.nlm.nih.gov/articles/PMC2754195)). *Bits*: per input group a
fast and a slow decrement counter; a mismatch against the stored template resets the fast one.
*Aim*: no teacher, novelty. *Cost*: fits. *Here*: idea 66 (tiring synapses). *Surprise*:
ordinary; the checks are what matter (section 3).

**115. The summed prediction (Rescorla-Wagner).** *What*: learning on each cue is driven by the
reward minus the summed prediction of all cues present, which explains blocking,
overshadowing and conditioned inhibition; it is the delta rule. *Source*: Schmajuk,
"Conditioning", Arbib's Handbook (owner's library, read). *Bits*: the readout's error must be
computed from the total drive of all present sources, so a cue that is already fully
predicted stops the others from learning. Check whether the error rule of finding 15 blocks.
*Aim*: readout. *Cost*: fits. *Here*: ideas 4, 58. *Surprise*: ordinary (LMS).

**116. Attention to cues (Mackintosh, Pearce-Hall).** *What*: Mackintosh: a cue's learning rate
rises when it is the best predictor; Pearce-Hall: it rises when what follows it is surprising;
both explain latent inhibition. *Source*: Schmajuk, "Conditioning", Arbib's Handbook. *Bits*:
an associability counter per input group that scales its evidence steps, raised by surprise
(or by being the best predictor) and lowered by being irrelevant. *Aim*: inside (which
inputs to learn from). *Cost*: fits. *Here*: ideas 6, 89; cat 38. *Surprise*: a twist.

**117. Extinction is new learning, tied to context.** *What*: an extinguished response comes
back in the original context (renewal), after time (spontaneous recovery) and after a reminder
(reinstatement); the original learning was not erased, a context-dependent inhibitory
memory was added. *Source*: Bouton 2004, Learning and Memory 11
([review copy](https://typeset.io/pdf/context-and-behavioral-processes-in-extinction-1xm3d3evmg.pdf));
Bouton 2002 ([pdf](https://www.appstate.edu/~steelekm/classes/psy5150/Documents/Bouton2002.pdf)).
*Bits*: when an answer that used to be good turns bad, do not lower its evidence; raise a
separate inhibitory counter keyed to the current context bits (a hash of slow-changing
inputs), which decays faster than the original. *Aim*: changed rule, keeping. *Cost*: fits.
*Here*: not present (the current rules erase). *Surprise*: genuinely different here.

**118. Rates, not trials.** *What*: Gallistel and Gibbon argue that animals estimate the rate
of reward with a cue and without it and respond when the ratio is high enough; acquisition
depends on the ratio of intervals, not on the number of pairings; Rescorla showed that reward
equally likely with and without the cue teaches nothing. *Source*: Gallistel and Gibbon 2000,
Psychological Review 107 ([record](https://scilit.com/publications/8e995e5c60790740235e7ad3e2f7a7ab));
Rescorla 1968 ([pdf](https://www.appstate.edu/~steelekm/classes/psy5300/Documents/Rescorla1968.pdf)).
*Bits*: for each source, two count pairs: rewards and ticks while active, rewards and ticks
while not; wire when the first rate exceeds the second by a ratio. *Aim*: inside (contingency
instead of co-occurrence), gap. *Cost*: fits. *Here*: idea 4. *Surprise*: a twist.

**119. Matching and melioration.** *What*: pigeons share pecks between two keys in the same
proportion as the rewards they get from them, not all on the better one. *Source*: Herrnstein
1961, JEAB 4 ([PMC1404074](https://pmc.ncbi.nlm.nih.gov/articles/PMC1404074)). *Bits*: press
buttons with probabilities proportional to their evidence. *Aim*: acting, lock. *Cost*: fits.
*Here*: not present. *Surprise*: ordinary; mainly a check.

**120. A pacemaker and an accumulator.** *What*: scalar timing theory: a pacemaker emits
pulses, a switch opened by the cue lets an accumulator count them, the count at reward is
stored in reference memory, and the animal responds when the current count is close to a
stored one; it gives timing whose spread grows with the interval. *Source*: Church and
Broadbent, in Schmajuk, "Conditioning", Arbib's Handbook (owner's library, read). *Bits*: a
counter per cue group started at cue onset; at good, push the count into a small ring of
stored counts; a unit fires when the live count is within a fixed ratio of any stored count.
*Aim*: gap (the gap and delayed-echo worlds). *Cost*: fits (a counter and a few compares).
*Here*: not present; cat 62 times by a bank of decays instead. *Surprise*: genuinely
different here; time held as a number, not as activity.

**121. Learning a map without reward (latent learning, Dyna).** *What*: Tolman's rats explored
a maze without food and later used what they had learned as soon as food appeared; Dyna agents
learn a model all the time and replay it to plan, and are tested on mazes that change.
*Source*: Tolman 1948 ([text](https://psychclassics.yorku.ca/Tolman/Maps/maps.htm)); Sutton and
Barto, 1st ed., ch. 9 (owner's library). *Bits*: always learn a table of (state hash, action)
to next state hash; when reward appears, replay the table backward from it during idle.
*Aim*: acting. *Cost*: fits. *Here*: idea 87; cat 47. *Surprise*: ordinary (known
reinforcement learning).

**122. Curiosity by learning progress.** *What*: a robot that rewards itself for the drop in
its own prediction error in each region of its experience spends time on learnable things in
order of difficulty and ignores noise. *Source*: Oudeyer, Kaplan, Hafner 2007, IEEE TEC
([pdf](https://adapt.informatik.hu-berlin.de/pub/papers/ims.pdf)). *Bits*: per sector, a fast
and a slow average of prediction error; their difference is learning progress; extra noise
and plasticity go where it is high. *Aim*: acting, no teacher. *Cost*: fits. *Here*: idea 36
(noise where surprise is: surprise is not progress). *Surprise*: a twist.

**123. Dreams as deliberate corruption.** *What*: Hoel proposes that dreams exist to stop the
brain overfitting to the day, by replaying it in strange, corrupted forms. *Source*: Hoel
2021, Patterns ([PMC8134940](https://pmc.ncbi.nlm.nih.gov/articles/PMC8134940)). *Bits*: in
idle stretches, replay recorded stretches with parts swapped, shifted or masked, learning on.
*Aim*: carry (generalisation), keeping. *Cost*: fits. *Here*: ideas 34, 87; cat 45-50.
*Surprise*: a twist; corrupt the replay on purpose.

**124. Spaced repetition (Leitner boxes).** *What*: cards answered correctly move to a box
reviewed less often; a miss sends them back to the first box. *Source*:
[Leitner system](https://en.wikipedia.org/wiki/Leitner_system). *Bits*: each stored
association has a box number; in idle time the system re-presents itself items from due boxes
(using its own memory as the cue), promoting or demoting them. *Aim*: keeping, many. *Cost*:
fits. *Here*: idea 35. *Surprise*: a twist; self-scheduled rehearsal.

**125. A race between rule and memory (instance theory).** *What*: Logan: every encounter
stores an instance; performance is a race between a slow general procedure and retrieval of
a past instance; with practice retrieval wins and the skill becomes automatic. *Source*:
Logan 1988, Psychological Review 95
([pdf](https://pcl.sitehost.iu.edu/rgoldsto/courses/cogscilearning/loganautomaticity.pdf)).
*Bits*: every answer is also stored in the content memory (idea 123) keyed by the input; each
tick both the sheet and the memory answer, and the more confident (or the faster) wins.
*Aim*: many, one-shot. *Cost*: fits. *Here*: ideas 123, 124. *Surprise*: a twist; it says how
the organs of idea 124 should combine.

**126. Chunks that compete (PARSER).** *What*: a stream of syllables is perceived in random
groupings of one to three known units; each grouping is stored with a weight; all weights
decay, overlapping chunks interfere, and chunks above a threshold become units; words fall
out with no statistics computed. *Source*: Perruchet and Vinter 1998, J Mem Lang 39
([pdf](https://lead.ube.fr/wp-content/uploads/2023/09/000029-parser-a-model-for-word-segmentation.pdf)).
*Bits*: a small table of chunks (sequences of up to three cue or chunk ids) with weights;
each tick take the next one to three items as a percept, strengthen it, weaken all others a
little and weaken those overlapping it more; strong chunks become symbols. *Aim*: gap, parts
and wholes, no teacher. *Cost*: fits. *Here*: idea 16. *Surprise*: genuinely different here.

**127. Learning to learn (learning sets).** *What*: monkeys given hundreds of new
two-object problems end up solving a new one after a single trial. *Source*: Harlow 1949,
Psychological Review 56 ([pdf](https://psyc.franklin.uga.edu/sites/default/files/inline-files/Harlow.pdf)).
*Bits*: a slow layer that learns which rule (win-stay-lose-shift over problems) pays, above the
fast learning of each problem. *Aim*: changed rule. *Cost*: fits. *Here*: not present.
*Surprise*: ordinary; mainly a check.

**128. A terraced scan with temperature (Copycat).** *What*: Copycat runs many small
stochastic agents (codelets) chosen by urgency; a temperature measures how incoherent the
current structure is and sets how random the choices are, so good structure protects itself
and bad structure is torn down. *Source*: Mitchell and Hofstadter on temperature
([pdf](https://journalpub.escholarship.org/cognitivesciencesociety/article/30848/galley/20697/download));
Hofstadter 1984, AI Memo 755 ([pdf](https://apps.dtic.mil/sti/pdfs/ADA142744.pdf)). *Bits*: a
global (or per-region) temperature set by how much the readout's evidence disagrees with
itself; high temperature raises noise and plasticity, low temperature freezes. *Aim*: lock,
relation tier. *Cost*: fits for the temperature; awkward for the symbolic rest. *Here*: idea
85 (arousal) is near. *Surprise*: a twist; temperature from coherence, not from time.

**129. Imitation through a mirror learned by babbling.** *What*: true imitation needs a map
from seen actions to one's own; one account is that it is learned when one sees one's own
actions. *Source*: "Imitation", Arbib's Handbook (owner's library). *Bits*: during babbling
(idea 38) the world shows the sensor sheet what each press looks like; co-activity wiring
links that view to the motor cells; later, being shown someone else's press fires them.
*Aim*: being shown the answer (a map hole). *Cost*: fits. *Here*: ideas 38, 39. *Surprise*:
ordinary.

## I. Braitenberg's Vehicles

Read in full from the owner's copy: Braitenberg, *Vehicles: Experiments in Synthetic
Psychology* (MIT Press, 1984; [publisher](https://mitpressbookstore.mit.edu/book/9780262521123)),
the fourteen vehicles and the "Biological Notes on the Vehicles". Page numbers are the book's.
It is the ancestor of this project's attitude: behaviour that looks like mind from a few wires,
and the "law of uphill analysis and downhill invention" (machines are easier to build than to
guess from outside, so observers overrate what is inside). Vehicles 1 to 3 (crossed and
uncrossed, excitatory and inhibitory sensor-motor wires) are the base the project already
stands on. Each entry from vehicle 4 says what the project has and what the book adds.

**130. Vehicle 4: a preferred intensity.** *What*: sensor-to-motor links that rise to a peak
and fall again (4a) or jump at a threshold (4b) make vehicles orbit sources, trace figure
eights, and seem to "ponder" before acting. *Source*: Vehicles, ch. 4, pp. 15-19; notes pp.
105-108 (motion is seen best at one speed; toads treat small things as prey, large as threat,
middle-sized as worth a look). *Bits*: a unit that fires when its active-input count is
between two thresholds, not above one (a band-pass popcount). *Aim*: magnitude tier, acting.
*Cost*: fits. *Here*: cat 28 (non-monotonic dendritic spikes); idea 71 has two thresholds for
a different purpose. *Adds*: band units as a cheap way to read an amount. *Surprise*: a twist.

**131. Vehicle 5: threshold logic, latches, and the beach.** *What*: threshold devices with
excitation, inhibition and a one-tick delay count, detect "a burst of exactly three", and hold a
bit in a pair that excite each other; a vehicle that leaves marks on a beach and reads them
back computes with numbers bigger than its brain (a Turing machine, notes p. 113). *Source*:
Vehicles, ch. 5, pp. 20-25; notes pp. 108-113. *Bits*: the sheet already is threshold logic;
the beach is entry 55 (a scratch grid the sheet writes and reads). *Aim*: gap. *Cost*: fits.
*Here*: the engine; entry 55. *Adds*: the case that a small fixed brain plus a writable world
outruns any fixed brain; Braitenberg had stigmergy as memory in 1984. *Surprise*: ordinary for
the latches, a twist for the beach.

**132. Vehicle 6: sloppy copying, with whole parts swapped.** *What*: copy vehicles off a table
as fast as others fall off it, with mistakes; the most creative mistakes are copying one part of
the brain from one vehicle and another part from another, so each part arrives intact and
tested. *Source*: Vehicles, ch. 6, pp. 26-28. *Bits*: in the design search (idea 98) or the
ecology (section 2), recombine at module boundaries (whole regions or organs from different
parents), never bit by bit inside a module; replace at the rate of failure, so the population
stays a fixed size. *Aim*: the search. *Cost*: fits. *Here*: ideas 30-33, 98. *Adds*:
recombination of intact modules. *Surprise*: ordinary (it is evolution), with one useful rule.

**133. Vehicle 7: Mnemotrix, which relaxes back.** *What*: a wire whose resistance drops when
both ends are active together and "little by little" returns to its initial value, so only
pairs that recur often enough stay coupled; it gives association, concepts and generalisation
(a "not grey" wire picks up the lesson "painted means dangerous"). *Source*: Vehicles, ch. 7,
pp. 29-32; notes pp. 114-116 (Hubel and Wiesel's squinting kittens: a cell keeps, from many
weak inputs, the ones that carry the same signal). *Bits*: co-activity wiring where each
strengthening decays back unless repeated within its decay time. *Aim*: many, keeping.
*Cost*: fits. *Here*: finding 3 (co-activity wiring) is Mnemotrix; idea 93 (counters that
forget) is the relaxation. *Adds*: rate of recurrence, not count, decides what stays.
*Surprise*: ordinary; the project has it.

**134. Vehicle 8: maps, motion detectors, and planning by current.** *What*: an orderly map of
the senses makes cheap tricks possible: an object detector is a threshold unit over four
neighbours; a motion detector ANDs one cell with a delayed copy of its neighbour (Reichardt);
lateral inhibition keeps edges and drops uniform areas; a network where a cell fires fully only
if a neighbour fired a moment before passes smooth trajectories and rejects noise, the test
that an object is real (notes p. 127, fig. 34); and a uniform conducting sheet with a voltage
between "here" and "goal" carries most current along the straight line, which gives the
heading. *Source*: Vehicles, ch. 8, pp. 33-42; notes pp. 120-123, 127-128. *Bits*: delay-and-AND
motion units and continuity units are idea 74 and the sheet's delays; the planner is a
diffusion or wavefront from the goal cell on an internal grid map (entry 85), followed by
stepping toward the neighbour that the wave reached first. *Aim*: acting (forage, mazes), carry.
*Cost*: fits. *Here*: idea 74; delays; inhibition. *Adds*: an internal map used to plan, and
continuity as the reality test. *Surprise*: ordinary for detectors, a twist for planning by
flow.

**135. Vehicle 9: shapes known in advance.** *What*: wiring each map point to its mirror point
across the midline, so that pairs excite each other, makes a bilateral-symmetry detector ("some
one is facing me"); approaching a radially symmetric pattern produces no seen motion, which
detects it; lateral inhibition responds best to stripes at the spacing set by its reach,
whatever their orientation. In the notes, the corpus callosum's 200 million fibres join mirror
points of the two hemispheres. *Source*: Vehicles, ch. 9, pp. 43-49; notes pp. 129-130. *Bits*:
fixed mirror links between symmetric points of the sensor sheet; a symmetry unit pooling them.
*Aim*: carry (a built-in invariant), relation tier (same or mirror). *Cost*: fits. *Here*: not
present. *Adds*: invariants wired in as structure of the map, which the owner's "no wiring at
birth that solves a task" rule would allow because they serve every task. *Surprise*: a twist.

**136. Vehicle 10: ideas from chains, and the danger of holding them.** *What*: from pairwise
"A is next to B" associations a closed chain can click into one idea (the edge of the world);
an event-driven ring of seven threshold devices, advanced one step by each flower, ends up with
one device always active when food is found ("every seventh flower"). Braitenberg warns that an
idea held active keeps all its members on together, so association wires everything to
everything and the order is lost; his fix is to let activity circulate around the chain so only
the serial links strengthen. *Source*: Vehicles, ch. 10, pp. 50-54. *Bits*: ring counters
stepped by events (not ticks) as inputs to the readout; and when holding activity (idea 111),
hold it as a circulating sequence, not a standing block, or turn co-activity wiring off for
held units. *Aim*: gap (counting, "every Nth"), many. *Cost*: fits. *Here*: idea 111 (a place
that holds) and finding 3 together are exactly the hazard he names. *Adds*: event clocks, and a
warning for idea 111. *Surprise*: a twist.

**137. Vehicle 11: Ergotrix, and sequences that teach concepts.** *What*: a one-way wire that
strengthens when one element fires shortly before another stores "B follows A"; Braitenberg
keeps it separate from Mnemotrix (things against their history) and then couples them:
whenever an Ergotrix wire between two groups strengthens, the Mnemotrix wires inside each group
strengthen too, so concepts form fastest for things that take part in regular sequences. The
notes tie this to the pyramidal cell: long-range input on the apical dendrite learns "terms"
(things made of many senses), local input on the basal dendrites learns "relations" within an
area, and one learning event over the whole cell makes each condition the other. *Source*:
Vehicles, ch. 11, pp. 55-61; notes pp. 134-140. *Bits*: wire-by-timing (idea 81) between
groups; when it strengthens, give a co-activity bonus inside the source group and the target
group. In unit terms: a unit's far slots and local slots learn under the same credit event.
*Aim*: gap (sequence), carry. *Cost*: fits. *Here*: ideas 16, 81 (Ergotrix); finding 3
(Mnemotrix); cat 31, 34 (apical and basal). *Adds*: the coupling rule. *Surprise*: a twist.

**138. Vehicle 12: threshold control as the pump of thoughts.** *What*: a box reads the total
activity (better, its rate of change, to act before an explosion) and sets every threshold;
when a concept's group ignites, thresholds rise, only the most self-consistent group survives
(focus), its Ergotrix links light its successors, thresholds rise again, the strongest
successor survives and the previous one is put out because the threshold is now higher. The
result is a train of states that follows learned order but, through the one-dimensional map
from activity now to activity next, cannot be predicted. The notes guess the striatum is the
activity detector. *Source*: Vehicles, ch. 12, pp. 62-69; notes pp. 143-144. *Bits*: a global
regulator on total spike count (or its change) that sets thresholds each tick. *Aim*: gap,
sequence recall, acting (lookahead). *Cost*: fits. *Here*: finding 2 (regulation by sector
causes the pulse) and idea 17 (no pulse) removed exactly this oscillation. *Adds*: the pulse
is the engine of stepping from one held assembly to its successor; with Ergotrix wiring it may
be worth having back, in a region of its own (idea 112). *Surprise*: genuinely different
relative to the project's reading of its own pulse.

**139. Vehicle 13 (a): a predictor running ahead, checked point by point.** *What*: let the
Ergotrix chains replay faster than the world and the brain predicts; keep two "half brains", one
fed only by the senses, one running the predictor, joined point to point by mutual inhibition
so only their differences stay active (a mismatch map); keep a short delay line of the last two
ticks so the moment that went wrong is still there when the mismatch shows; on conflict,
"believe the sensors" and switch the predictor off. *Source*: Vehicles, ch. 13, pp. 70-79,
figure 20. *Bits*: a second sheet (or table) whose transitions are learned from the first and
stepped k times per world tick; a comparator layer XORs its state for t+k against the sensory
copy when t+k arrives (held in a delay ring); mismatch bits are surprise, per place; on a large
mismatch the predictor is reset from the sensory copy. *Aim*: no teacher (predict the next
input), gap, surprise as a signal for ideas 36 and 84. *Cost*: fits (a second engine, an XOR
and a delay ring). *Here*: idea 11 (every neuron predicts) is local; nothing runs ahead as a
whole or compares maps. *Adds*: the whole foresight loop. *Surprise*: genuinely different here.

**140. Vehicle 13 (b): reverberation after a strong outcome.** *What*: when an old, built-in
evaluator (a vehicle 6 brain with its motors removed: one wire for good, one for bad) fires
strongly, the predictor is cut off from the senses and fed from the delay line, and its output
fed back into the delay line, so the two ticks before the event circulate again and again
until the evaluator calms. A rare but decisive event is made frequent inside, so one meeting
with a vicious green vehicle is enough. *Source*: Vehicles, ch. 13, pp. 76-79, figure 21.
*Bits*: the project keeps its last 16 spike maps (idea 62). On a strong outcome (good or bad
after a long run of the other, or a large surprise), replay the last two or three maps through
the learning rule N times at once, N set by how strong the outcome was, then continue. *Aim*:
one-shot learning of rare events, gap (credit to the ticks just before). *Cost*: fits (N extra
learning passes, only on rare outcomes). *Here*: idea 87 replays in sleep, later; cat 47-48
(prioritized replay) are the nearest. *Adds*: replay at once, looped, sized by the outcome.
*Surprise*: a twist; ready to build on what exists.

**141. Vehicle 14: optimism.** *What*: when the predictor offers several equally likely next
states, hold the present, run through the options, have the evaluator score each, and take the
most pleasing; because the next states include the vehicle's own movements, choosing the best
imagined continuation steers the vehicle toward goals it cannot see yet, and it even performs
the expected actions early, "as if it could hasten the event". Braitenberg says this takes the
mystery out of goal-directed behaviour and that he does not know whether the idea is his. *Source*:
Vehicles, ch. 14, pp. 80-84; notes p. 144. *Bits*: transition counters from (state hash,
action) to next state hashes (Ergotrix in table form), value counters per state hash (the
evaluator); at each choice, look up the likely next states for each action and bias the choice
toward the one whose next state has the best value, ties broken by value rather than by chance.
*Aim*: acting. *Cost*: fits (a few lookups per decision). *Here*: not present; the project has no
model-based choice at all. *Adds*: one-step lookahead with an optimistic tie-break. It is
one-step model-based reinforcement learning under another name. *Surprise*: genuinely different
here; also a behaviour check: does the design act in anticipation (section 3, item 15 is the
timing version).

**142. From the notes: negation by inhibitory relay.** *What*: a cell that learns only from
inputs active together cannot learn "A and not B"; Braitenberg proposes that every input also
inhibits some cells through an interneuron, so the cortex has a set of cells for "input on" and
another for "input off", and conjunction learning then handles negated terms as easily ("a
featherless biped"). *Source*: Vehicles, notes pp. 139-140. *Bits*: give every input line (and
optionally every sheet region) an inverted copy that co-activity wiring and the evidence readout
can wire to. *Aim*: combination tier (exclusive-or needs "not"). *Cost*: fits (one more line per
input). *Here*: table neurons can express negation; popcount masks cannot. *Surprise*: a twist;
cheap and testable at once.

**143. From the notes: fixation by asymmetric motion responses.** *What*: a fly turns toward
seen motion, but more for backward motion on one side than for forward; a wiggling object then
produces a net turn toward itself, so many identical detectors with slightly asymmetric
outputs, summed onto a few motor cells, make the fly fixate and land on objects. *Source*:
Vehicles, notes pp. 105-108 (Reichardt). *Bits*: motion units (entry 134) whose push on the
left and right motor groups differs by direction; no learning; a steering reflex for the catch
world. *Aim*: acting (catch). *Cost*: fits. *Here*: not present. *Surprise*: a twist; it is a
prior, not a learning rule.

**144. From the notes: the neuronal zoom.** *What*: cortical cells with dendritic fields of very
different sizes overlap in the same region; we switch between reading the small ones (single
dots, centipede legs) and the large ones (density, shading), taking whichever "makes the most
sense". *Source*: Vehicles, notes pp. 124-127. *Bits*: two populations of units over the same
input with reach differing by about five times; the readout takes evidence from both and the
better-predicting set gains weight (entry 116). *Aim*: carry (scale, clutter). *Cost*: fits.
*Here*: idea 106 (stages with growing reach) is serial; this is parallel. *Surprise*: a twist.

**145. From the notes: memory shows as growing difference.** *What*: Braitenberg argues that
real memory traces must make synapses differ more from each other with experience; a change
that affects all synapses alike is ageing, not learning; Schüz found such growing variance
between inexperienced and experienced guinea pig cortex. *Source*: Vehicles, notes pp. 119-120.
*Bits*: a measurement: across a life, track the variance between units' wiring (or evidence)
in the inner sheet; learning that is real raises it, drift that is shared does not. *Aim*: the
inside wall, as a diagnostic for when inner learning starts to happen. *Cost*: fits. *Here*: not
measured. *Surprise*: a twist; a cheap probe for the open problem.

**146. From the notes: intensity becomes timing.** *What*: a real neuron reaches threshold
sooner when driven harder, so the amount above threshold, lost in a clocked threshold device,
survives as spike time and desynchronises outputs. *Source*: Vehicles, notes pp. 110-113.
*Bits*: within a tick, fire strongly driven units first (sub-tick order), as in cat 152. *Aim*:
carry (intensity invariance). *Cost*: awkward (needs ordering inside a tick). *Here*: cat 152.
*Surprise*: ordinary.

## J. The owner's earlier designs

Two of the owner's earlier attempts, cloned under `thonksheet-library/repos/`. Mechanisms there
that the project has not tried are listed here, credited to him.

**147. A phase tag on every neuron (binary-brain-v1).** *What*: in his one-bit substrate each
neuron carries a fixed 16-bit oscillator phase and frequency tag beside its coordinates, with
grid-like and place-like codes expected from interference between tags. *Source*: the owner,
`binary-brain-v1/SPEC.md`, sections 2 and 9 (2025). *Bits*: each unit has its own small phase
counter with its own period; a unit may fire, or learn, only in its own phase window, so the
same input reaches different units at different times and interference between periods gives a
coarse code of elapsed time and place. *Aim*: gap, carry. *Cost*: fits (one counter per unit).
*Here*: idea 86 is one global rhythm; cat 61 (phase coding), 68-69 (grid codes). *Surprise*: a
twist; per-unit clocks, not a shared one.

**148. Four broadcast bits, with pleasure as surprise while error falls (binary-brain-v1).**
*What*: his substrate receives only four global bits: pleasure, pain, surprise and comfort
(activity in band); surprise is "mean prediction error over a threshold", and pleasure is
"surprise while the loss is falling" (or an outside score); plasticity happens only under
surprise or pain, with the sign set by pleasure. *Source*: the owner, `binary-brain-v1/SPEC.md`,
sections 5.2 and 6. *Bits*: as written; the rule there is already in bit operations. *Aim*: no
teacher, acting. *Cost*: fits. *Here*: idea 84 (novelty opens the gate) and entry 122 (learning
progress) are its parts; the combination, with pleasure defined as progress, is not in ideas.md.
*Surprise*: a twist; it is entry 122 arrived at independently and wired to a gate.

**149. A machine whose state is the address of its next operation (vvm).** *What*: one state
vector queries a bank of operation vectors by similarity; the nearest few are candidates, one is
sampled and added into the state, which is then thresholded and normalised; the result is the
next query. There is no program counter and no separate key and value: "op == address ==
effect". Two control fixes from his notes: an operation that just ran is locked out for a few
ticks (refractory), and an operation whose result was wrong is pushed slightly away from the
query that called it (weak rejection), so failures stop monopolising a region of state space.
*Source*: the owner, `vvm/README.md`, `vvm/docs/vvm-seed-spec.md` and
`vvm/docs/model-architecture.md` ("Route stability", "Weak rejection"). He trained the bank with
gradients; the translation drops that. *Bits*: state is a bit vector (say 512 bits, which can
include sensor lines); the bank holds a few thousand op words; each tick, popcount-match the
state against the bank (hashed buckets keep it cheap), XOR or OR the winner into the state,
threshold. Learning without gradients: on good, move the used op's bits a step toward the query
that called it (and its effect toward the state that followed); on bad, flip a few of its bits
away from the query (his rejection, in bits). *Aim*: gap (the state carries itself forward as a
program), acting, many. *Cost*: fits. *Here*: idea 123 is the memory organ; nothing in ideas.md
uses retrieved content as the next step's instruction. *Surprise*: genuinely different; a fifth
whole-system design (section 2).

---

## Sources, read and recalled, and thin veins

Read in the primary text: Braitenberg's Vehicles, all of it from vehicle 3 on, and the
biological notes (section I, entries 130-146); the owner's `binary-brain-v1/SPEC.md` and the
vvm README and design notes (section J); Baum and Durdanovic section 2 (entry 63); in the
owner's books, Arbib's articles "Habituation", "Conditioning", "Stochastic resonance" and the
opening of "Chains of oscillators"; Kennedy et al.'s sections on Latané and on Boyd and
Richerson; Koza's contents and the encapsulation operation; the Ant Fugue passages of GEB; the
contents of ABOP and the opening of Packard and Wolfram. That is about a quarter of the
entries. Of the rest (entries 1-129), for about half,
the mechanism was checked against an abstract or a search summary of the cited source. The
rest are standard results described from memory, with the link checked to exist but the text
not re-read. Treat specific numbers as checked only where the entry quotes them from the
source (Couzin's 5% of 200, Baum's 200 against 8 blocks, Herrnstein's matching).

Thin veins: kinetic and molecular machinery beyond proofreading (most of it reduces to
switches and integrators already listed); physics past hysteresis and waves (annealing,
spin glasses and criticality are already ML-adjacent and known here); voting and auctions
(they reduce to entries 63, 64 and 66). Richest: computation and engineering (branch
prediction, compression and caching were built for exactly the gap and many walls under
hardware limits) and the immune system (a whole learning system with no wiring).
