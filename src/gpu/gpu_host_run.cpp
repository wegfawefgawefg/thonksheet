#include "gpu/gpu_host_run.hpp"

#include "gpu/gpu_memory.hpp"
#include "minimal/minimal.hpp"

#include <chrono>
#include <cstdio>
#include <thread>

// a host-world batch: the worlds' tables, then exchanges until every life is done.

static double seconds_since(std::chrono::steady_clock::time_point t) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t).count();
}

bool host_run_open(GpuContext& g, GpuBatch& b, const GpuFlags& f, std::vector<HostWorld>& worlds,
                   const std::vector<GpuLifeSpec>& lives, const char* life_spv,
                   const char* birth_spv) {
    Blob* first = minimal_birth(lives[0].genome, lives[0].mg, lives[0].seed);
    std::vector<uint64_t> words;
    std::string why;
    bool ok = host_worlds_build(worlds, *first, words, why);
    const MinimalBlob& m = minimal_of(*first);
    GpuShape shape;
    shape.tiles = first->tile_count;
    shape.tiles_w = first->tiles_w;
    shape.stride = m.stride;
    shape.max_windows = 0;
    shape.workgroup = f.workgroup;
    shape.wave = f.wave;
    shape.host = 1;
    // the grown-back pieces: room for the most any life of the batch asks for
    int ev_on = 0;
    for (const GpuLifeSpec& L : lives) {
        GpuLifeData d;
        gpu_life_unborn(d, L, f.run);
        shape.slots = d.genome.slots > shape.slots ? d.genome.slots : shape.slots;
        shape.delay = shape.delay || d.genome.delay_max > 1;
        ev_on = d.genome.ev_on > ev_on ? d.genome.ev_on : ev_on;
        shape.ev_bits =
            d.genome.ev_on && d.genome.ev_bits > shape.ev_bits ? d.genome.ev_bits : shape.ev_bits;
    }
    shape.ev = ev_on ? 1 : 0;
    shape.ev_rows = ev_on ? gpu_ev_rows(first->sensor, first->motor, first->tile_count, ev_on) : 1;
    std::vector<uint64_t> sensor(first->sensor, first->sensor + first->tile_count + 1);
    std::vector<uint64_t> motor(first->motor, first->motor + first->tile_count + 1);
    minimal_free(first);
    if (!ok) {
        std::fprintf(stderr, "blobgpu: %s\n", why.c_str());
        return false;
    }
    if (shape.ev && !f.birth_gpu) {
        std::fprintf(stderr, "blobgpu: evidence counters are born on the GPU only\n");
        return false;
    }
    // what the shader reads of the batch: the plug's ticks; the rest is per life
    GpuParams p{};
    p.task = GPU_HOST_WORLD;
    p.ticks_per_step = f.run.plug.ticks_per_step;
    p.rest_ticks = f.run.plug.rest_ticks;
    p.reward_line_ticks = f.run.plug.reward_line_ticks;
    p.window = 1;
    p.steps = 1;
    return gpu_batch_open(g, b, shape, p, words, life_spv, birth_spv, sensor.data(), motor.data(),
                          ev_on);
}

// the host's half of an exchange for lives [from, to) of a group. more threads only when one
// would take over 4 ms: on a loaded machine a nice 19 thread can wait milliseconds to start,
// and most worlds take well under a microsecond a life
static void exchange(const GpuFlags& f, const std::vector<HostWorld>& worlds,
                     std::vector<HostLife>& host, GpuGroup& group, int from, int to, bool answer,
                     int tiles, std::vector<HostScratch>& scratch, HostTimes& times,
                     double& per_life) {
    int n = to - from;
    int threads = (int)(per_life * n / 0.004) + 1;
    threads = threads > f.host_threads ? f.host_threads : threads;
    std::vector<HostTimes> part((size_t)threads);
    std::vector<std::thread> helpers;
    for (int t = 1; t < threads; t++) {
        int a = from + n * t / threads, z = from + n * (t + 1) / threads;
        helpers.emplace_back(host_exchange, std::cref(worlds), std::ref(host), std::ref(group), a,
                             z, answer, tiles, std::ref(scratch[(size_t)t]),
                             std::ref(part[(size_t)t]));
    }
    host_exchange(worlds, host, group, from, from + n / threads, answer, tiles, scratch[0],
                  part[0]);
    for (std::thread& h : helpers) {
        h.join();
    }
    double world = 0;
    for (const HostTimes& p : part) {
        times.exchange += p.exchange;
        times.world += p.world;
        world += p.world;
    }
    per_life = n > 0 ? world / n : per_life;
}

