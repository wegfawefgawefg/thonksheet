#!/usr/bin/env bash
# blobgpu: many minimal-engine lives at once on the GPU (src/gpu/, shaders/gpu/,
# docs/results-2026-11-25-gpu.md). Headless only: no window, nothing on the display. Every
# life is bit-identical to the CPU engine with --m.rng_mode 1 (keyed random words).
#
#   scripts/gpu.sh                  a small safe batch: 64 lives of echo k=2, 2,000 steps
#   scripts/gpu.sh check            the shader against the CPU engine, tick by tick: nine cases
#                                   of the Echo family in the shader and nine of host worlds, on
#                                   lavapipe (software Vulkan) first, then on the GPU
#   scripts/gpu.sh check host       only the host-world cases (check shader: only the others);
#                                   host worlds include the evidence counters, far slots and
#                                   delays, and the memory engine (--engine memory)
#   scripts/gpu.sh against          host worlds against blobrun --engine minimal --m.rng_mode 1
#                                   (or --engine memory) as text: six worlds, evidence, far slots,
#                                   delays, and the memory engine on five worlds; seeds 5-8 and
#                                   twins, 2,000 steps (out/gpu/against/)
#   scripts/gpu.sh run --lives 4096 --task shuffled-echo --steps 40000 --twin
#                                   any batch; blobrun's flags (--task --k --steps --ticks
#                                   --window --g.name --m.name ...); rows to out/gpu/lives.csv
#   scripts/gpu.sh settings in.csv out.csv [flags]
#                                   a population: one life per row of in.csv (columns: seed and
#                                   any minimal number, e.g. noise_shift, or g.name), scores
#                                   (late, early, twin_late, twin_early) per row to out.csv
#   scripts/gpu.sh settings in.csv out.csv --tasks echo:k=8,order,gap,catch:catch-size=4/4 --twin
#                                   every row on every world of the list, one batch: late_<world>
#                                   and twin_late_<world> per row (key=value is a blobrun flag)
#   scripts/gpu.sh settings in.csv out.csv --engine memory --tasks echo:k=8,order,gap --twin
#                                   the same for the memory engine (columns y.name)
#   scripts/gpu.sh speed            the speed table: 1, 64, 1,024, 16,384 lives, learning on and
#                                   off, then the CPU engine on one core, same task (a few min)
#   scripts/gpu.sh speed-host       every path (in the shader, host worlds, evidence, far slots,
#                                   delays, the memory engine) at 1, 64, 1,024 lives and the most
#                                   that fit, beside one CPU core (about 15 min)
#   scripts/gpu.sh big              16,384 lives of echo k=2 with twins, 40,000 steps (the
#                                   search's life; 40 minutes on the RX 9070 XT, 10.8 GB)
#
# Every task runs. The Echo family (echo, shuffled-echo, delayed-echo, both-not-either) runs
# inside the shader; every other task, and these with --host, runs as a host world: the sheet
# on the GPU, the task and plug code of src/world on the CPU, one exchange per task step. The
# minimal engine with evidence counters, far slots and delays (any --m. number; these run as
# host worlds); not drop_mode 1 or noise_pair.
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

