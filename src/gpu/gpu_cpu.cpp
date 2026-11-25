#include "gpu/gpu_cpu.hpp"

#include "minimal/minimal.hpp"
#include "world/plug.hpp"

#include <cstdio>
#include <cstring>
#include <string>

// CPU lives for the check and the speed table.

bool cpu_life_start(CpuLife& c, const GpuLifeSpec& spec, const RunFlags& flags) {
    c = CpuLife{};
    c.flags = flags;
    c.flags.seed = spec.seed;
    c.flags.task_seed = spec.seed;
    c.flags.genome = spec.genome;
    c.blob = minimal_birth(spec.genome, spec.mg, spec.seed);
    c.ticker = new TaskTicker{};
    return ticker_start(*c.ticker, *c.blob, c.flags);
}

int64_t cpu_life_ticks(CpuLife& c, int64_t ticks) {
    TaskTicker& tt = *c.ticker;
    Blob& blob = *c.blob;
    int64_t ran = 0;
    // life_run's loop for one phase
    while (ran < ticks && !c.done) {
        ticker_before_tick(tt, blob);
        minimal_tick(blob);
        ran++;
        if (!ticker_after_tick(tt, blob)) {
            continue;
        }
        if (tt.valence != 0) {
            plug_mark_acted(tt.plug, blob, tt.move);
            minimal_reward(blob, tt.valence);
        }
        c.step++;
        if (c.step % c.flags.window == 0) {
            c.scores.push_back(task_window_score(tt.task));
        }
        c.done = c.step >= c.flags.steps;
    }
    c.ticks += ran;
    return ran;
}

void cpu_life_free(CpuLife& c) {
    if (c.blob) {
        minimal_free(c.blob);
    }
    delete c.ticker;
    c = CpuLife{};
}

static bool same_words(const uint64_t* a, const uint64_t* b, size_t count, size_t& at) {
    for (size_t i = 0; i < count; i++) {
        if (a[i] != b[i]) {
            at = i;
            return false;
        }
    }
    return true;
}

bool cpu_life_matches(const CpuLife& c, const GpuLifeData& gpu, int recent, std::string& why) {
    const MinimalBlob& m = minimal_of(*c.blob);
    const Blob& b = m.shell;
    const GpuLife& L = gpu.life;
    char text[256];
    if (L.now != m.now || L.tick != b.stats.tick) {
        std::snprintf(text, sizeof(text), "clock: gpu now %d tick %llu, cpu now %d tick %llu",
                      L.now, (unsigned long long)L.tick, m.now, (unsigned long long)b.stats.tick);
        why = text;
        return false;
    }
    // the maps of the last `recent` ticks, newest first, then the noise maps
    size_t stride = (size_t)m.stride, ring = stride * MIN_HISTORY, at = 0;
    for (int plane = 0; plane < 2; plane++) {
        const uint64_t* cpu = plane == 0 ? m.spikes : m.noise;
        const uint64_t* mine = gpu.ring.data() + plane * ring;
        for (int age = 0; age < recent && age < MIN_HISTORY; age++) {
            size_t index = (size_t)((m.now - age) & (MIN_HISTORY - 1)) * stride;
            if (!same_words(cpu + index, mine + index, (size_t)b.tile_count + 1, at)) {
                std::snprintf(
                    text, sizeof(text), "%s map of tick %llu, tile %zu: gpu %016llx cpu %016llx",
                    plane == 0 ? "spike" : "noise",
                    (unsigned long long)(b.stats.tick - (uint64_t)age), at,
                    (unsigned long long)mine[index + at], (unsigned long long)cpu[index + at]);
                why = text;
                return false;
            }
        }
        if (!same_words(cpu, mine, ring, at)) {
            std::snprintf(text, sizeof(text), "%s ring word %zu", plane == 0 ? "spike" : "noise",
                          at);
            why = text;
            return false;
        }
    }
    size_t n = (size_t)b.neuron_count;
    if (!same_words(m.mask, gpu.mask.data(), n * MIN_SLOTS, at)) {
        std::snprintf(text, sizeof(text), "mask word %zu (neuron %zu, slot %zu)", at,
                      (at / (MIN_SLOTS * 64)) * 64 + at % 64, (at / 64) % MIN_SLOTS);
        why = text;
        return false;
    }
    if (std::memcmp(m.threshold, gpu.threshold.data(), n) != 0) {
        why = "thresholds";
        return false;
    }
    if (!same_words(b.inhibitory, gpu.inhibitory.data(), (size_t)b.tile_count + 1, at)) {
        why = "inhibitory word of tile " + std::to_string(at);
        return false;
    }
    const Task& task = c.ticker->task;
    if (L.reward_avg != m.reward_avg || L.world != task.random.state || L.cue != task.cue ||
        L.phase != task.phase) {
        std::snprintf(text, sizeof(text),
                      "reward average or task: gpu avg %d cue %d phase %d, cpu avg %d cue %d "
                      "phase %d",
                      L.reward_avg, L.cue, L.phase, m.reward_avg, task.cue, task.phase);
        why = text;
        return false;
    }
    return true;
}
