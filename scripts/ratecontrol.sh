#!/usr/bin/env bash
# A rate control that does not fight the rest (idea 175; queue item 2k;
# docs/results-2026-11-26-rate-control.md). Each candidate is genome numbers (all off by
# default; the list is CANDIDATES in scripts/ratecontrol.py) scored on five tests beside the
# standard control (std):
#   alive  160,000-step lives (echo at 3 tiles, delayed echo of 2) and a 20,000-digit MNIST
#          life with an input gap of 2,000 ticks, each under the base output rule, base with
#          competitive learning and the one setting (blobprobe ratehealth); stage 0 (blobbench)
#   read   the standard read of the born sheet, MNIST and the pair test, --twice 1 (repeat.py)
#   latch  the hand-made latch (blobprobe looptrace --loop-strong 1) lived and read under the
#          candidate; delayed echo at 2, 3, 4, 8 steps with competitive learning living under it
#   pred   the prediction sheet's rate (side 2, wide lattice, rule on; loops_probe.py)
#   map    the quick map (scripts/oneset.py) for base, a2rucomp and best with the flags added
#   scripts/ratecontrol.sh ac                    every test for one candidate (default seeds:
#                                                read 5-12, the rest 5-8)
#   scripts/ratecontrol.sh a,c,d --tests read,latch,pred    some tests, several candidates
#   scripts/ratecontrol.sh ac --tests map --seeds 5-12      the quick map on the report seeds
#   scripts/ratecontrol.sh table                 the five-test table of every candidate, from
#                                                what has run (also: NAME --table-only)
#   scripts/ratecontrol.sh smoke --quick         std and ac through every test, tiny, seed 5
# Options: --seeds 5-8, --jobs 3 (this PC: at most 3), --tests a,b, --quick, --table-only.
# Runs in out/ratecontrol/ (out/ratecontrol-quick/ for --quick), quick-map lives in
# out/oneset/cache*.jsonl; finished runs are kept and a line run again only runs what is
# missing. The standard control's traces and lives are copied from the loops, timerule and
# repeatability jobs' outputs (the same code; off is byte-identical).
# On stevejobs: MINI_DIR=thonk-ratecontrol scripts/mini.sh bg rc 'scripts/ratecontrol.sh a --tests alive --jobs 2'
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun --target blobprobe --target blobmnist --target blobbench >/dev/null
[ -d data/mnist ] || [ -n "${THONK_MNIST:-}" ] || echo "no data/mnist: scripts/get_mnist.sh" >&2
exec python3 scripts/ratecontrol.py "$@"
