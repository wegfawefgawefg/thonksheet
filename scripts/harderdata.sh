#!/usr/bin/env bash
# A harder real-data test with more room than MNIST (idea 185;
# docs/results-2026-11-26-harder-data.md). Other data sets in MNIST's own file format
# (scripts/get_data.sh: fashion, kmnist, kannada) go through scripts/mnist.sh's sheet and
# reader with nothing changed but the data folder. For each set and each test (class: the
# ten classes; pair: are two interleaved half-size images the same class) it finds the born
# sheet, the plain learners on the bits, the backprop teacher's bound written into the real
# sheet (a measuring stick, never the method), competitive learning, and the live every-event
# one-hop rule (scripts/everyevent.sh's arms out, outf3, outint2) with its coin. NAME is a
# fixed recipe:
#   room      born, the plain learners and the teacher's bound; no lives
#   screen    20,000-digit lives of competitive learning and the rule, no coins
#   full      everything, 60,000-digit lives, with coins
#   quiet     born and the teacher's wiring read the quiet way (--read-freeze 3 --ticks 80)
#   smoke     every part on one set (use with --quick)
# Examples:
#   scripts/harderdata.sh full --sets kmnist             the second real-data test
#   scripts/harderdata.sh room                           mnist, fashion, kmnist; seeds 5,6,7
#   scripts/harderdata.sh screen --sets fashion,kmnist --tests pair
#   scripts/harderdata.sh full --sets kmnist --parts born,rule --seeds 5-12 --jobs 2
#   scripts/harderdata.sh full --table-only              print what is there
#   scripts/harderdata.sh smoke --quick                  short lives: a check of the whole line
# Options: --sets mnist,fashion,kmnist,kannada  --tests class,pair  --seeds 5-7  --jobs 3
#   --parts bars,born,teacher,compete,rule,quiet  --life N  --coins 0|1  --quick
#   --arms out,outf3,outint2 (the rule's arms: any arm of scripts/everyevent.py)
#   --table-only  --reverse (the job list from its end, for a second machine).
# Results in out/harderdata/run/SET/ (run-quick with --quick), one file a run; a finished run
# is kept, so a line run again only runs what is missing. The tables go to
# out/harderdata/run/table-NAME.txt. The teacher needs torch: LADDER_VENV or
# out/ladder2/venv (scripts/ladder2.sh makes it). One thread a job.
set -euo pipefail
cd "$(dirname "$0")/.."
venv=${LADDER_VENV:-out/ladder2/venv}
if [ ! -x "$venv/bin/python" ]; then
    echo "no torch venv at $venv: run scripts/ladder2.sh smoke --quick once, or set LADDER_VENV"
    exit 1
fi
# the sets asked for (default: the recipe's), fetched if missing
sets="mnist fashion kmnist"
[ "${1:-}" = smoke ] && sets=fashion
args=("$@")
for i in "${!args[@]}"; do
    [ "${args[$i]}" = --sets ] && sets="${args[$((i + 1))]//,/ }"
done
for s in $sets; do
    [ "$s" = mnist ] && [ -n "${THONK_MNIST:-}" ] && continue
    [ -f "data/$s/t10k-labels-idx1-ubyte" ] || scripts/get_data.sh "$s"
done
scripts/build.sh --target blobmnist >/dev/null
export OMP_NUM_THREADS=1 MKL_NUM_THREADS=1
exec nice -n 19 "$venv/bin/python" scripts/harderdata.py "$@"
