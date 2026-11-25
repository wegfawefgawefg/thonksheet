#!/usr/bin/env bash
# Do the inner rules learn more from a steadier sheet? (idea 174, queue item 2j;
# docs/results-2026-11-26-steady.md). Competitive learning (compete2's B5) and the one-hop
# rule with errors from the answer live 60,000 MNIST digits (or pairs) with one change to
# the life (longer showings, steadier regulation, the counted rule, slower rules over longer
# lives), then the frozen sheet is read by a fresh reader, the standard way (std) and the
# quiet way (regulation frozen, 80 ticks a digit), everything at matched rate (cap 1000).
# Each arm runs with its controls: lived, shuffled digits, churn (competitive learning) or
# the coin (one-hop rule); the born sheet and the teacher's wiring are read alongside.
#   scripts/steady.sh search                         every arm, seeds 5-8, both tests, both reads
#   scripts/steady.sh search --reads std --kinds born,lived,shuffled,churn,coin
#   scripts/steady.sh search --arms base,count5,s80t --seeds 5-12      some arms, more seeds
#   scripts/steady.sh search --tasks pair --reads quiet                one test, one read
#   scripts/steady.sh search --table-only            the table from what has run
#   scripts/steady.sh smoke --quick                  five arms, seed 5, short lives: end to end
#   JOBS=2 scripts/steady.sh search                  runs at once (default 3)
# Arms (scripts/steady.py ARMS): base s40d s40t s80d s80t slowth th32 thfreeze slowbias count5
# count1 r6 r7 r5l r6l r7l (competitive learning); hbase hs40d hs80d hs80t hslowth hthfreeze
# hslowbias h9l h10 h11 h10l h11l (the one-hop rule). Kinds: born teacher lived shuffled churn
# coin. Results in out/steady/NAME/TASK-READ/ (one file a run, kept: a line run again only
# runs what is missing), the table in out/steady/NAME/table.txt. MNIST from data/mnist or
# THONK_MNIST; the teacher's wirings from REPEAT_WIRING (default: the main checkout's
# out/from-agents/repeat/wiring). On stevejobs:
#   MINI_DIR=thonk-steady scripts/mini.sh bg st 'THONK_MNIST=~/thonk-hop2/data/mnist JOBS=2 scripts/steady.sh search --arms r5l --kinds lived,shuffled'
set -euo pipefail
cd "$(dirname "$0")/.."
[ -x build/blobmnist ] || scripts/build.sh --target blobmnist
[ -d data/mnist ] || [ -n "${THONK_MNIST:-}" ] || echo "no data/mnist: scripts/get_mnist.sh" >&2
[ $# -ge 1 ] || { sed -n '2,26p' "$0"; exit 1; }
exec python3 scripts/steady.py "$@" --jobs "${JOBS:-3}"
