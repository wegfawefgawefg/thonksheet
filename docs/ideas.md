# Ideas

The running list. The owner, 2026-11-24: "just keep listing ideas, trying them, it may take
100 ideas but surely we will find some core ones that actually work. we know its possible."

Rules for this file:
- Every idea gets a number that never changes, a tag for whether it is a known idea or new
  as far as we know, what it is aimed at, and a status.
- An idea is tried on the fixed ladder (same tasks, same seeds, the sweep tool) and its
  line gets the result and a pointer to the results doc. Nulls and failures stay on the
  list; they are results.
- Add ideas at the bottom whenever one comes up. Do not reorder.

Aims: **carry** (a cue reaching across tiles), **credit** (wiring the right thing on good
and bad), **room** (not saturating, holding more than one thing), **alive** (stable
activity), **speed**.

Status words: idea, building, tried (with result), kept, dropped.

## Credit rules

1. **Noise is the experiment.** [known rule; exact noise bit is ours] credit, room. Only
   neurons whose spike was noise get graded, against a running average of reward. *building*
2. **A budget per neuron.** [known] room. Every strengthening is paid for by a weakening
   elsewhere on the same neuron. *idea*
3. **Causal credit.** [known family] credit. Grade only a source that fired exactly its
   slot's delay before the target fired. *idea*
4. **Reward minus expectation.** [known] credit, room. Keep the current rule but act only on
   reward better or worse than its running average. *idea*
5. **A critic per sector.** [known family] credit. Each sector keeps its own average of the
   reward that follows its activity and grades itself against that. *idea*
6. **Grade the unexpected.** [rare] credit. Only pairs where the target firing was unusual
   for it get graded. *idea*
7. **Fast bit, slow bit.** [known theory] room. Use the two weight planes as a quick bit
   that flips easily and a slow bit that follows only if the quick one held. *idea*
8. **Undo on bad.** [new?] credit. Keep a tiny log of the last flips per neuron and revert
   them on bad, instead of weakening everything that was active. *idea*
9. **Winners only.** [known] credit, room. In each tile only the few most-driven neurons
   that fired get graded. *idea*
10. **Motor first.** [known-ish] credit. Only motor neurons learn until their score stops
    rising, then the rest unlock. *idea*

## Carrying a signal

11. **Every neuron predicts.** [known family] carry. Each neuron learns to guess its own
    next firing from its delayed inputs, with no reward. *idea*
12. **One spike makes one spike.** [known theory, rare as a regulator] carry, alive. Regulate
    each sector so a spike causes on average one spike downstream. *idea*
13. **Relays at birth.** [plain engineering] carry. A share of neurons born as delay-line
    relays of one source. *idea*
14. **Highways.** [design] carry. Born with a few top-strength connections along one
    direction per neuron, so activity has a way to flow. *idea*
15. **Learned delays.** [rare] carry. Move a slot's delay one tick, by chance, on an
    outcome. *idea*
16. **Chains.** [known] carry. Grow and strengthen A to B to C where they fire in order at
    their delays. *idea*
17. **No pulse.** [ours] alive, credit. Slow the sector regulator so activity stops
    pulsing. *tried: Echo late score 0.85 to 0.90 mean over 4 seeds, the failed seed
    recovers; too few seeds. docs/results-2026-11-24-rhythm.md*
18. **Open a gate near input.** [known in biology] carry. Sensor activity lowers inhibition
    around it for a moment. *idea*

## Structure and wiring

19. **Layers inside a tile.** [known] carry, room. Input, middle and output groups by bit
    position. *idea, asked for by the owner*
20. **Fewer slots per neuron.** [ours] speed, variety. Listen to 4 of the 9 local tiles.
    *idea*
21. **Fine locality inside a tile.** [known] carry. Prefer close cells at birth and growth.
    *idea*
22. **Start as a reflex, pull apart.** [new?] carry. Input and output areas overlap at
    first and are moved apart as the score holds. *idea*
23. **Grow when it matters.** [known-ish] credit. Growth chance goes up for a while after
    good. *idea*
24. **Drop what never mattered.** [plain] room. A connection not graded in many rewards is
    removed. *idea*
25. **Rebirth.** [known] room, alive. A neuron silent or stuck for long is reborn with fresh
    random wiring. *idea*

## Different neurons

26. **Table neurons.** [old idea, new on this sheet] speed, credit. A neuron reads 6 bits
    and looks up a 64-entry table of tiny counters. *building*
27. **All-of neurons.** [known] credit. Fire only if every chosen input is on. *idea*
28. **Context gate.** [known in biology] carry, room. Input on the far slots decides whether
    local input is allowed to fire the neuron. *idea*
29. **Bursts as a second symbol.** [known, 2021] credit. Two spikes close together count for
    more downstream and mark the moment for learning. *idea*

## Selection

30. **Selection among tiles.** [new?] credit, carry. Copy the masks of tiles active before
    good outcomes over tiles that did poorly. *idea*
31. **Random flips.** [known] baseline. Flip bits, keep them if the score went up.
    *building*
32. **Twin tournament.** [known] baseline. Two blobs that differ by a few flips; the better
    one each window is copied over the other. *idea*
33. **A bandit per sector.** [new?] credit. Each sector alternates between two small mask
    variants and keeps the one followed by more reward. *idea*

## Memory

34. **Sleep.** [known] room. Input cut, weak weights decay, replay. *idea, liked by the
    owner*
35. **Make it permanent.** [known family] room. A connection strengthened by good many times
    is locked. *idea*

## Exploration

36. **Noise where surprise is.** [rare] credit. More spontaneous firing in sectors whose
    predictions are failing. *idea*
37. **Noise on motors only.** [plain test] credit. Compare with noise everywhere. *idea*
38. **Babbling.** [known in robotics] carry. Before any task, random motor firing while
    prediction learns what each motor does to the sensors. *idea*

## Teaching signals

39. **Show the answer.** [known] carry. After a wrong answer the world briefly fires the
    right motor cells. Tests association without the credit problem. *idea*
40. **More to sense.** [task design] credit. Extra sensor lines that say how close the goal
    is. *idea*

## Baselines and measurements

41. **Frozen blob, learned readout.** [known] baseline. *building*
42. **Carrying test.** How many tiles away can a cue still be read from activity. *tried:
    cue identity is readable 3 tiles out at birth, 4 with growth and pruning on, 6 after a
    long life or with many far links. Echo fails inside that range, so from 2 to 4 tiles the
    problem is credit, not carrying. docs/results-2026-11-24-carry.md*
43. **Capacity test.** How many cue-button pairs one blob holds: 2, 4, 8, 16. *idea*
44. **Retention test.** Learn A, learn B, retest A. *idea*
45. **Throughput numbers.** Synapse checks, real events, signal bits and stored bits in the
    speed test. *idea*
46. **A reader as weak as the rule.** Repeat the carrying test with a readout that learns the
    way the blob does, to see how far the real rule can read. *idea*
47. **Many far links.** [plain] carry. Half the neurons send far instead of 15%. *tried in
    the carrying test: reach 4 to 6 tiles; not yet tried on a task*
48. **Pool before deciding.** [known] credit. A motor cell listens to many weak neurons
    through one hop of relay cells that each sum a neighbourhood, so thinly spread
    information gets gathered before the reward rule has to find it. *idea*
