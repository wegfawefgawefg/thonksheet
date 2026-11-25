#!/usr/bin/env bash
# Competitive learning followed up (queue item 2a; docs/results-2026-11-25-compete2.md). Every
# table of that doc from one line. The rule is idea 158's best setting, B5:
# --g.learn_mode 5 --g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5 (and A5, the same
# without the tile's winners); each with the rate held at target (--g.threshold_cap 1000) in
# the life and the read, or not.
#   mnist: a fresh linear reader of the frozen sheet (scripts/inside.sh's test), seeds 5-12:
#     born, lived, shuffled (the life on digits whose pixels are moved afresh each digit),
#     churn (the same moves toward random sources, --g.compete_churn 1), and the random
#     projection per seed. Lived minus each control per seed, with the read rate.
#   calc: the calculation battery's held-out columns (xor4, add4), the same four kinds; the
#     shuffled life moves each trial's input lines by a fresh permutation.
#   hopcomp: the one-hop rule beside competitive learning on calc (a taught life), split into
#     its parts (tile winners, competition, both), seeds 5-8.
#   time: the suite's delayed echo, order and gap rows, rule on against off (blob-readout's
#     readout with the tile's winners either way), seeds 5-12.
#   speed: blobbench and an MNIST life, rule off, on, and on re-picking every update.
#   all: mnist, calc, hopcomp and time.
#   scripts/compete2.sh mnist                    the MNIST tables (128 runs, about 1.5 h here)
#   scripts/compete2.sh calc                     (256 runs, about 40 min)
#   scripts/compete2.sh hopcomp
#   scripts/compete2.sh time
#   scripts/compete2.sh speed
#   scripts/compete2.sh all --quick              2 seeds, short lives: a smoke check
#   scripts/compete2.sh mnist --settings b5cap,a5cap --seeds 5-8    part of it
#   scripts/compete2.sh mnist --table-only       print what is there
# Options: --seeds 5-12, --settings b5,b5cap,a5,a5cap, --kinds born,lived,shuffled,churn,
#   --members xor4,add4, --jobs 3. Finished runs are kept. Results and tables in
#   out/compete2/ (out/compete2-quick/ for --quick). On stevejobs:
#   MINI_DIR=thonk-compete2 scripts/mini.sh bg c2 'scripts/compete2.sh mnist --jobs 2'
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f data/mnist/t10k-labels-idx1-ubyte ] || scripts/get_mnist.sh
scripts/build.sh --target blobmnist --target blobprobe --target blobrun --target blobbench >/dev/null
exec python3 scripts/compete2.py "$@"
