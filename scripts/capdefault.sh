#!/usr/bin/env bash
# The threshold cap lifted: decide the default (idea 180; queue item 2m;
# docs/results-2026-11-26-cap-default.md). Five parts, each run with flags (the default in
# genome.hpp is not touched):
#   chain   the two-step chain looked inside: quick-map chain lives through `blobprobe
#           capsplit`, which caps motor cells and the rest apart and counts what is pressed
#           on each stage. Arms: cap 64, cap 1000, the cap lifted on motor cells only, on the
#           rest only, motor thresholds held as born, the rate held by the sector bias, three
#           and four buttons, no stage line; base, a2rucomp, best (and the critic), seeds 5-12
#           and 1-4
#   rate    MNIST online and reach (far3, far5) the same way: cap 64, cap 1000, cap 1000 at a
#           higher target (64, 128), cap 64 with the rate held by the sector bias, the cap by
#           kind of cell; with the rate each life really ran at (sensor cells left out)
#   map     the quick map (scripts/oneset.py) for base, a2rucomp and best at cap 64 and cap
#           1000 (and each fix named in FIXES of capdefault_more.py), seeds 5-12; the chain,
#           cartpole and noisy rows on seeds 1-4
#   inside  the inside-learning test (blobmnist ceiling), MNIST and the pair test, for the born
#           sheet, competitive learning and the one-hop rule at both caps, 8 seeds, with the
#           firing rate; runs the earlier jobs made with the same flags are copied
#   speed   blobbench with the cap at 64 and at 1000, taking turns
#   scripts/capdefault.sh chain                  one part (also: chain,rate)
#   scripts/capdefault.sh rate --arms c64t64 --rows mnist --cols a2rucomp    part of the rate part
#   scripts/capdefault.sh table                  every table from what has run
#   scripts/capdefault.sh smoke --quick          every part, tiny, seed 5 (out/capdefault-quick/)
# Options: --seeds 5-8, --jobs 3 (this PC: at most 3), --quick, --table-only, and for the
# rate part --arms, --rows, --cols.
# Outputs in out/capdefault/ (look/ the probe's runs, inside/ the reads, table-PART.md);
# quick-map lives in out/oneset/cache*.jsonl. A finished run is kept; a line run again only
# runs what is missing.
# On stevejobs: MINI_DIR=thonk-capdefault scripts/mini.sh bg cd 'THONK_MNIST=$HOME/thonk-hop2/data/mnist scripts/capdefault.sh inside --jobs 2'
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun --target blobprobe --target blobmnist --target blobbench >/dev/null
[ -d data/mnist ] || [ -n "${THONK_MNIST:-}" ] || echo "no data/mnist: scripts/get_mnist.sh" >&2
exec python3 scripts/capdefault.py "$@"
