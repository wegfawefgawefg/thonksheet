#!/usr/bin/env bash
# Making the learned loops hold as long as the task needs (idea 172; queue item 2i;
# docs/results-2026-11-26-loops.md). Competitive learning (--g.compete_on 1 --g.tile_winners 4
# --g.compete_shift 5) builds a loop of cells per cue that holds it about two steps. Part A:
# what ends it. Part B: the smallest change that holds it.
#   scripts/loops.sh trace           A: blobprobe looptrace after a delayed-echo life with the
#                                    rule, each suspect switched after the life (the wiring is
#                                    the life's) or the rule living under it; seeds 5-8
#   scripts/loops.sh suspects        A: delayed echo at 2, 3, 4, 8, 16 steps with each suspect
#                                    switched after a normal life (blobprobe transfer), and
#                                    with the rule living under it (blobrun); seeds 5-8
#   scripts/loops.sh pred            the rate on the prediction battery's sheet (side 2, wide
#                                    sensor lattice): thresholds and sector bias; seeds 5-8
#   scripts/loops.sh best            B: the change on every time and memory row and the rows
#                                    that must not break; rule off, churn, at target rate;
#                                    seeds 5-12 (--change leak, the default, or leak56)
#   scripts/loops.sh runner --change leak56    B: the runner-up, rule on and at target rate
#   scripts/loops.sh controls        B: frozen read, time-shuffled life, churn (transfer)
#   scripts/loops.sh long            B: 160,000-step lives with the change; seeds 5-8
#   scripts/loops.sh mnist           B: the MNIST inside read (scripts/inside.sh) of
#                                    competitive learning with and without the change
#   scripts/loops.sh all --quick     every mode but mnist on 2 seeds, short lives: a smoke check
#   scripts/loops.sh best --table-only    print a table from finished runs
# Options: --seeds 5-12, --jobs 3 (this PC: at most 3), --change leak|leak56, --arms a,b
# (launch only these arms; the table shows what is there); anything else goes to every run.
# Runs and tables in out/loops/ (out/loops-quick/ for --quick). Finished runs are kept. best
# copies the plain arms' lives of the timerule job from out/from-agents/timerule/lives when
# they are there (the same flags on the same code). The changes are genome numbers:
#   leak       --g.leak_shift_min 4 --g.leak_shift_max 6
#   leak56     --g.leak_shift_min 5 --g.leak_shift_max 6
# On stevejobs: MINI_DIR=thonk-loops scripts/mini.sh bg lp 'scripts/loops.sh best --jobs 2'
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun --target blobprobe >/dev/null
exec python3 scripts/loops.py "$@"
