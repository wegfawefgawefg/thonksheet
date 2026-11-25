# Prior art

Written from memory on 2026-11-24, not from a fresh search. Check a claim before leaning on
it. Every piece of this design has been tried somewhere; the whole has not, as far as we know.

- **Hierarchical Temporal Memory (Numenta).** Binary neurons, dendrite branches with their
  own thresholds, synapses that grow toward recently active cells, always learning, no
  backprop. Closest on mechanisms. It learned sequences and anomalies but never got
  reward-driven behavior working and never matched deep networks.
- **Reward-gated spike-timing learning** (Izhikevich 2007 and after). A broadcast reward
  acting on recently used synapses. Our good/bad signal. Shown on small tasks.
- **Synaptic tagging** (Frey and Morris 1997). The biology behind "recently used, then a
  later signal decides".
- **Few-state synapses with random flips** (Amit and Fusi 1994; the cascade model, 2005).
  Why a low chance of change gives long memory, and why a hidden "hard to change" state
  helps.
- **Evolved learning rules** (Najarro and Risi 2020, and others). Evolution finds the
  plasticity rule; the network starts random and learns within one life. It drove a racing
  car and walked a quadruped. Small nets, floats, one task each. The best evidence for the
  genome idea.
- **Tsetlin machines and weightless networks.** Learning with bit operations and chance
  updates. Classification only.
- **Reservoir computing.** A fixed random recurrent net with a trained readout. Shows a
  free-running blob holds useful time information.
- **Dynamic sparse training** (deep rewiring, RigL). Prune and regrow at a fixed connection
  count, with backprop.
- **Streaming and continual reinforcement learning** (Sutton's group, Carmack's work). The
  same always-on, no-episode goal, with backprop.
- **Loss of plasticity** (Dohare et al. 2024). Networks trained forever stop being able to
  learn; replacing little-used units fixes it. Our growth and pruning play that role.
- **Neuromorphic chips** (Loihi, SpiNNaker). The same hardware-first idea on special
  hardware. We aim at ordinary CPUs and GPUs.

What nobody has shown: a bit-packed, free-running, self-wiring blob whose genome is searched
over a broad curriculum, learning arbitrary tasks. That is either the opening or the warning.

## Corrections and additions, 2026-11-25 (from a sourced search)

Checked against sources while writing `docs/mechanism-catalogue.md`; entry numbers below
refer to it. The list above stands except where noted.

- **Tsetlin machines and weightless networks are not "classification only".** Tsetlin
  machines have been extended to regression, convolution and contextual bandits (Granmo 2018,
  https://arxiv.org/abs/1804.01508, and its follow-ups, including a NeurIPS 2022 contextual-
  bandit paper). Weightless (n-tuple, RAM) networks go back to Bledsoe and Browning 1959; our
  table neurons (finding 5) are RAM neurons with counters, so this is direct prior art for
  them (entries 130 to 132).
- **Dynamic sparse training is not only a backprop method.** Deep rewiring came out of
  synaptic sampling (Kappel et al. 2015, PLoS Comput Biol, https://doi.org/10.1371/journal.pcbi.1004485),
  where noisy rewiring under a fixed connection count is driven by local rules (entry 23).
- **Few-state synapses: the best-matched result was missing.** Baldassi, Braunstein, Brunel and
  Zecchina 2007 (PNAS, https://arxiv.org/abs/0707.1295) learn online with binary synapses and
  hidden integer states; an error moves every hidden state, a correct answer with a thin margin
  deepens the hidden state of the synapses that helped, a comfortable correct answer does
  nothing. This is our evidence counter plus error rule with one case added, and it qualifies
  finding 17 (entry 15). Benna and Fusi 2016 (Nat Neurosci, https://doi.org/10.1038/nn.4401)
  supersede the cascade model on capacity (entry 13).
- **HTM's "permanence" is our evidence counter.** Each potential synapse has a hidden scalar and
  is connected above a threshold (Hawkins and Ahmad 2016, https://doi.org/10.3389/fncir.2016.00023;
  Cui, Ahmad and Hawkins 2017, https://doi.org/10.3389/fncom.2017.00111). Finding 14 has prior
  art in it. Numenta's later work turned to sensorimotor object recognition with grid-like
  location codes (Lewis et al. 2019, https://doi.org/10.3389/fncir.2019.00022); this search
  found no HTM work on reward-driven behavior, which fits the line above but does not prove it.
- **Reward-gated spike-timing learning has a known requirement.** The reward must be centered
  per stimulus by a critic, not by one global average (Frémaux, Sprekeler and Gerstner 2010,
  J Neurosci, https://doi.org/10.1523/JNEUROSCI.6249-09.2010, cited from memory). Experimental
  support for eligibility traces of about a second arrived from 2014 on (Yagishita et al. 2014,
  https://doi.org/10.1126/science.1255514; He et al. 2015, with separate traces for
  strengthening and weakening, https://doi.org/10.1016/j.neuron.2015.09.037).
- **Evolved learning rules: weaker evidence than stated.** In Najarro and Risi 2020, reviewers
  noted that fixed weights beat the evolved Hebbian rules in two of three settings
  (NeurIPS 2020 reviews). Meta-learned rule searches so far mostly rediscover known rules
  (Confavreux et al. 2020; Jordan et al. 2021, https://doi.org/10.7554/eLife.66273). Still the
  best evidence for searching the rule, but not proof that search finds new ones.
- **Reservoir computing is the nearest prior art to the headline, and it has two animals.** A
  fixed random expansion with learning only at the output is the fly mushroom body (Dasgupta
  et al. 2017, https://doi.org/10.1126/science.aam9868; Lin et al. 2014, where breaking the
  sparsening feedback inhibition stops flies telling similar odors apart,
  https://doi.org/10.1038/nn.3660) and the Marr-Albus cerebellum. Litwin-Kumar et al. 2017
  (https://doi.org/10.1016/j.neuron.2017.01.030) show both sit near the in-degree that maximizes
  dimension.
- **Pushing the output's error inward has a direct precedent.** The attention-gated family
  (AGREL, AuGMEnT, BrainProp) feeds back from the chosen action to tag the synapses that fed it
  and uses a global reward error; BrainProp is proven equal to backprop for one output at a time
  (Pozzi, Bohté and Roelfsema 2020, NeurIPS), and AuGMEnT learns what to hold in working memory
  from reward (Rombouts et al. 2015, https://doi.org/10.1371/journal.pcbi.1004060). Missing
  above.
- **Loss of plasticity: the fix is more specific than "replace little-used units".** Dohare et
  al. 2024 (Nature, https://doi.org/10.1038/s41586-024-07711-7) reinitialize a small fraction of
  the least-used units continually; the preprint ties the loss to dead units, very large weights
  and lost unit diversity.
- **Neuromorphic chips: closer relatives than Loihi.** Binary synapses with stochastic on-chip
  learning exist in silicon (Frenkel et al. 2019, ODIN and MorphIC, https://arxiv.org/abs/1804.07858)
  and in memory devices (Suri et al. 2013, https://doi.org/10.1109/TED.2013.2263000); Loihi's
  learning engine runs programmable rules on pre, post and reward traces (Davies et al. 2018,
  https://doi.org/10.1109/MM.2018.112130359).
- **The owner's handbook (Arbib (ed.) 2003, on disk) adds older prior art.** Mel's article on
  dendritic learning gives the capacity of neurons made of small thresholded branches (46 times
  a linear cell); Bastian's on electrolocation describes anti-Hebbian cancellation of
  self-caused input; Bullock and Rhodes's on competitive queuing describes holding a sequence
  as a gradient. See entries 167 to 186.
