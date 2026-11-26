#!/usr/bin/env bash
# The closed loop on the whole quick map, an output that hears only good and bad, and why
# parity of 3 fell (idea 188; docs/results-2026-11-26-loop-everywhere.md). Each NAME is a
# fixed recipe in scripts/loopmap.py:
#   mapscreen     the quick map's 18 rows where the world can name the right line, seeds 1-3:
#                 the blob's own output alone (alone) and the closed loop (loop), beside the
#                 cached columns base, a2rucomp and best
#   map           the same on seeds 5-12
#   mapknobs      the loop with the knobs validation liked on MNIST, seeds 1-3
#   judgescreen   told only good or bad (--g.out_judge 1..4, --g.out_tell): the rows with
#                 more than two buttons and the acting rows, seeds 1-3
#   same          the check that with two buttons good-or-bad is the told rule: the two
#                 kinds of life window for window, 13 rows, seeds 1-3
#   judge         told the right line and told good or bad (form 2) side by side, all 22
#                 rows, seeds 5-12
#   judgebits     the form that breaks one-down-for-one-up at 12, 8 and 6 bits a counter
#   judgesheet    the MNIST sheet told only good or bad: a 60,000-digit life, then a fresh
#                 linear reader; validation, seeds 1 and 2
#   judgereport   the chosen form on the test digits, seeds 5-12, with its coin
#   parityscreen  parity of 3: what differs between the every-event arm and the closed loop,
#                 a difference an arm, and each sheet read by both kinds of output; seeds 5-7
#   parity        the arms the answer leans on, seeds 5-12
#   store         competitive learning beside the loop, two stores and one, three rows
#   smoke         one seed, short lives, every kind of run (use with --quick)
# Map columns (--cols a,b): alone, loop, loopw, alonej1..4, loopj1..4, loopj1t..4t (the
# cells hear only the pressed line on bad), loopj1g..4g (nothing on bad), alonecomp,
# loopcomp, loopcomp1, loopbyte, loopbytecomp. Told the right line, a column is not run on
# the acting rows (bandit, chain, cartpole, catch): there the world's "right" is the
# scripted player's move. A good-or-bad column reads the told life on the two-button rows
# that `same` checks.
# Examples:
#   scripts/loopmap.sh map                       run what is missing, print the table
#   scripts/loopmap.sh smoke --quick             short lives: a check of the whole line
#   scripts/loopmap.sh map --table-only --detail     with each row's twin and spread
#   scripts/loopmap.sh judgescreen --rows mnist,echo16 --cols alone,alonej2
#   scripts/loopmap.sh parityscreen --arms born,old,inold --seeds 5-8
# Options: --seeds 5-8, --jobs 3, --rows a,b and --cols a,b and --detail (map recipes),
# --arms a,b, --coins a,b|none, --tests mnist,pair, --quick, --table-only.
# Map lives are cached in out/oneset/cache-loopmap.jsonl (beside the caches of
# scripts/oneset.sh, which are read too); the rest in out/loopmap/NAME/. Finished runs are
# kept. LOOPMAP_BUILD picks another build folder.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || [ -n "${THONK_MNIST:-}" ] || scripts/get_mnist.sh
[ -n "${LOOPMAP_BUILD:-}" ] || scripts/build.sh --target blobmnist --target blobprobe --target blobrun >/dev/null
exec python3 scripts/loopmap.py "$@"
