#!/usr/bin/env bash
# Motor cells regulated apart from hidden cells, and a chain row a coin cannot pass (idea 184;
# queue item 2n; docs/results-2026-11-26-motor-apart.md). Every part runs with flags: the new
# genome numbers (motor_apart, motor_target_per_1024, motor_bias_max, threshold_cap_motor,
# threshold_down_shift_motor) are off by default and no default is touched.
#   screen  part 1: quick-map lives (mnist, echo2, bandit, xor, far3) under base, a2rucomp and
#           best through `blobprobe capsplit`, one arm each way of regulating motor cells:
#           the life's online score, hidden and motor rates, silent hidden cells, the bias at
#           either limit. Arms (scripts/motorapart.py has the flags): std; out1 (motor cells
#           out of the sector's count); out2 (and out of its bias); own (a bias per kind of
#           cell); own128 (their own bias and walk toward 128); walk128 (out of the count,
#           their own walk: target 128, cap 1000); walkonly (that walk alone); m64, out1h,
#           c1000 (the cap by kind of cell)
#   reader  part 1: the fresh reader after such a life (MNIST: `blobmnist ceiling`; the toy
#           rows: `blobprobe transfer --keep-output 1`)
#   map     part 2: the whole quick map for base, a2rucomp and best with an arm, seeds 5-12,
#           against the cached cap-64 lives
#   inside  part 2: the inside-learning test (born, competitive learning, the one-hop rule
#           with 12-bit every-event counters) on MNIST and the pair test, with the firing rate
#   alive   long lives under an arm (scripts/ratecontrol.sh's alive test, `blobprobe ratehealth`)
#   chain   part 3: the chain with two, three and four buttons scored on the opening press
#           (`--chain-first 1`), under base, a2rucomp, best and the critic at cap 64 and 1000
#   lean    part 4: three buttons at cap 1000, the opening press under the one setting, it
#           less each of its pieces, and the base rule with each piece, seeds 1-12
#   speed   blobbench before and after, and with each kind on
#   scripts/motorapart.sh screen                       one part (also: screen,reader)
#   scripts/motorapart.sh screen --arms std,out1 --rows mnist --cols best --seeds 5-12
#   scripts/motorapart.sh map --arms out1              the map with that arm
#   scripts/motorapart.sh table                        every table from what has run
#   scripts/motorapart.sh smoke --quick                every part, tiny, seed 5 (out/motorapart-quick/)
# Options: --seeds 5-8, --jobs 3 (this PC: at most 3), --quick, --table-only, --arms, --rows,
# --cols, --skip-std (inside: only the arms; a machine without the earlier runs to copy).
# Outputs in out/motorapart/ (look/, reader/, chain/, lean/, inside/, alive/, table-PART.md);
# quick-map lives in out/oneset/cache.jsonl. A finished run is kept; a line run again only
# runs what is missing.
# On stevejobs: MINI_DIR=thonk-motorapart scripts/mini.sh bg ma 'THONK_MNIST=$HOME/thonk-hop2/data/mnist scripts/motorapart.sh inside --arms out1 --skip-std --jobs 3'
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun --target blobprobe --target blobmnist --target blobbench >/dev/null
[ -d data/mnist ] || [ -n "${THONK_MNIST:-}" ] || echo "no data/mnist: scripts/get_mnist.sh" >&2
exec python3 scripts/motorapart.py "$@"
