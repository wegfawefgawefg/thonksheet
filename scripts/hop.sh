#!/usr/bin/env bash
# The error sent one hop, to every hidden cell (genome hop_on; docs/results-2026-11-25-hop.md).
# Each scores the inside after a taught life against the sheet as born and the controls.
#   toy: a fresh output (the readout rule, learning 20,000 steps) on the sheet after a
#     40,000-step life with the readout and the rule, against the born sheet; tasks xor
#     (both-not-either), parity3, parity4 (points). Settings: born, off (the readout alone
#     in the life), m2 (told the right button), m1 (told good or bad), m1good (and on good
#     the pressed button should have been higher), m2coin and m1coin (the same errors on
#     random lines: the coin control).
#   calc: blobprobe calc's ridge reader of the frozen sheet after a taught life of --life
#     ticks (a verdict per bit), motor cells put back as born; xor4, parity4, add4.
#   MNIST: scripts/inside.sh NAME --teach 2 ... (born, lived, shuffled, coin).
# Every flag it does not know goes to every life: that is the rule under test.
#   scripts/hop.sh toy toys                                    seeds 5-8, every task
#   scripts/hop.sh toy toys --settings born,m2 --tasks xor --seeds 5-12
#   scripts/hop.sh toy toys-near0 --g.hop_near 0               a variant (new NAME)
#   scripts/hop.sh calc calc --seeds 5,6
#   scripts/hop.sh toy toys --table-only
# Options: --seeds 5-8, --jobs 3, --tasks, --members, --settings, --life (calc ticks).
# Results in out/hop/NAME/table.txt; finished runs are kept.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobprobe >/dev/null
exec python3 scripts/hop.py "$@"
