#include "gpu/gpu_memory.hpp"

#include <cstdio>
#include <cstring>
#include <thread>

// memory-engine batches: births on the CPU, packed into the groups' buffers; the check.

bool gpu_memory_supported(const MemoryGenome& yg, std::string& why) {
    if (yg.sheet != 0) {
        why = "y.sheet (a sheet in front of the memory) is not on the GPU";
    } else if (yg.fade_shift != 0) {
        why = "y.fade_shift draws in table order; not on the GPU";
    } else {
        return true;
    }
    return false;
}

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// a life's store sizes, clamped as memory_birth's sanitize() clamps them
struct MemSizes {
    int tables, bits, entries;
};
static MemSizes sizes_of(const MemoryGenome& yg) {
    MemSizes s;
    s.tables = clamp_int(yg.tables, 1, 4096);
    s.bits = clamp_int(yg.table_bits, 1, MEM_TABLE_BITS_MAX);
    s.entries = yg.predict ? 1 << clamp_int(yg.predict_bits, 4, 20) : 1;
    return s;
}

bool gpu_memory_open(GpuContext& g, GpuBatch& b, const GpuFlags& f, std::vector<HostWorld>& worlds,
                     const std::vector<GpuLifeSpec>& lives, const char* spv) {
    Blob* first = memory_birth(lives[0].genome, lives[0].yg, lives[0].seed);
    const MemoryBlob& m = memory_of(*first);
    std::string why;
    bool ok = true;
    for (int t = 0; t <= first->tile_count; t++) {
        // the GPU fires the motor cells in a map of their own
        if (first->sensor[t] & first->motor[t]) {
            why = "the sheet's sensor and motor cells overlap";
            ok = false;
        }
    }
    std::vector<uint64_t> words;
    ok = ok && host_worlds_build(worlds, *first, words, why);
    GpuShape shape;
    shape.tiles = first->tile_count;
    shape.tiles_w = first->tiles_w;
    shape.stride = first->tile_count + 1;
    shape.workgroup = f.workgroup;
    shape.wave = f.wave;
    shape.host = 1;
    shape.engine = 1;
    shape.in_words = m.in_words > 0 ? m.in_words : 1;
    shape.motor_cells = m.motor_cells > 0 ? m.motor_cells : 1;
    for (const GpuLifeSpec& L : lives) {
        MemSizes s = sizes_of(L.yg);
        shape.tables = s.tables > shape.tables ? s.tables : shape.tables;
        shape.counters = (s.tables << s.bits) * MEM_UNITS > shape.counters
                             ? (s.tables << s.bits) * MEM_UNITS
                             : shape.counters;
        shape.pos = s.tables * s.bits > shape.pos ? s.tables * s.bits : shape.pos;
        shape.entries = s.entries > shape.entries ? s.entries : shape.entries;
    }
    // per batch: the input tiles, then the motor cells
    std::vector<uint32_t> meta((size_t)(MEM_IN_WORDS + shape.motor_cells), 0);
    for (int w = 0; w < m.in_words; w++) {
        meta[(size_t)w] = (uint32_t)m.in_tile[w];
    }
    for (int c = 0; c < m.motor_cells; c++) {
        meta[(size_t)(MEM_IN_WORDS + c)] = (uint32_t)m.motor_neuron[c];
    }
    memory_free(first);
    if (!ok) {
        std::fprintf(stderr, "blobgpu: %s\n", why.c_str());
        return false;
    }
    GpuParams p{};
    p.task = GPU_HOST_WORLD;
    p.ticks_per_step = f.run.plug.ticks_per_step;
    p.rest_ticks = f.run.plug.rest_ticks;
    p.reward_line_ticks = f.run.plug.reward_line_ticks;
    p.window = 1;
    p.steps = 1;
    if (!gpu_batch_open(g, b, shape, p, words, spv, nullptr)) {
        return false;
    }
    gpu_free(g, b.ev_meta);
    return gpu_buffer_new(g, b.ev_meta, meta.data(), meta.size() * 4);
}

