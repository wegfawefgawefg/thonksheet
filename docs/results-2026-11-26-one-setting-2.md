# The one setting with what merged after it (plan item A2, 2026-11-26)

Run by the coordinator on the quick map of docs/results-2026-11-26-one-setting.md (22
worlds, seeds 5 to 12, 20,000-step lives, each beside its learning-off twin; a row passes
when it is at least halfway from chance to perfect and above its twin by 2 standard
errors). Five columns, named in `scripts/variants/oneset.txt`:

- `a2ru`: the output rule with the runner-up rule (`--g.readout_good 2 --g.margin_percent
  20 --g.margin_win 2`, idea 161), nothing else.
- `a2comp`: the output rule with competitive learning (`--g.tile_winners 4 --g.compete_on 1
  --g.compete_shift 5`, idea 158; no reward reaches the inside).
- `a2rucomp`: both. No cell kind built by hand, no reward to the inside.
- `a2bestru`: the one setting (`best`) with the runner-up rule in place of its close-vote
  gate.
- `a2bestcomp`: the one setting with competitive learning added.

## Short version

- **Without any hand-built cell, 13 of 22** (`a2rucomp`; the output rule alone 6, the one
  setting 15, the GRU 16). It is the best column on exclusive-or (0.997), parity of 3
  (0.831; the one setting 0.642, the GRU 0.518), gap (0.938), copy of 2 (0.927), and ties
  the best on online MNIST (0.838; GRU 0.733). Parity of 3 is passed by no other sheet
  column.
- **What it lacks against the one setting is holding and many cues:** the 1,000-tick hold
  0.508 (1.000), 16 cues 0.620 (0.955), order 0.733 (0.994), catch 0.366 (0.671), reach to
  5 tiles 0.841 (0.933). Those are what time and hold cells give.
- **The runner-up rule belongs in the one setting:** `a2bestru` passes the same 15 rows and
  lifts online MNIST 0.714 to 0.841, with every two-button row unchanged (with two buttons
  the two rules are the same rule). 8 cues 0.976 to 0.992; pairs 0.525 to 0.460; noisy
  lines 0.307 to 0.215.
