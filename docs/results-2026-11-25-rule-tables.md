# The learning rule as a table (idea 120, 2026-11-25)

The owner, on searching over the numbers of existing mechanisms: "is just combination
search no? param search. it cant come up with totally new mechanisms right". Right. This
job makes the learning rule itself data, so a search can write a rule nobody wrote.

Every hand-written rule in the engine has one shape: at some moment, for a (neuron, source)
pair, look at a few local yes-or-no facts and move that pair's evidence up, down or not at
all, with some chance. Here that shape is the whole rule: a table with one entry per
combination of ten facts.

SHORT

## What was built

### The ten facts

Each fact is one bit per (neuron, source) pair, and for one neuron and one slot each is a
single 64-bit word over the slot's 64 sources (or one bit for the whole neuron). Bit f of an
entry's index is fact f.

| bit | fact | as a word |
|---|---|---|
| 0 | the source fired. At a reward: in the window before it, with one chance per tick it fired. On the neuron's own spike: in the last `recent_ticks` | the spike maps of the window, one at a time; `blob.recent` |
| 1 | the source fired exactly one slot delay before the neuron's last spike (the causal one) | the ring map at age (last spike + slot delay) |
| 2 | the source is inhibitory (far slots count as excitatory, as in learn_readout) | `inhibitory[src]` |
| 3 | the source is connected | `w0 \| w1` |
| 4 | the source's counter is above zero (direct read-off: the connection is strong) | 64 counter bytes compared; or `w1` |
| 5 | the neuron fired recently: in the window (reward), in the ticks before this spike (own spike) | OR of maps, per tile |
| 6 | the neuron is a button's cell (the plug's mark, `motor_attached`) | per tile |
| 7 | a button's cell: of the button just pressed (`motor_acted`). Any other neuron: its last spike was noise (`noise_ring`) | per tile |
| 8 | the moment: a reward just arrived (1), or the neuron just fired (0) | |
| 9 | bad: the reward was bad (reward moment), or the last reward was bad (own spike) | |

Sensor cells are never touched (the world drives them; their weights only feed the
prediction). Facts 6 and 7 together give four kinds of neuron: an inner neuron fired by its
inputs, an inner neuron fired by noise, a cell of another button, a cell of the pressed
button. The plug's mark says which button was pressed, never which was right; bad says
only that it was wrong. No fact carries the answer.

Left out, from the candidate list: "it is a sensor cell" (inert, see above), "its tile's
surprise is high" (no rule used it and it would cost a bit). Fact 7 doubles up so that the
noise fact (the noise rule of learn modes 3 and 4) costs no extra bit.

### The table and its file

An entry is 4 bits: action (0 nothing, 1 counter up, 2 counter down, 3 counter one step
toward zero) times 4, plus a chance index (one of four shifts, 2^-shift). 1,024 entries are
512 bytes. In front of them an 8-digit header: the four shifts, the window (ticks before a
reward that count, 1 to 15), the read-off, the balance, and a 0. A rule is therefore one
string of 1,032 hex digits, header first, then entry 0, 1, 2 ... A rule file is that string
with any whitespace and `#` comments (`rules/*.rule`; `scripts/rule_table.py` reads and
writes them). This is the format a general search can treat as one hex-valued number.

Read-offs (header digit 6):
- 0 evidence: the counters of `learn_evidence.cpp` (signed, 6 bits), and its read-off: a
  cell wires the sources with the highest counters, as many as it was born with, all at
  strength 4. This is the readout's own store; with the table on, every non-sensor neuron
  has a row (704 bytes each), not only the motor cells.
- 1 evidence, strength by counter: the same, but a wired source's strength follows its
  counter (1 to 3: strength 1, 4: strength 4, 5 and up: 5). Counters start at the born
  strengths, so a neuron read off before anything moved keeps exactly the wiring it was born
  with. Without this, the first read-off of an inner neuron would turn all its connections
  to strength 4 and change the sheet for reasons that have nothing to do with learning.
- 2 direct: no counters. The connection's own level (0, 1, 4, 5) is the counter, and a new
  connection is made only under `fan_in_max`. This is the store the old rule and growth
  write to.

Balance (header digit 7): as in learn_readout, the chance for cells of the other buttons
drops by their number over the pressed button's.