// the check after a step: each CPU life runs its step, then the sheets, the task and the
// reward the host answered are compared
static bool check_part(GpuContext& g, GpuBatch& b, GpuGroup& group, int from, int to,
                       const std::vector<HostLife>& host, std::vector<CpuLife>& cpu,
                       const std::vector<bool>& was_running) {
    GpuLifeData data;
    for (int i = from; i < to; i++) {
        size_t life = (size_t)(group.first + i);
        if (!was_running[life]) {
            continue;
        }
        CpuLife& c = cpu[life];
        int ran = (int)cpu_life_step(c);
        std::string why;
        bool same = false;
        if (b.shape.engine == 1) {
            same = gpu_memory_matches(g, b, group, i, *c.blob, why);
        } else if (gpu_group_read(g, b, group, i, data)) {
            same = cpu_life_matches(c, data, b, ran < 31 ? ran : 31, true, why);
        } else {
            return false;
        }
        const HostLife& h = host[life];
        const Task &a = h.task, &z = c.ticker->task;
        if (same && (a.random.state != z.random.state || a.cue != z.cue || a.phase != z.phase ||
                     a.steps != z.steps || h.valence != c.pending || h.move != c.pending_move)) {
            same = false;
            why = "the host's task or answer differs from the CPU life's";
        }
        if (!same) {
            std::fprintf(stderr, "check: life %zu (seed %llu) differs after tick %llu: %s\n", life,
                         (unsigned long long)c.flags.seed, (unsigned long long)c.blob->stats.tick,
                         why.c_str());
            return false;
        }
    }
    return true;
}

// what one pass over the lives needs
struct HostPass {
    const GpuFlags& f;
    const std::vector<HostWorld>& worlds;
    std::vector<HostLife>& host;
    std::vector<HostScratch>& scratch;
    int tiles, ticks;
    int part;             // lives per dispatch
    double card_life = 0; // seconds per life-step on the card, measured
    std::vector<bool> running;
};

// any life of [from, to) of a group still to step; marks which were running
static bool any_running(HostPass& p, const GpuGroup& group, int from, int to) {
    bool live = false;
    for (int i = from; i < to; i++) {
        size_t life = (size_t)(group.first + i);
        p.running[life] = !p.host[life].done;
        live = live || p.running[life];
    }
    return live;
}

// one step of every life, each dispatch waited for (and checked, with cpu). parts grow from
// 64 lives toward --dispatch-ms.
static bool step_waited(GpuContext& g, GpuBatch& b, HostPass& p, std::vector<CpuLife>* cpu,
                        bool& any, double& per_life, HostRunStats& out) {
    const double aim = p.f.dispatch_ms / 1000.0;
    p.running.resize(p.host.size());
    any = false;
    for (GpuGroup& group : b.groups) {
        for (int from = 0; from < group.count; from += p.part) {
            int to = from + p.part < group.count ? from + p.part : group.count;
            if (!any_running(p, group, from, to)) {
                continue;
            }
            any = true;
            auto t = std::chrono::steady_clock::now();
            bool ok = gpu_group_step_part(g, b, group, p.ticks, from, to - from);
            double took = seconds_since(t);
            out.times.card += took;
            out.longest = took > out.longest ? took : out.longest;
            out.dispatches++;
            p.card_life = took / (to - from);
            exchange(p.f, p.worlds, p.host, group, from, to, true, p.tiles, p.scratch, out.times,
                     per_life);
            out.times.exchanges++;
            if (!ok || (cpu && !check_part(g, b, group, from, to, p.host, *cpu, p.running))) {
                return false;
            }
            if (!cpu && to - from == p.part) {
                p.part = took < aim / 2 ? p.part * 2
                                        : (took > aim * 2 && p.part > 1 ? p.part / 2 : p.part);
            }
        }
    }
    return true;
}