// a born life's numbers and small state, as the shader reads them
static void pack_life(const MemoryBlob& m, const HostWorld& w, int world, GpuMemGenome& G,
                      GpuMemLife& L) {
    const MemoryGenome& y = m.mg;
    G = GpuMemGenome{y.learning_on,
                     y.code,
                     y.tables,
                     y.table_bits,
                     y.counter_max,
                     y.good_step,
                     y.bad_step,
                     y.others_up,
                     y.credit_ticks,
                     y.trace_ticks,
                     y.trace_across,
                     y.across_ticks,
                     y.explore_max,
                     y.explore_min,
                     y.avg_shift,
                     y.predict,
                     y.predict_bits,
                     0,
                     0,
                     0};
    L = GpuMemLife{};
    L.tick = m.tick;
    L.now_start = m.now_start;
    L.frame_count = m.frame_count;
    L.last_reward_tick = m.last_reward_tick;
    L.predict_key = m.predict_key;
    std::memcpy(L.rng, m.rng, sizeof(L.rng));
    std::memcpy(L.code_now, m.code_now, sizeof(L.code_now));
    std::memcpy(L.past_code, m.past_code, sizeof(L.past_code));
    std::memcpy(L.past_in_hash, m.past_in_hash, sizeof(L.past_in_hash));
    std::memcpy(L.in_now, m.in_now, sizeof(L.in_now));
    for (int i = 0; i < m.in_words; i++) {
        L.predicted[i] = m.shell.predicted[m.in_tile[i]];
    }
    for (int j = 0; j < MEM_DEPTH_MAX; j++) {
        L.past_level[j] = m.past_level[j];
    }
    L.level_now = m.level_now;
    L.drive = m.drive;
    L.units = m.units;
    L.explore_unit = m.explore_unit;
    L.new_cell = m.new_cell;
    L.frame_due = m.frame_due ? 1 : 0;
    L.good_avg = m.good_avg;
    L.predict_have = m.predict_have ? 1 : 0;
    for (int u = 0; u <= MEM_UNITS; u++) {
        L.tie_key[u] = m.tie_key[u];
    }
    L.block = world;
    L.motor_lines = w.motor_lines;
    L.attached = w.attached;
}

// what one group uploads, born on the CPU
struct MemPack {
    std::vector<GpuMemGenome> genomes;
    std::vector<GpuMemLife> lives;
    std::vector<uint32_t> pos;
    std::vector<uint64_t> rows, levels;
};

static void birth_some(const GpuShape& s, const std::vector<HostWorld>& worlds,
                       const std::vector<GpuLifeSpec>& lives, int first, int count, int from,
                       int step, MemPack& pack) {
    const size_t rows = (size_t)s.in_words * 64 * MEM_FIELD_WORDS;
    const size_t levels = (size_t)MEM_LEVELS * MEM_FIELD_WORDS;
    for (int i = from; i < count; i += step) {
        const GpuLifeSpec& spec = lives[(size_t)(first + i)];
        Blob* blob = memory_birth(spec.genome, spec.yg, spec.seed);
        const MemoryBlob& m = memory_of(*blob);
        pack_life(m, worlds[(size_t)spec.world], spec.world, pack.genomes[(size_t)i],
                  pack.lives[(size_t)i]);
        int bits = m.mg.table_bits;
        for (int t = 0; t < m.mg.tables * bits; t++) {
            pack.pos[(size_t)i * s.pos + (size_t)t] = m.pos[t];
        }
        std::memcpy(&pack.rows[(size_t)i * rows], m.rows,
                    sizeof(uint64_t) * (size_t)m.in_words * 64 * MEM_FIELD_WORDS);
        std::memcpy(&pack.levels[(size_t)i * levels], m.levels, sizeof(uint64_t) * levels);
        memory_free(blob);
    }
}