`--rule-file path` (or `--rule-hex digits`) loads the table; genome number `rule_on`
switches it on: 0 off (today's behavior, byte-identical), 1 the table is the only learning,
on every neuron; 2 the genome's learn mode runs as usual and the table runs on every neuron
that is not a button cell (the inside); 3 the table is the only learning, on button cells
only (the sheet frozen).

### How it runs

`src/blob/learn_rule.cpp` (300 lines), `learn_rule_table.cpp` (parse and compile),
`learn_rule.hpp`. One hook at the top of `tick_learn`; Blob and Genome fields at the ends of
their structs; `learn_evidence.cpp` gives every neuron a row when the table is on and gains
the graded read-off.

- On load, the 1,024 entries are cut into 32 groups by the five moment and neuron facts,
  and in each group the entries that share an action and chance are merged into cubes over
  the facts they do not depend on (the error rule is 4 cubes, the old rule 2, growth 1).
- Per tick, after integrate-and-fire: on a reward, for each tile the neuron facts are words;
  for each group that has cubes, the neurons of that group are an AND of words; a neuron in
  no non-empty group costs nothing. For each such neuron and slot the five source facts are
  built, and for each cube the mask of pairs is an AND of facts or their complements, then a
  sparse random word for the chance, then the counters under the mask move. A cube that
  wants "the source fired" at a reward is applied once per tick of the window with that
  tick's spike map, so a source that fired three times gets three chances, as in the error
  rule; once a move takes a pair out of its cube (a strong link weakened to weak), that pair
  gets no more chances from it. On ticks without a reward the same runs for neurons that
  just fired, if the table has any own-spike entries.
- After a neuron's counters move it is read off once. No floats; chance is a random word;
  nothing per synapse is branched on.

### Off is byte-identical

`out/rules/ident/run.sh`: echo (mode 0) and exclusive-or (learn mode 5 with evidence, 128
cells), seed 5, 3,000 steps, against a build of 1372305: the csvs are identical and the
summaries differ only in ticks per second.

## The form is rich enough: three hand-written rules as tables

`scripts/rules.sh reproduce` (`scripts/sweep.py` over `scripts/variants/rules-*.txt`): each
rule beside its table, seeds 5-12, full lives (`--ticks 20 --sensor-side 3 --motor-side 3
--steps 40000 --window 1000`), late score (mean of the last 4 windows) and the learning-off
twin. The error rule runs with 128 motor cells per button as its results doc did; the old
rule and growth on the packed plug of the design engine. Table minus hand is paired by
seed, with its standard error.

| rule | task | hand | table | table - hand | twin |
|---|---|---|---|---|---|
| error rule (`rules/error.rule`) | echo k=2 | 1.000 | 1.000 | +0.000 | 0.519 |
| | echo k=4 | 0.997 | 0.997 | +0.000 ±0.001 | 0.247 |
| | exclusive-or | 0.883 | 0.869 | -0.014 ±0.010 | 0.489 |
| old good/bad rule (`rules/old.rule`) | echo k=2 | 0.555 | 0.626 | +0.071 ±0.010 | 0.476 |
| | echo k=4 | 0.197 | 0.221 | +0.024 ±0.007 | 0.153 |
| | exclusive-or | 0.462 | 0.465 | +0.003 ±0.007 | 0.457 |
| growth (`rules/grow.rule`) | echo k=2 | 0.536 | 0.647 | +0.111 ±0.030 | 0.476 |
| | echo k=4 | 0.155 | 0.166 | +0.011 ±0.011 | 0.153 |
| | exclusive-or | 0.448 | 0.456 | +0.008 ±0.012 | 0.457 |

The tables, as `scripts/rules.sh print` gives them:

- error rule (read-off 0, window 15, balance on): "when bad arrives, a cell of the pressed
  button loses evidence for excitatory sources that fired (one chance per tick they fired),
  chance 1 in 16"; "... gains evidence for inhibitory sources that fired"; "when bad
  arrives, a cell of another button gains evidence for excitatory sources that fired";
  "... loses evidence for inhibitory sources that fired". 64 entries, 4 cubes.
- old rule (read-off 2, window 6): "when good arrives, any neuron that fired recently
  strengthens its links from connected sources that fired, chance 1 in 4 per tick they
  fired"; "when bad arrives, any neuron that fired recently weakens its links from strong
  sources that fired, chance 1 in 4". 64 entries, 2 cubes.
- growth (read-off 2): "when the neuron fires, any neuron strengthens its links from
  unconnected sources that fired recently, chance 1 in 4096". 128 entries, 1 cube.

- **The error rule is reproduced.** Same scores to the third decimal on echo, -0.014 on
  exclusive-or (t -1.5, within noise; per seed the two lives differ by at most 0.06). The
  table does the same moves; only the order in which random words are drawn differs.
- **The old rule and growth are reproduced in kind and score a little higher as tables**,
  significantly on echo k=2 (+0.07 and +0.11) and not on the rest. Both differences are in
  the moment, not in the facts, and are known:
  - the hand-written old rule acts on the 4 ticks after a reward on pairs that fired in the
    6 ticks before each of those ticks, so it also strengthens links active while the good
    or bad line itself fires; the table looks only at the 6 ticks before the reward.
  - hand-written growth picks one slot at random and then one candidate in it, which
    favours sources in slots with few candidates (far slots); the table gives every
    candidate pair the same chance (1 in 4,096, about 1 in 32 per firing, the same rate).
  Neither rule learns much here anyway (old rule +0.08 over its twin on echo k=2, growth
  +0.06 and nothing elsewhere); the point is that both are expressible and behave alike.

So all three hand-written rules are rows of the same table. Matching scores, not bytes, was
the bar.

SEARCH

COST

## Script lines

```
scripts/rules.sh print rules/error.rule                 a table as sentences
scripts/rules.sh life rules/error.rule echo --k 4       one life, button cells only
scripts/rules.sh inside rules/old.rule order            readout at the buttons, table inside
scripts/rules.sh reproduce                              the three reproductions, seeds 5-12
scripts/rules.sh search NAME --mode inside --init hand --gens 20 --jobs 2
scripts/rules.sh status NAME
scripts/rules.sh show NAME 3
scripts/rules.sh confirm NAME 4                         best 4 on seeds 5-12
```

ROUGH