// the rest of the life with the queue kept full: the lives are cut into parts of about
// --dispatch-ms each (two at least from 512 lives), each with its own command buffer and fence.
// while the card runs the parts queued behind it, the host answers the part that just finished and
// queues its next step.
static bool step_queued(GpuContext& g, GpuBatch& b, HostPass& p, double& per_life,
                        HostRunStats& out) {
    struct Part {
        GpuGroup* group;
        int from, to;
        GpuSlot slot;
        bool live;
    };
    double aim = p.f.dispatch_ms / 1000.0;
    int size = p.card_life > 0 ? (int)(aim / p.card_life) : p.part;
    size = size < 1 ? 1 : size;
    std::vector<Part> parts;
    for (GpuGroup& group : b.groups) {
        int count = group.count, cut = size < count ? size : count;
        if (b.groups.size() == 1 && cut == count && count >= 512) {
            // two parts, so one is always queued; a small batch does not fill the card, and
            // halving it only doubles the dispatches
            cut = (count + 1) / 2;
        }
        for (int from = 0; from < count; from += cut) {
            parts.push_back(Part{&group, from, from + cut < count ? from + cut : count, {}, true});
        }
    }
    bool ok = true;
    for (Part& q : parts) {
        ok = ok && gpu_slot_open(g, q.slot);
        q.live = ok && any_running(p, *q.group, q.from, q.to);
        ok = ok && (!q.live ||
                    gpu_group_step_async(g, b, *q.group, p.ticks, q.from, q.to - q.from, q.slot));
    }
    for (bool any = true; ok && any;) {
        any = false;
        for (Part& q : parts) {
            if (!q.live || !ok) {
                continue;
            }
            auto t = std::chrono::steady_clock::now();
            ok = gpu_slot_wait(g, q.slot);
            out.times.card += seconds_since(t); // the host waiting on the card
            out.dispatches++;
            exchange(p.f, p.worlds, p.host, *q.group, q.from, q.to, true, p.tiles, p.scratch,
                     out.times, per_life);
            out.times.exchanges++;
            q.live = any_running(p, *q.group, q.from, q.to);
            ok = ok && (!q.live || gpu_group_step_async(g, b, *q.group, p.ticks, q.from,
                                                        q.to - q.from, q.slot));
            any = any || q.live;
        }
    }
    for (Part& q : parts) {
        gpu_slot_close(g, q.slot);
    }
    return ok;
}

bool host_run(GpuContext& g, GpuBatch& b, const GpuFlags& f, const std::vector<HostWorld>& worlds,
              const std::vector<GpuLifeSpec>& lives, std::vector<std::vector<double>>& scores,
              std::vector<CpuLife>* cpu, HostRunStats& out) {
    const int tiles = b.shape.tiles;
    std::vector<HostLife> host(lives.size());
    for (size_t i = 0; i < lives.size(); i++) {
        host_life_start(host[i], lives[i].world, lives[i].seed, worlds[(size_t)lives[i].world]);
    }
    std::vector<HostScratch> scratch((size_t)f.host_threads);
    for (HostScratch& s : scratch) {
        host_scratch_open(s, worlds, tiles);
    }
    // the first step's lines
    double per_life = 1e-6;
    for (GpuGroup& group : b.groups) {
        exchange(f, worlds, host, group, 0, group.count, false, tiles, scratch, out.times,
                 per_life);
    }

    // a dispatch is one step: the rest ticks owed, then the step's ticks
    const int ticks = b.params.rest_ticks + b.params.ticks_per_step;
    HostPass pass{f, worlds, host, scratch, tiles, ticks, cpu ? 1 << 30 : 64, 0, {}};
    bool ok = true, any = true;
    auto start = std::chrono::steady_clock::now();
    if (cpu) {
        // checking: every step waited for and compared
        while (ok && any) {
            ok = step_waited(g, b, pass, cpu, any, per_life, out);
        }
    } else {
        // the first step waited for, in parts that grow toward --dispatch-ms, which measures a
        // life's step on the card; then every step queued
        ok = step_waited(g, b, pass, nullptr, any, per_life, out);
        ok = ok && (!any || step_queued(g, b, pass, per_life, out));
    }
    out.seconds = seconds_since(start);
    for (GpuGroup& group : b.groups) {
        std::vector<GpuLife> s((size_t)group.count);
        if (ok && b.shape.engine == 0 && gpu_group_lives(g, group, s.data())) {
            for (const GpuLife& L : s) {
                out.ticks += (int64_t)L.tick;
            }
        }
    }
    out.ticks = ok && b.shape.engine == 1 ? gpu_memory_ticks(g, b) : out.ticks;
    for (size_t i = 0; i < lives.size(); i++) {
        scores[i] = host[i].scores;
    }
    return ok;
}