bool gpu_memory_upload(GpuContext& g, GpuBatch& b, const GpuFlags& f,
                       const std::vector<HostWorld>& worlds,
                       const std::vector<GpuLifeSpec>& lives) {
    const GpuShape& s = b.shape;
    const size_t rows = (size_t)s.in_words * 64 * MEM_FIELD_WORDS;
    const size_t levels = (size_t)MEM_LEVELS * MEM_FIELD_WORDS;
    const size_t in_words = (size_t)s.in_words;
    // a group: as many lives as the largest per-life buffer allows under the binding limit
    uint64_t limit = g.limits.maxStorageBufferRange;
    limit = limit < (1ull << 30) ? limit : (1ull << 30);
    uint64_t largest = (uint64_t)s.counters;
    largest =
        (uint64_t)s.entries * in_words * 8 > largest ? (uint64_t)s.entries * in_words * 8 : largest;
    int size = (int)(limit / largest);
    size = size < 1 ? 1 : (size > 65535 ? 65535 : size);
    size = f.check ? (int)lives.size() : size;
    size = f.group > 0 && f.group < size ? f.group : size;
    const int total = (int)lives.size();
    for (int first = 0; first < total; first += size) {
        int count = total - first < size ? total - first : size;
        size_t n = (size_t)count;
        MemPack pack;
        pack.genomes.resize(n);
        pack.lives.resize(n);
        pack.pos.assign(n * (size_t)s.pos, 0);
        pack.rows.assign(n * rows, 0);
        pack.levels.assign(n * levels, 0);
        std::vector<std::thread> helpers;
        int threads = f.birth_threads < 4 ? f.birth_threads : 4;
        for (int h = 1; h < threads; h++) {
            helpers.emplace_back(birth_some, std::cref(s), std::cref(worlds), std::cref(lives),
                                 first, count, h, threads, std::ref(pack));
        }
        birth_some(s, worlds, lives, first, count, 0, threads, pack);
        for (std::thread& h : helpers) {
            h.join();
        }
        if ((int)b.groups.size() >= 256) {
            std::fprintf(stderr, "blobgpu: more than 256 groups\n");
            return false;
        }
        b.groups.push_back(GpuGroup{});
        GpuGroup& gr = b.groups.back();
        gr.first = first;
        gr.count = count;
        GpuParams p = b.params;
        p.lives = count;
        std::vector<int32_t> unit_of(n * (size_t)s.motor_cells, -1);
        bool ok = gpu_buffer_new(g, gr.params, &p, sizeof(p)) &&
                  gpu_buffer_new(g, gr.genomes, pack.genomes.data(), sizeof(GpuMemGenome) * n) &&
                  gpu_buffer_new(g, gr.lives, pack.lives.data(), sizeof(GpuMemLife) * n) &&
                  gpu_buffer_new(g, gr.masks, nullptr, (uint64_t)s.counters * n) &&
                  gpu_buffer_new(g, gr.thresholds, pack.pos.data(), pack.pos.size() * 4) &&
                  gpu_buffer_new(g, gr.rings, pack.rows.data(), pack.rows.size() * 8) &&
                  gpu_buffer_new(g, gr.inhibitory, pack.levels.data(), pack.levels.size() * 8) &&
                  gpu_buffer_new(g, gr.scores, nullptr, (uint64_t)128 * s.tables * 4 * n) &&
                  gpu_buffer_new(g, gr.ev_planes, nullptr, sizeof(GpuMemFrame) * 128 * n) &&
                  gpu_buffer_new(g, gr.ev_lines, nullptr, (uint64_t)s.entries * 8 * n) &&
                  gpu_buffer_new(g, gr.delays, nullptr, (uint64_t)s.entries * in_words * 8 * n) &&
                  gpu_buffer_new(g, gr.far_tiles, unit_of.data(), unit_of.size() * 4) &&
                  gpu_group_exchange(g, b, gr) && gpu_group_bind(g, b, gr);
        if (!ok) {
            return false;
        }
    }
    std::printf("births on the CPU (%d threads), packed and uploaded\n",
                f.birth_threads < 4 ? f.birth_threads : 4);
    return true;
}

uint64_t gpu_memory_life_bytes(const GpuShape& s) {
    uint64_t in_words = (uint64_t)s.in_words;
    return (uint64_t)s.counters + (uint64_t)s.pos * 4 + in_words * 64 * MEM_FIELD_WORDS * 8 +
           (uint64_t)MEM_LEVELS * MEM_FIELD_WORDS * 8 + 128ull * s.tables * 4 +
           sizeof(GpuMemFrame) * 128 + (uint64_t)s.entries * 8 * (1 + in_words) +
           (uint64_t)s.motor_cells * 4 + sizeof(GpuMemLife) + sizeof(GpuMemGenome);
}

int64_t gpu_memory_ticks(GpuContext& g, GpuBatch& b) {
    int64_t ticks = 0;
    std::vector<GpuMemLife> s;
    for (GpuGroup& gr : b.groups) {
        s.resize((size_t)gr.count);
        if (!gpu_download(g, gr.lives, 0, s.data(), sizeof(GpuMemLife) * s.size())) {
            return -1;
        }
        for (const GpuMemLife& L : s) {
            ticks += L.tick;
        }
    }
    return ticks;
}

static bool differs(std::string& why, const char* what) {
    why = what;
    return false;
}

