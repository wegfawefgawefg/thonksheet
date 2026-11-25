#!/usr/bin/env bash
# What keeps the vm's walk from collapsing into a loop (docs/results-2026-11-25-vm-route.md):
# each piece switched off in turn; vmprobe --walk at birth and after a 10,000-step life of
# delayed echo and of order (seeds 1, 2), and the share of presses right while learning.
# Two jobs at a time. Outputs in out/vm/ablate/.
#   scripts/vm_ablate.sh
cd "$(dirname "$0")/.."
mkdir -p out/vm/ablate
P="nice -n 19 ./build/vmprobe --ticks 20 --sensor-side 3 --motor-side 3 --show 0 --walk"
flags_of() {
    case "$1" in
    default) echo "" ;;
    one-winner) echo "--v.active 1" ;;
    no-refract) echo "--v.refract 0" ;;
    no-homeostat) echo "--v.homeostat 0" ;;
    weak-input) echo "--v.addr_x_shift 7" ;;
    no-events) echo "--v.event_on 0" ;;
    no-answers) echo "--v.write_on_bad 0 --v.write_on_good 0" ;;
    no-route) echo "--v.event_route 0" ;;
    no-settle) echo "--v.event_settle 0" ;;
    learning-off) echo "--v.learning_on 0" ;;
    esac
}
list=out/vm/ablate/jobs.txt
: > "$list"
for name in default one-winner no-refract no-homeostat weak-input no-events no-answers no-route \
    no-settle learning-off; do
    f=$(flags_of "$name")
    echo "$P --task delayed-echo --delay 2 --steps 1 --window 1 --test-steps 0 $f > out/vm/ablate/$name-birth.txt" >> "$list"
    for s in 1 2; do
        echo "$P --task delayed-echo --delay 2 --steps 10000 --test-steps 2000 --seed $s $f > out/vm/ablate/$name-delay2-s$s.txt" >> "$list"
        echo "$P --task order --steps 10000 --test-steps 2000 --seed $s $f > out/vm/ablate/$name-order-s$s.txt" >> "$list"
    done
done
tr '\n' '\0' < "$list" | xargs -0 -P 2 -I{} sh -c '{}'
