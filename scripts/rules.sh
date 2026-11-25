#!/usr/bin/env bash
# Idea 120: the learning rule as a table. Results: docs/results-2026-11-25-rule-tables.md
#
# One life with a rule file, beside its baselines (anything after the file overrides):
#   scripts/rules.sh life rules/error.rule echo --k 4            button cells only (rule_on 3)
#   scripts/rules.sh life rules/old.rule echo --g.rule_on 1      the table on every neuron
#   scripts/rules.sh inside rules/old.rule order                 the error-rule readout at the
#                                                                buttons, the table inside
# A rule file as plain sentences, and two tables compared:
#   scripts/rules.sh print rules/error.rule
#   scripts/rules.sh diff rules/error.rule out/rules/search/frozen-hand/tables/ID.rule
# The three hand-written rules as tables against their originals (seeds 5-12, 2 at a time):
#   scripts/rules.sh reproduce
# The rule search (scripts/rule_search.py; modes frozen, whole, inside; init hand or random):
#   scripts/rules.sh search inside-hand --mode inside --init hand --gens 20 --jobs 2
#   scripts/rules.sh status inside-hand
#   scripts/rules.sh show inside-hand 3                          the best 3 as sentences
#   scripts/rules.sh confirm inside-hand 4                       the best 4 on seeds 5-12
#   scripts/rules.sh confirm inside-hand 2 --steps 40000         the same, full-length lives
# On the second machine (its own folder; at most 4 jobs there):
#   MINI_DIR=thonk-rules scripts/mini.sh push
#   MINI_DIR=thonk-rules scripts/mini.sh bg inside-hand 'python3 scripts/rule_search.py run --name inside-hand --mode inside --init hand --jobs 2'
#   MINI_DIR=thonk-rules scripts/mini.sh pull                    results land in out/mini/rules/
set -euo pipefail
cd "$(dirname "$0")/.."
what="${1:-}"
[ $# -gt 0 ] && shift
STD="--ticks 20 --sensor-side 3 --motor-side 3 --motor-spread 128 --steps 40000 --window 1000"
READOUT="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1"
case "$what" in
life | inside)
    file="$1"
    task="${2:-echo}"
    shift 2 || shift $#
    scripts/build.sh --target blobrun
    if [ "$what" = life ]; then
        exec nice -n 19 ./build/blobrun --task "$task" $STD --g.rule_on 3 --rule-file "$file" "$@"
    fi
    exec nice -n 19 ./build/blobrun --task "$task" $STD $READOUT --g.rule_on 2 --rule-file "$file" "$@"
    ;;
print)
    exec python3 scripts/rule_table.py print "$@"
    ;;
diff)
    exec python3 scripts/rule_table.py diff "$@"
    ;;
reproduce)
    scripts/build.sh --target blobrun
    LIFE="--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000"
    sw() { python3 scripts/sweep.py --jobs 2 --seeds 5-12 --out-dir out/rules/sweep "$@"; }
    for t in "echo k=2|--task echo --k 2" "echo k=4|--task echo --k 4" \
        "exclusive-or|--task both-not-either"; do
        name="${t%%|*}"
        args="${t#*|}"
        sw --name "$name, error rule" --variants scripts/variants/rules-error.txt -- $args $LIFE \
            --motor-spread 128
        sw --name "$name, old rule" --variants scripts/variants/rules-old.txt -- $args $LIFE
        sw --name "$name, growth" --variants scripts/variants/rules-grow.txt -- $args $LIFE
    done
    ;;
search)
    name="$1"
    shift
    scripts/build.sh --target blobrun
    exec python3 scripts/rule_search.py run --name "$name" "$@"
    ;;
status)
    exec python3 scripts/rule_search.py status --name "$1"
    ;;
show)
    exec python3 scripts/rule_search.py show --name "$1" --top "${2:-3}"
    ;;
confirm)
    name="$1"
    top="${2:-4}"
    shift 2 || shift $#
    exec python3 scripts/rule_search.py confirm --name "$name" --top "$top" "$@"
    ;;
*)
    sed -n '2,24p' "$0"
    exit 1
    ;;
esac