# host worlds: the world's own code on the CPU, the sheet on the GPU, checked every step
check_host() {
    local run=$1 b=("${base[@]}" --window 50 --steps 150)
    mkdir -p out/gpu
    printf 'seed,grade_mode,budget_on,noise_shift,learn_shift,noise_sign,cowire_on,cowire_needs_input,credit_ticks,avg_shift,up_shift,g.inhibitory_percent,budget\n1,0,1,9,0,1,1,1,20,5,4,25,48\n2,1,0,7,1,-1,1,0,10,3,3,10,96\n' \
        >out/gpu/settings-host.csv
    # evidence counters: motor cells or every neuron, each read-off rule, quiet and co-activity
    # variants, and a plain row in the same batch
    printf 'seed,ev_on,ev_bits,ev_init,ev_spikes,ev_inh,ev_quiet,ev_read,ev_cowire,ev_fired_min,ev_shift,ev_quiet_shift\n1,1,6,4,1,0,1,0,1,1,0,0\n2,1,4,2,0,1,0,0,1,1,0,0\n3,2,6,4,1,0,1,0,1,1,0,0\n4,2,5,3,2,1,2,1,0,2,1,1\n5,1,3,1,1,0,1,2,1,1,0,2\n6,0,4,2,0,0,1,0,1,1,0,0\n' \
        >out/gpu/settings-evidence.csv
    # far slots and delays, alone, together and with evidence counters, and a plain row
    printf 'seed,far_slots,far_percent,far_reach,delay_max,delay_percent,far_delay_per_tile,ev_on,ev_bits,ev_init,ev_spikes,credit_ticks\n1,2,100,3,1,100,1,0,4,2,0,20\n2,0,15,3,8,100,1,0,4,2,0,20\n3,1,50,2,6,60,2,0,4,2,0,25\n4,2,100,5,15,100,3,1,6,4,1,10\n5,1,15,3,4,100,1,2,6,4,1,20\n6,0,15,3,1,100,1,0,4,2,0,20\n' \
        >out/gpu/settings-far.csv
    one() {
        local name=$1
        shift
        printf '%-18s ' "$name"
        $run --check --out out/gpu/check-$name.csv "$@" 2>&1 |
            grep -E "^check|validation messages|blobgpu:" | sed 's/: [0-9]* ticks.*, \([0-9]* validation\)/: \1/' |
            tr '\n' ' '
        echo
    }
    one host-echo --host --lives 2 --task echo --k 2 "${b[@]}"
    one host-order --lives 1 --seed-from 3 --twin --task order "${b[@]}"
    one host-gap --lives 1 --seed-from 3 --twin --task gap "${b[@]}"
    one host-both --host --lives 1 --seed-from 3 --twin --task both-not-either "${b[@]}"
    one host-catch --lives 1 --seed-from 3 --twin --task catch --catch-size 4 4 "${b[@]}"
    one host-cartpole --lives 1 --seed-from 3 --twin "${b[@]}" --task cartpole --sensor-side 2 \
        --cart-reward lean --number-code place
    one host-sequence --lives 1 --seed-from 3 --twin --task sequence "${b[@]}"
    one host-place --lives 1 --seed-from 3 --twin --task place "${b[@]}"
    one host-worlds --settings out/gpu/settings-host.csv --twin --rest-ticks 3 --reward-ticks 4 \
        --tasks echo:k=4,order,gap,catch:catch-size=4/4,cartpole:sensor-side=2:cart-reward=lean \
        "${b[@]}"
    one evidence --settings out/gpu/settings-evidence.csv --twin --rest-ticks 2 \
        --tasks both-not-either,echo:k=4 "${b[@]}"
    one far-delays --settings out/gpu/settings-far.csv --twin --rest-ticks 2 \
        --tasks echo:k=4,gap,order "${b[@]}"
    # the memory engine: the default store, the cheap one, other depths, codes and credit
    printf 'seed,y.tables,y.table_bits,y.depth,y.code,y.others_up,y.trace_across,y.predict,y.predict_bits,y.now_only,y.explore_max,y.counter_max,y.taps\n1,256,10,4,0,0,2,1,12,1,32,31,16\n2,128,8,4,0,0,2,1,12,1,32,31,16\n3,200,9,3,1,1,1,0,12,2,64,15,8\n4,64,12,2,0,0,0,1,6,0,16,63,32\n' \
        >out/gpu/settings-memory.csv
    one memory --engine memory --settings out/gpu/settings-memory.csv --twin --rest-ticks 1 \
        --tasks echo:k=4,gap,catch:catch-size=4/4,sequence "${b[@]}"
    one memory-worlds --engine memory --lives 1 --seed-from 5 --twin --rest-ticks 2 \
        --tasks echo:k=4,cartpole:sensor-side=2:cart-reward=lean,delayed-echo,order,both-not-either \
        "${b[@]}"
}

# one world, seeds 5-8 and their twins, on the GPU and in blobrun; the windows compared as text
against() {
    local name=$1
    shift
    local dir=out/gpu/against cpu_args=() engine=(--engine minimal --m.rng_mode 1)
    mkdir -p $dir
    for a in "$@"; do [ "$a" = --host ] || cpu_args+=("$a"); done
    [[ " $* " == *" --engine memory "* ]] && engine=()
    gpu --lives 4 --seed-from 5 --twin "$@" --out $dir/$name.csv --windows $dir/$name-w.csv |
        grep -E "^gpu" || true
    awk -F, 'NR > 1 { s[$1 "," $3 "," $4 "," $2] = $5 }
        END { for (k in s) { split(k, p, ","); if (p[4] == 0)
              print p[1] "," p[2] "," p[3] "," s[k] "," s[p[1] "," p[2] "," p[3] ",1"] } }' \
        $dir/$name-w.csv | sort -t, -k1,1n -k2,2n >$dir/$name-gpu.txt
    : >$dir/$name-cpu.txt
    for seed in 5 6 7 8; do
        nice -n 19 build/blobrun "${engine[@]}" --seed $seed "${cpu_args[@]}" \
            --out $dir/$name-br-s$seed.csv --alive-ticks 0 --rhythm-ticks 0 >/dev/null
        awk -F, -v s=$seed 'NR > 1 { print s "," $1 "," $2 "," $3 "," $4 }' \
            $dir/$name-br-s$seed.csv >>$dir/$name-cpu.txt
    done
    sort -t, -k1,1n -k2,2n -o $dir/$name-cpu.txt $dir/$name-cpu.txt
    if cmp -s $dir/$name-gpu.txt $dir/$name-cpu.txt; then
        echo "$name: identical as text to blobrun ($(wc -l <$dir/$name-cpu.txt) windows)"
    else
        echo "$name: DIFFERS from blobrun"
    fi
}