- **Competitive learning does not add to the one setting:** `a2bestcomp` passes 15, gains
  reach (5 tiles 0.968) and loses order (0.883), gap (0.786), exclusive-or (0.875) and
  copy (0.820); parity of 3 stays at 0.662. Hold cells and learned loops compete rather
  than add; with two winners per tile (the one setting's) the rule is also not at the
  setting it was tuned at (four).
- **Nothing here learns the chain, cartpole, noisy lines, keeping pairs or the held-out
  rows.** Competitive learning takes the chain to 0.000 in every column it is in.
- Each piece alone: the runner-up rule 9 rows (it is an output rule: MNIST 0.821, 16 cues
  0.538, bandit 0.997; it costs exclusive-or 0.07); competitive learning 11 rows (reach,
  exclusive-or, parity of 3, gap, delayed echo, copy; MNIST online stays at 0.234 because
  the plain output rule cannot use it).

## The table

`scripts/oneset.sh table base,best,a2ru,a2comp,a2rucomp,a2bestru,a2bestcomp,gru`. Late
score; `*` passes; in brackets the paired difference to base, `+`/`-` at 2 standard errors.

| row | base | best | a2ru | a2comp | a2rucomp | a2bestru | a2bestcomp | gru |
|---|---|---|---|---|---|---|---|---|
| echo2 | 1.000* | 1.000* (+0.000) | 1.000* (+0.000) | 1.000* (+0.000) | 1.000* (+0.000) | 1.000* (+0.000) | 1.000* (+0.000) | 1.000* (-0.000) |
| echo8 | 0.801* | 0.976* (+0.176+) | 0.930* (+0.129+) | 0.957* (+0.156+) | 0.925* (+0.124+) | 0.992* (+0.192+) | 0.970* (+0.170+) | 0.998* (+0.197+) |
| echo16 | 0.266 | 0.955* (+0.689+) | 0.538* (+0.272+) | 0.423 (+0.157+) | 0.620* (+0.354+) | 0.952* (+0.686+) | 0.942* (+0.676+) | 0.992* (+0.726+) |
| shuffled | 1.000* | 1.000* (+0.000) | 1.000* (+0.000) | 1.000* (+0.000) | 1.000* (+0.000) | 1.000* (+0.000) | 1.000* (+0.000) | 1.000* (-0.000-) |
| far3 | 0.847* | 0.975* (+0.128+) | 0.811* (-0.036-) | 0.984* (+0.137+) | 0.925* (+0.079+) | 0.975* (+0.128+) | 0.996* (+0.149+) | 1.000* (+0.153+) |
| far5 | 0.576 | 0.933* (+0.357+) | 0.595 (+0.019+) | 0.854* (+0.278+) | 0.841* (+0.265+) | 0.933* (+0.357+) | 0.968* (+0.391+) | 1.000* (+0.424+) |
| xor | 0.882* | 0.936* (+0.054+) | 0.812* (-0.070-) | 0.994* (+0.112+) | 0.997* (+0.115+) | 0.936* (+0.054+) | 0.875* (-0.007) | 0.988* (+0.106+) |
| parity3 | 0.561 | 0.642 (+0.081+) | 0.553 (-0.008) | 0.798* (+0.237+) | 0.831* (+0.270+) | 0.642 (+0.081+) | 0.662 (+0.101+) | 0.518 (-0.043) |
| order | 0.676 | 0.994* (+0.318+) | 0.745* (+0.068+) | 0.683 (+0.007) | 0.733 (+0.057) | 0.994* (+0.318+) | 0.883* (+0.206+) | 0.998* (+0.322+) |
| gap | 0.509 | 0.897* (+0.387+) | 0.525 (+0.016) | 0.918* (+0.409+) | 0.938* (+0.429+) | 0.897* (+0.387+) | 0.786* (+0.277+) | 0.994* (+0.485+) |
| delayed2 | 0.544 | 1.000* (+0.456+) | 0.580 (+0.036+) | 0.935* (+0.391+) | 0.919* (+0.375+) | 1.000* (+0.456+) | 1.000* (+0.456+) | 0.999* (+0.455+) |
| hold1000 | 0.506 | 0.998* (+0.492+) | 0.469 (-0.037) | 0.501 (-0.005) | 0.508 (+0.002) | 0.998* (+0.492+) | 1.000* (+0.494+) | 0.495 (-0.011) |
| bandit | 0.773* | 0.995* (+0.223+) | 0.997* (+0.225+) | 0.778* (+0.005) | 1.000* (+0.227+) | 0.995* (+0.223+) | 0.996* (+0.224+) | 1.000* (+0.227+) |
| chain | 0.306 | 0.150 (-0.155) | 0.275 (-0.031) | 0.000 (-0.305-) | 0.000 (-0.306-) | 0.150 (-0.155) | 0.000 (-0.306-) | 0.010 (-0.295-) |
| cartpole | 17 | 17 (-0.502) | 14 (-3.191-) | 19 (+1.369+) | 13 (-4.450-) | 17 (-0.408) | 16 (-0.945) | 703* (+686+) |
| catch | 0.298 | 0.671* (+0.373+) | 0.405 (+0.106+) | 0.277 (-0.021-) | 0.366 (+0.067+) | 0.664* (+0.365+) | 0.697* (+0.399+) | 0.723* (+0.425+) |
| noisy | 0.268 | 0.307 (+0.039) | 0.347 (+0.079+) | 0.199 (-0.070-) | 0.218 (-0.051) | 0.215 (-0.053-) | 0.192 (-0.076) | 0.666* (+0.398+) |
| copy2 | 0.638 | 0.897* (+0.259+) | 0.709 (+0.070+) | 0.888* (+0.250+) | 0.927* (+0.289+) | 0.897* (+0.259+) | 0.820* (+0.182+) | 0.998* (+0.359+) |
| inv-scale | 0.699 | 0.602 (-0.097) | 0.681 (-0.018) | 0.711 (+0.013) | 0.719 (+0.020) | 0.602 (-0.097) | 0.598 (-0.100) | 0.569 (-0.129-) |
| inv-position | 0.487 | 0.471 (-0.016) | 0.502 (+0.015) | 0.512 (+0.025) | 0.536 (+0.049) | 0.471 (-0.016) | 0.452 (-0.034-) | 0.388 (-0.099-) |
| pairs | 0.273 | 0.525 (+0.252+) | 0.392 (+0.119+) | 0.227 (-0.046) | 0.358 (+0.085+) | 0.460 (+0.188+) | 0.546 (+0.273+) | 0.338 (+0.065) |
| mnist | 0.214 | 0.714* (+0.499+) | 0.821* (+0.606+) | 0.234 (+0.019) | 0.838* (+0.624+) | 0.841* (+0.627+) | 0.721* (+0.507+) | 0.733* (+0.519+) |
| rows passed | 6 | 15 | 9 | 11 | 13 | 15 | 15 | 16 |

## What is suspect, what was not done

- Lives are 20,000 steps, half the standard; one setting of each rule, nothing tuned.
- No inside-learning read (fresh output on the frozen lived sheet) was taken for these
  columns; idea 166 took it for competitive learning on delayed echo and gap.
- Not run: `a2bestru` with competitive learning; competitive learning in the one setting
  with four winners per tile; hold cells without time cells beside competitive learning.

## Script lines

```
scripts/oneset.sh a2rucomp                 one column (cached lives are reused)
scripts/oneset.sh table base,best,a2ru,a2comp,a2rucomp,a2bestru,a2bestcomp,gru --se
```
