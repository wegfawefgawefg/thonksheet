#!/usr/bin/env bash
# blobgpu: many minimal-engine lives at once on the GPU (src/gpu/, shaders/gpu/,
# docs/results-2026-11-25-gpu.md). Headless only: no window, nothing on the display. Every
# life is bit-identical to the CPU engine with --m.rng_mode 1 (keyed random words).
#
#   scripts/gpu.sh                  a small safe batch: 64 lives of echo k=2, 2,000 steps
#   scripts/gpu.sh check            the shader against the CPU engine, tick by tick: nine cases
#                                   on lavapipe (software Vulkan) first, then on the GPU
#   scripts/gpu.sh run --lives 4096 --task shuffled-echo --steps 40000 --twin
#                                   any batch; blobrun's flags (--task --k --steps --ticks
#                                   --window --g.name --m.name ...); rows to out/gpu/lives.csv
#   scripts/gpu.sh settings in.csv out.csv [flags]
#                                   a population: one life per row of in.csv (columns: seed and
#                                   any minimal number, e.g. noise_shift, or g.name), scores
#                                   (late, early, twin_late, twin_early) per row to out.csv
#   scripts/gpu.sh speed            the speed table: 1, 64, 1,024, 16,384 lives, learning on and
#                                   off, then the CPU engine on one core, same task (a few min)
#   scripts/gpu.sh big              16,384 lives of echo k=2 with twins, 40,000 steps (the
#                                   search's life; 40 minutes on the RX 9070 XT, 10.8 GB)
#
# Tasks on the GPU: echo, shuffled-echo, delayed-echo, both-not-either. The plain minimal
# engine only (no far slots, delays, evidence counters, drop_mode 1 or noise_pair).
# Base flags as scripts/minimal.sh: 20 ticks a step, 3x3 sensor and motor groups.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobgpu blobrun >/dev/null

base=(--ticks 20 --sensor-side 3 --motor-side 3)
gpu() {
    env -u DISPLAY -u WAYLAND_DISPLAY nice -n 19 build/blobgpu "$@"
}
lavapipe() {
    env -u DISPLAY -u WAYLAND_DISPLAY VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json \
        LP_NUM_THREADS=2 nice -n 19 build/blobgpu --device lavapipe --validate "$@"
}
kernel_log() {
    journalctl -k --since "${1:-10 minutes ago}" 2>/dev/null | grep -i amdgpu || true
}

check_all() {
    local run=$1 b=("${base[@]}" --window 100 --steps 300)
    mkdir -p out/gpu
    printf 'seed,grade_mode,budget_on,noise_shift,learn_shift,noise_sign,cowire_on,cowire_needs_input,credit_ticks,avg_shift,up_shift,g.inhibitory_percent,budget\n1,0,1,9,0,1,1,1,20,5,4,25,48\n2,1,0,7,1,-1,1,0,10,3,3,10,96\n3,0,1,11,2,1,0,1,30,8,5,40,24\n4,1,1,4,0,1,1,1,5,0,2,25,200\n' \
        >out/gpu/settings-check.csv
    one() {
        local name=$1
        shift
        printf '%-18s ' "$name"
        $run --check --out out/gpu/check-$name.csv "$@" 2>&1 |
            grep -E "^check|validation messages|blobgpu:" | sed 's/: [0-9]* ticks.*, \([0-9]* validation\)/: \1/' |
            tr '\n' ' '
        echo
    }
    one echo2 --lives 2 --task echo --k 2 "${b[@]}"
    one shuffled-twin --lives 2 --seed-from 5 --task shuffled-echo --twin "${b[@]}"
    one settings-echo4 --settings out/gpu/settings-check.csv --task echo --k 4 "${b[@]}"
    one settings-delayed --settings out/gpu/settings-check.csv --task delayed-echo "${b[@]}"
    one settings-both --settings out/gpu/settings-check.csv --task both-not-either "${b[@]}"
    one rest-offset --lives 2 --task echo --k 3 --cue-offset 1 --rest-ticks 3 \
        --reward-ticks 4 "${b[@]}"
    one tiles16 --lives 2 --tiles 16 --task echo --k 2 --ticks 8 --steps 400 --window 100
    one workgroup64 --lives 3 --workgroup 64 --task echo --k 2 "${b[@]}"
    one workgroup1024 --lives 2 --workgroup 1024 --task both-not-either "${b[@]}"
}

case "${1:-small}" in
small)
    gpu --lives 64 --task echo --k 2 "${base[@]}" --steps 2000 --window 100 --twin \
        --out out/gpu/small.csv
    kernel_log
    ;;
check)
    echo "lavapipe:"
    check_all lavapipe
    echo "GPU:"
    check_all "gpu --validate"
    kernel_log
    ;;
run)
    shift
    gpu "${base[@]}" "$@"
    kernel_log
    ;;
settings)
    gpu --settings "$2" --out "$3" "${base[@]}" "${@:4}"
    kernel_log
    ;;
speed)
    b=(--task echo --k 2 "${base[@]}" --steps 2000 --window 100 --out out/gpu/speed.csv)
    for lives in 1 64 1024 16384; do
        for learning in 1 0; do
            echo "GPU, $lives lives, learning $learning:"
            gpu --lives $lives --m.learning_on $learning "${b[@]}" | grep -E "^gpu|^batch|^births"
        done
    done
    for learning in 1 0; do
        echo "CPU engine, one core, 4 lives, learning $learning (today's words, then keyed):"
        nice -n 19 build/blobgpu --device cpu --lives 4 --m.learning_on $learning "${b[@]}" | grep "^cpu"
        nice -n 19 build/blobgpu --device cpu --lives 4 --m.learning_on $learning --m.rng_mode 1 \
            "${b[@]}" | grep "^cpu"
    done
    echo "load: $(cut -d' ' -f1-3 /proc/loadavg)"
    kernel_log "30 minutes ago"
    ;;
big)
    gpu --lives 16384 --twin --task echo --k 2 "${base[@]}" --steps 40000 --window 1000 \
        --out out/gpu/big.csv --windows out/gpu/big-windows.csv
    kernel_log "2 hours ago"
    ;;
*)
    sed -n '2,24p' "$0"
    exit 1
    ;;
esac