bool gpu_memory_matches(GpuContext& g, GpuBatch& b, GpuGroup& group, int index, const Blob& cpu,
                        std::string& why) {
    const GpuShape& s = b.shape;
    const MemoryBlob& m = *reinterpret_cast<const MemoryBlob*>(&cpu);
    const uint64_t i = (uint64_t)index, in_words = (uint64_t)s.in_words;
    GpuMemLife L;
    std::vector<int8_t> counters((size_t)s.counters);
    std::vector<uint32_t> slots((size_t)128 * s.tables);
    std::vector<GpuMemFrame> frames(128);
    std::vector<uint64_t> tags((size_t)s.entries), inputs((size_t)s.entries * in_words);
    std::vector<int32_t> unit_of((size_t)s.motor_cells);
    bool ok =
        gpu_download(g, group.lives, i * sizeof(L), &L, sizeof(L)) &&
        gpu_download(g, group.masks, i * s.counters, counters.data(), counters.size()) &&
        gpu_download(g, group.scores, i * slots.size() * 4, slots.data(), slots.size() * 4) &&
        gpu_download(g, group.ev_planes, i * sizeof(GpuMemFrame) * 128, frames.data(),
                     sizeof(GpuMemFrame) * 128) &&
        gpu_download(g, group.ev_lines, i * tags.size() * 8, tags.data(), tags.size() * 8) &&
        gpu_download(g, group.delays, i * inputs.size() * 8, inputs.data(), inputs.size() * 8) &&
        gpu_download(g, group.far_tiles, i * unit_of.size() * 4, unit_of.data(),
                     unit_of.size() * 4);
    if (!ok) {
        return differs(why, "read back failed");
    }
    if (L.tick != m.tick || L.now_start != m.now_start || L.frame_count != m.frame_count ||
        L.last_reward_tick != m.last_reward_tick || L.predict_key != m.predict_key ||
        std::memcmp(L.rng, m.rng, sizeof(L.rng)) != 0) {
        return differs(why, "clock, frames, reward tick, predictor key or random state");
    }
    if (std::memcmp(L.code_now, m.code_now, sizeof(L.code_now)) != 0 ||
        std::memcmp(L.past_code, m.past_code, sizeof(L.past_code)) != 0 ||
        std::memcmp(L.past_in_hash, m.past_in_hash, sizeof(L.past_in_hash)) != 0 ||
        std::memcmp(L.in_now, m.in_now, sizeof(uint64_t) * in_words) != 0) {
        return differs(why, "the input, the code of now or the past events");
    }
    for (int w = 0; w < m.in_words; w++) {
        if (L.predicted[w] != m.shell.predicted[m.in_tile[w]]) {
            return differs(why, "the predicted input");
        }
    }
    bool same = L.level_now == m.level_now && L.drive == m.drive && L.units == m.units &&
                L.explore_unit == m.explore_unit && L.new_cell == m.new_cell &&
                L.frame_due == (m.frame_due ? 1 : 0) && L.good_avg == m.good_avg &&
                L.predict_have == (m.predict_have ? 1 : 0);
    for (int j = 0; j < MEM_DEPTH_MAX; j++) {
        same = same && L.past_level[j] == m.past_level[j];
    }
    for (int u = 0; u <= MEM_UNITS; u++) {
        same = same && L.tie_key[u] == m.tie_key[u];
    }
    if (!same) {
        return differs(why, "levels, drive, buttons, exploring, new cell, good rate or ties");
    }
    size_t used = (size_t)(m.mg.tables << m.mg.table_bits) * MEM_UNITS;
    if (std::memcmp(counters.data(), m.counters, used) != 0) {
        return differs(why, "the counters");
    }
    int64_t oldest = m.frame_count > 128 ? m.frame_count - 128 : 0;
    for (int64_t f = oldest; f < m.frame_count; f++) {
        int ring = (int)(f & 127);
        const MemoryFrame& c = m.frames[ring];
        const GpuMemFrame& d = frames[(size_t)ring];
        same =
            same && c.first_tick == d.first_tick && c.last_tick == d.last_tick && c.unit == d.unit;
        for (int t = 0; t < m.mg.tables; t++) {
            same = same && m.frame_slot[(size_t)ring * m.mg.tables + t] ==
                               slots[(size_t)ring * s.tables + (size_t)t];
        }
    }
    if (!same) {
        return differs(why, "the frames or their slots");
    }
    if (m.predict_tag) {
        size_t entries = (size_t)1 << m.mg.predict_bits;
        if (std::memcmp(tags.data(), m.predict_tag, entries * 8) != 0 ||
            std::memcmp(inputs.data(), m.predict_in, entries * in_words * 8) != 0) {
            return differs(why, "the next-input store");
        }
    }
    for (int c = 0; c < m.motor_cells; c++) {
        if (unit_of[(size_t)c] != m.unit_of[c]) {
            return differs(why, "a motor cell's button");
        }
    }
    return true;
}
