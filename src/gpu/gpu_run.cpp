#include "gpu/gpu_run.hpp"

#include "minimal/minimal.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

// births, upload, read back, the check, and the CPU path.

static bool same_sheet(const Genome& a, const Genome& b) {
    return a.tiles_w == b.tiles_w && a.tiles_h == b.tiles_h && a.sensor_x == b.sensor_x &&
           a.sensor_y == b.sensor_y && a.sensor_w == b.sensor_w && a.sensor_h == b.sensor_h &&
           a.sensor_stride == b.sensor_stride && a.motor_x == b.motor_x && a.motor_y == b.motor_y &&
           a.motor_w == b.motor_w && a.motor_h == b.motor_h && a.motor_stride == b.motor_stride;
}

bool gpu_run_supported(const std::vector<GpuLifeSpec>& lives) {
    for (size_t i = 0; i < lives.size(); i++) {
        std::string why;
        if (!gpu_genome_supported(lives[i].mg, why)) {
            std::fprintf(stderr, "blobgpu: life %zu: %s\n", i, why.c_str());
            return false;
        }
        if (!same_sheet(lives[i].genome, lives[0].genome)) {
            std::fprintf(stderr, "blobgpu: life %zu has another sheet; a batch shares one\n", i);
            return false;
        }
    }
    return true;
}

bool gpu_run_open(GpuContext& g, GpuBatch& b, const GpuFlags& f,
                  const std::vector<GpuLifeSpec>& lives, const char* life_spv,
                  const char* birth_spv) {
    Blob* first = minimal_birth(lives[0].genome, lives[0].mg, lives[0].seed);
    RunFlags rf = f.run;
    rf.task_seed = lives[0].seed;
    GpuWorld w;
    std::string why;
    bool ok = gpu_world_build(w, *first, rf, why);
    minimal_free(first);
    if (!ok) {
        std::fprintf(stderr, "blobgpu: %s\n", why.c_str());
        return false;
    }
    w.shape.workgroup = f.workgroup;
    w.shape.wave = f.wave;
    return gpu_batch_open(g, b, w.shape, w.params, w.words, life_spv, birth_spv);
}

// lives [first, first + count) born and packed, every `step`-th one from `from`
static void birth_some(const GpuFlags& f, const std::vector<GpuLifeSpec>& lives, int first,
                       int count, int from, int step, GpuLifeData* out,
                       const std::vector<HostWorld>* worlds) {
    for (int i = from; i < count; i += step) {
        const GpuLifeSpec& spec = lives[(size_t)(first + i)];
        if (f.birth_gpu) {
            gpu_life_unborn(out[i], spec, f.run);
        } else {
            Blob* blob = minimal_birth(spec.genome, spec.mg, spec.seed);
            RunFlags rf = f.run;
            rf.task_seed = spec.seed;
            gpu_life_pack(out[i], minimal_of(*blob), rf, spec.seed);
            minimal_free(blob);
        }
        if (worlds) {
            // host worlds: the life's table block, buttons and attached lines
            const HostWorld& w = (*worlds)[(size_t)spec.world];
            out[i].life.block = spec.world;
            out[i].life.motor_lines = w.motor_lines;
            out[i].life.attached = w.attached;
        }
    }
}