case "${1:-small}" in
small)
    gpu --lives 64 --task echo --k 2 "${base[@]}" --steps 2000 --window 100 --twin \
        --out out/gpu/small.csv
    kernel_log
    ;;
check)
    which=${2:-all}
    if [ "$which" != host ]; then
        echo "lavapipe, in the shader:"
        check_all lavapipe
    fi
    if [ "$which" != shader ]; then
        echo "lavapipe, host worlds:"
        check_host lavapipe
    fi
    if [ "$which" != host ]; then
        echo "GPU, in the shader:"
        check_all "gpu --validate"
    fi
    if [ "$which" != shader ]; then
        echo "GPU, host worlds:"
        check_host "gpu --validate"
    fi
    kernel_log
    ;;
against)
    b=("${base[@]}" --steps 2000 --window 100)
    against echo --host --task echo --k 2 "${b[@]}"
    against order --task order "${b[@]}"
    against gap --task gap "${b[@]}"
    against both --host --task both-not-either "${b[@]}"
    against catch --task catch --catch-size 4 4 "${b[@]}"
    against cartpole --task cartpole --cart-reward lean --number-code place "${b[@]}" \
        --sensor-side 2
    # evidence counters: settings A and C of docs/results-2026-11-24-evidence.md
    against evidence-A --task echo --k 4 "${b[@]}" --m.ev_on 1 --m.ev_bits 6 --m.ev_init 4 \
        --m.ev_spikes 1
    against evidence-C --task order "${b[@]}" --m.ev_on 2 --m.ev_bits 6 --m.ev_init 4 \
        --m.ev_spikes 1
    against far --task echo --k 4 "${b[@]}" --m.far_slots 2 --m.far_percent 100
    against delays --task gap "${b[@]}" --m.delay_max 8
    against far-delays-evidence --task order "${b[@]}" --m.far_slots 2 --m.far_percent 100 \
        --m.delay_max 6 --m.ev_on 1 --m.ev_bits 6 --m.ev_init 4 --m.ev_spikes 1
    # the memory engine, against blobrun --engine memory
    against memory-echo4 --engine memory --task echo --k 4 "${b[@]}"
    against memory-catch --engine memory --task catch --catch-size 4 4 "${b[@]}"
    against memory-cartpole --engine memory --task cartpole --cart-reward lean \
        --number-code place "${b[@]}" --sensor-side 2
    against memory-sequence --engine memory --task sequence "${b[@]}"
    against memory-delayed --engine memory --task delayed-echo "${b[@]}"
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
speed-host)
    # every path at 1, 64, 1,024 lives and the largest that fits, echo (k=4 for the grown-back
    # pieces), 500-step lives; then the CPU engine on one core, same flags (keyed words)
    b=("${base[@]}" --steps 500 --window 100 --out out/gpu/speed.csv)
    row() {
        local name=$1 largest=$2
        shift 2
        for lives in 1 64 1024 $largest; do
            printf '%-22s %6s lives: ' "$name" $lives
            gpu --lives $lives "${b[@]}" "$@" 2>/dev/null | grep -E "^gpu" |
                sed 's/.*s: \([0-9]*\) life-ticks.*/\1/'
        done
        printf '%-22s    cpu core: ' "$name"
        local cpu_args=()
        for a in "$@"; do [ "$a" = --host ] || cpu_args+=("$a"); done
        nice -n 19 build/blobgpu --device cpu --m.rng_mode 1 --lives 2 "${b[@]}" "${cpu_args[@]}" |
            grep "^cpu" | sed 's/.*ticking: \([0-9]*\) life-ticks.*/\1/'
    }
    row "shader echo k=2" 16384 --task echo --k 2
    row "host echo k=2" 16384 --host --task echo --k 2
    row "host catch 4x4" 16384 --task catch --catch-size 4 4
    row "evidence A, echo k=4" 16384 --task echo --k 4 --m.ev_on 1 --m.ev_bits 6 --m.ev_init 4 \
        --m.ev_spikes 1
    row "evidence C, echo k=4" 4096 --task echo --k 4 --m.ev_on 2 --m.ev_bits 6 --m.ev_init 4 \
        --m.ev_spikes 1
    row "far 2, echo k=4" 16384 --task echo --k 4 --m.far_slots 2 --m.far_percent 100
    row "delays 8, echo k=4" 16384 --task echo --k 4 --m.delay_max 8
    row "memory echo k=2" 2048 --engine memory --task echo --k 2
    row "memory catch 4x4" 2048 --engine memory --task catch --catch-size 4 4
    echo "load: $(cut -d' ' -f1-3 /proc/loadavg)"
    kernel_log "60 minutes ago"
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
