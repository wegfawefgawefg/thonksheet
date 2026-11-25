#!/usr/bin/env bash
# The calculation battery (ideas 133, 134): worlds whose answer is several bits at once
# (binary arithmetic, bit operations, an ALU, a register machine, lookup and sorting,
# geometry), each with some input cases held out and never rewarded. Every member is scored
# by the offline linear reader on the sheet as born, by the working readout rule with a
# verdict per bit and with one verdict for the whole answer, with the sheet learning too,
# and by the minimal engine and table neurons; beside the learning-off twin and coin flips.
# Prints one table and writes it to out/calc/<name>/table.txt.
#   scripts/calc.sh main                         everything, seeds 5 to 12, 2 runs at a time
#   scripts/calc.sh main --quick                 2 seeds, short lives: a smoke check
#   scripts/calc.sh main --only arith,program    some families (or member names)
#   scripts/calc.sh main --configs probe,bit     some ways of scoring (probe bit whole good
#                                                m6 minimal table)
#   scripts/calc.sh main --table-only            print what is there, run nothing
#   scripts/calc.sh main --jobs 4                more at once (the second machine)
# Finished runs are kept: running it again only runs what is missing. Anything else on the
# line goes to every call (blobrun flags, --g.name value).
# One life: scripts/task.sh calc --calc add --calc-bits 4 --calc-grade bit --motor-spread 128
#   --g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun --target blobprobe
exec python3 scripts/calc.py "$@"
