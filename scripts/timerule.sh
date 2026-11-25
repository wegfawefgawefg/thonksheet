#!/usr/bin/env bash
# Why competitive learning lifts the time rows, and how far it goes (idea 166; queue item 2f;
# docs/results-2026-11-25-timerule.md). The rule is idea 158's B5: --g.compete_on 1
# --g.tile_winners 4 --g.compete_shift 5, left on for the whole life beside the readout
# (--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1).
# Every table of the doc from one line:
#   scripts/timerule.sh repro        the suite's three time rows and the prediction battery's
#                                    delayed echo, rule off and on, seeds 5 and 6
#   scripts/timerule.sh walk         from the suite's sheet to the prediction battery's, one
#                                    difference at a time (sensor side, lattice, motor spread,
#                                    delay, tile winners in the off arm, life length), off /
#                                    on / churn, seeds 5-8
#   scripts/timerule.sh walk8        the cells that carry the answer (and 3 steps) on 8 seeds
#   scripts/timerule.sh controls     the freeze, time-shuffled, reward-free, churn and
#                                    target-rate arms (blobprobe transfer), seeds 5-12
#   scripts/timerule.sh wiring       what the cells wired (blobprobe timewire), seeds 5-8
#   scripts/timerule.sh reach        the map and time rows, rule off / on / churn / on at
#                                    target rate, seeds 5-12, the GRU's number beside
#   scripts/timerule.sh readout      the same rows with the output-gap readout, off and on
#   scripts/timerule.sh all --quick  2 seeds, lives a tenth as long: a smoke check
#   scripts/timerule.sh walk --table-only        print the table from finished runs
# Options: --seeds 5-12, --jobs 3 (this PC: at most 3); anything else goes to every run.
# Runs and tables in out/timerule/ (out/timerule-quick/ for --quick). Finished runs are kept.
# On stevejobs: MINI_DIR=thonk-timerule scripts/mini.sh bg tr 'scripts/timerule.sh walk --jobs 2'
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun --target blobprobe >/dev/null
exec python3 scripts/timerule.py "$@"
