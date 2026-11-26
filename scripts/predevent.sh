#!/usr/bin/env bash
# Idea 183, prediction errors through the every-event one-hop rule
# (docs/results-2026-11-26-predict-every-event.md). Idea 164 (scripts/predhop.sh) sent the
# next-step predictor's wrong answers to every hidden cell and its counters stepped with a
# small chance. Here the counters are 12-bit planes that step on every marked input
# (--g.hop_bits 12 --g.hop_shift 0), each prediction line has a fine zone
# (--g.hop_pred_fine F: no teaching when the answer was right by that margin), and the
# errors of the far horizons teach too (--predict-hop is a set of horizons). One blobprobe
# predict run per world, arm and seed on idea 164's sheet: a life, freeze, then a fresh
# linear reader of the hidden cells for what just changed, the right button, and every input
# line 1 and 8 steps on.
# Arms (any list with --arms):
#   born, pred (the rule off), old (idea 164's rule)
#   ev<set>f<fine>   every-event counters taught by the horizons in <set> (bits over
#                    1,2,4,8: 1 the next step alone, 15 all four, 14 the far three) with a
#                    fine zone of <fine> 32nds (0: wrong answers only)
#   letters after it: o<M> the errors come from a whole-number predictor outside the sheet
#            with a margin of M steps (a measuring stick; predo2: it learns beside the
#            rule-off life), n near-not-fired marks (--g.hop_fired 0), b the buttons'
#            errors too
#   endings: c its coin control (the errors of another step), s the shuffled-frame life,
#            k the rate held at target (--g.threshold_cap 1000; bornk, predk too)
#   scripts/predevent.sh smoke --quick                    every part, 1 seed, short lives
#   scripts/predevent.sh screen --seeds 1,2 --worlds catch,forage --arms born,pred,old,ev1f0,ev1f8
#   scripts/predevent.sh time --seeds 5-12 --worlds delayed-echo,predict-next --arms born,predo2,ev15f0o2,ev15f0o2c,ev15f0o2s
#   scripts/predevent.sh time --table-only --as time8 --seeds 5-12 ...   tables from finished runs, saved as time8.md
#   scripts/predevent.sh task1 --base one --worlds catch,forage,order,delayed2 --arms predo2,ev14f0o2
#   scripts/predevent.sh moved movedtest --test --seeds 5-12 --arms born,moved,movedc,moveds
# --worlds all is idea 164's ten; order, gap, delayed2 are rows of the quick map with time in
# them. --base predhop (default: idea 164's button rule), one (the one setting's button
# rule), onenw (the same without the backward walk). --steps N (20000), --trials N (3000),
# --jobs N (3). Anything else on the line goes to every run (later wins), e.g. --read-own 1
# (the read's moves are the frozen sheet's own buttons, not a coin).
# Writes out/predevent/NAME/ (a log and csv per run), NAME.md and NAME.csv. Finished runs are
# kept: running a line again only runs what is missing.
# moved NAME (scripts/predmoved.py, part 4): MNIST lives in which each digit is shown for 3
# steps, a pixel further each step, and an outside predictor's next-frame errors teach the
# cells; then the usual frozen read on the usual labels. Arms born, off (the rule off),
# moved, movedc (coin), moveds (pixels shuffled every step), a k after any for the rate held
# at target; --test for the test digits (default: the validation slice), --life N digits.
set -euo pipefail
cd "$(dirname "$0")/.."
mnist_here() { [ -d data/mnist ] || [ -n "${THONK_MNIST:-}" ]; }
if [ "${1:-}" = moved ]; then
    mnist_here || { echo "no data/mnist: scripts/get_mnist.sh" >&2; exit 1; }
    scripts/build.sh --target blobmnist >/dev/null
    shift
    exec python3 scripts/predmoved.py "$@"
fi
scripts/build.sh --target blobprobe >/dev/null
if [ "${1:-}" = smoke ]; then
    shift
    python3 scripts/predevent.py smoke "$@" --worlds catch,delayed-echo \
        --arms born,pred,old,ev1f0,ev15f8,ev15f0o2,ev15f0o2c,ev15f0o2s,ev14f8nk,bornk
    python3 scripts/predevent.py smoke-one "$@" --base one --worlds catch,order \
        --arms pred,ev15f0o2 --read-own 1
    if mnist_here; then
        scripts/build.sh --target blobmnist >/dev/null
        exec python3 scripts/predmoved.py smoke-moved "$@"
    fi
    exit 0
fi
exec python3 scripts/predevent.py "$@"