bool gpu_run_upload(GpuContext& g, GpuBatch& b, const GpuFlags& f,
                    const std::vector<GpuLifeSpec>& lives, std::vector<CpuLife>* check,
                    const std::vector<HostWorld>* worlds) {
    int total = (int)lives.size();
    int size = f.check ? total : gpu_group_size(g, b.shape);
    size = f.group > 0 && f.group < size ? f.group : size;
    std::vector<GpuLifeData> data;
    double born = 0, sent = 0;
    for (int first = 0; first < total; first += size) {
        int count = total - first < size ? total - first : size;
        data.resize((size_t)count);
        // births are independent: --birth-threads of them at once
        auto t = std::chrono::steady_clock::now();
        std::vector<std::thread> helpers;
        for (int h = 1; h < f.birth_threads; h++) {
            helpers.emplace_back(birth_some, std::cref(f), std::cref(lives), first, count, h,
                                 f.birth_threads, data.data(), worlds);
        }
        birth_some(f, lives, first, count, 0, f.birth_threads, data.data(), worlds);
        for (std::thread& helper : helpers) {
            helper.join();
        }
        auto u = std::chrono::steady_clock::now();
        born += std::chrono::duration<double>(u - t).count();
        if (!gpu_group_add(g, b, first, data.data(), count)) {
            return false;
        }
        sent += std::chrono::duration<double>(std::chrono::steady_clock::now() - u).count();
    }
    if (f.birth_gpu) {
        std::printf("births on the GPU, with upload %.2f s\n", sent);
    } else {
        std::printf("births %.2f s on %d CPU threads, packing and upload %.2f s\n", born,
                    f.birth_threads, sent);
    }
    if (check) {
        check->resize(lives.size());
        for (size_t i = 0; i < lives.size(); i++) {
            const RunFlags& rf = worlds ? (*worlds)[(size_t)lives[i].world].flags : f.run;
            if (!cpu_life_start((*check)[i], lives[i], rf)) {
                return false;
            }
        }
    }
    return true;
}

bool gpu_run_scores(GpuContext& g, GpuBatch& b, std::vector<std::vector<double>>& scores) {
    int windows = b.shape.max_windows > 0 ? b.shape.max_windows : 1;
    std::vector<uint32_t> raw;
    std::vector<GpuLife> states;
    for (GpuGroup& gr : b.groups) {
        raw.resize((size_t)windows * 2 * (size_t)gr.count);
        states.resize((size_t)gr.count);
        if (!gpu_group_scores(g, b, gr, raw.data()) || !gpu_group_lives(g, gr, states.data())) {
            return false;
        }
        for (int i = 0; i < gr.count; i++) {
            std::vector<double>& s = scores[(size_t)(gr.first + i)];
            int written = states[(size_t)i].windows < windows ? states[(size_t)i].windows : windows;
            s.clear();
            for (int w = 0; w < written; w++) {
                uint32_t right = raw[((size_t)i * windows + w) * 2];
                uint32_t chances = raw[((size_t)i * windows + w) * 2 + 1];
                // task_window_score: right answers over chances
                s.push_back(chances > 0 ? (double)right / (double)chances : 0.0);
            }
        }
    }
    return true;
}

bool gpu_run_check(GpuContext& g, GpuBatch& b, GpuGroup& group, std::vector<CpuLife>& cpu,
                   int ticks, bool host) {
    GpuLifeData data;
    for (int i = 0; i < group.count; i++) {
        CpuLife& c = cpu[(size_t)(group.first + i)];
        cpu_life_ticks(c, ticks);
        if (!gpu_group_read(g, b, group, i, data)) {
            return false;
        }
        std::string why;
        if (!cpu_life_matches(c, data, ticks, host, why)) {
            std::fprintf(stderr, "check: life %d (seed %llu) differs after tick %llu: %s\n",
                         group.first + i, (unsigned long long)c.flags.seed,
                         (unsigned long long)c.blob->stats.tick, why.c_str());
            return false;
        }
    }
    return true;
}

void cpu_run(const GpuFlags& f, const std::vector<GpuLifeSpec>& lives,
             const std::vector<HostWorld>& worlds, std::vector<std::vector<double>>& scores,
             int64_t& ticks, double& seconds) {
    ticks = 0;
    seconds = 0;
    for (size_t i = 0; i < lives.size(); i++) {
        CpuLife c;
        const RunFlags& rf = worlds.empty() ? f.run : worlds[(size_t)lives[i].world].flags;
        if (!cpu_life_start(c, lives[i], rf)) {
            std::fprintf(stderr, "blobgpu: life %zu: the plug does not fit\n", i);
            cpu_life_free(c);
            return;
        }
        auto start = std::chrono::steady_clock::now();
        // a life is at most steps x (ticks per step + rest ticks) ticks
        int64_t cap = c.flags.steps * (c.flags.plug.ticks_per_step + c.flags.plug.rest_ticks + 1);
        ticks += cpu_life_ticks(c, cap);
        seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        scores[i] = c.scores;
        cpu_life_free(c);
    }
}
