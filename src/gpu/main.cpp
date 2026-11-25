// blobgpu: many minimal-engine lives at once on a Vulkan compute device, headless (no window,
// no surface, ever). Each life is born, uploaded, and stepped by the shader
// shaders/gpu/minimal_life.comp. The Echo family runs in the shader with its task; every
// other task runs as a host world (gpu_host.hpp): the sheet on the GPU, the world's own code
// on the CPU, one exchange per step. One row per life (or per settings row, with --tasks).
// docs/results-2026-11-25-gpu.md; scripts/gpu.sh runs it.

#include "gpu/gpu_host_run.hpp"
#include "gpu/gpu_memory.hpp"
#include "gpu/gpu_report.hpp"
#include "gpu/gpu_run.hpp"

#include <chrono>
#include <cstdio>

#ifndef BLOBGPU_SHADER_DIR
#define BLOBGPU_SHADER_DIR "build/shaders/gpu"
#endif

static double seconds_since(std::chrono::steady_clock::time_point t) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t).count();
}

// the Echo family with its task in the shader: dispatches of many ticks, growing toward
// --dispatch-ms. returns the life-ticks run, or -1.
static int64_t shader_world_loop(GpuContext& g, GpuBatch& b, const GpuFlags& f,
                                 std::vector<CpuLife>& cpu, double& seconds, int& dispatches,
                                 double& longest) {
    bool ok = true;
    int per_dispatch = f.check ? f.check_ticks : 1;
    std::vector<GpuLife> states;
    auto loop_start = std::chrono::steady_clock::now();
    for (bool all_done = false; ok && !all_done;) {
        all_done = true;
        double slowest = 0;
        for (GpuGroup& group : b.groups) {
            if (group.done) {
                continue;
            }
            auto t = std::chrono::steady_clock::now();
            ok = gpu_group_step(g, b, group, per_dispatch);
            double took = seconds_since(t);
            slowest = took > slowest ? took : slowest;
            states.resize((size_t)group.count);
            ok = ok && gpu_group_lives(g, group, states.data());
            group.done = true;
            for (const GpuLife& L : states) {
                group.done = group.done && L.done != 0;
            }
            all_done = all_done && group.done;
            if (ok && f.check) {
                ok = gpu_run_check(g, b, group, cpu, per_dispatch);
            }
            dispatches++;
            if (!ok) {
                break;
            }
        }
        longest = slowest > longest ? slowest : longest;
        if (!f.check) {
            double aim = f.dispatch_ms / 1000.0;
            per_dispatch = slowest < aim / 2   ? per_dispatch * 2
                           : slowest > aim * 2 ? per_dispatch / 2
                                               : per_dispatch;
            per_dispatch = per_dispatch < 1             ? 1
                           : per_dispatch > f.max_ticks ? f.max_ticks
                                                        : per_dispatch;
        }
    }
    seconds = seconds_since(loop_start);
    int64_t ticks = 0;
    for (GpuGroup& group : b.groups) {
        std::vector<GpuLife> s((size_t)group.count);
        if (ok && gpu_group_lives(g, group, s.data())) {
            for (const GpuLife& L : s) {
                ticks += (int64_t)L.tick;
            }
        }
    }
    return ok ? ticks : -1;
}

// the memory engine: every life's genome runs as written, and every life shares one sheet
static bool memory_supported(const std::vector<GpuLifeSpec>& lives) {
    for (size_t i = 0; i < lives.size(); i++) {
        std::string why;
        const Genome &a = lives[i].genome, &b = lives[0].genome;
        if (!gpu_memory_supported(lives[i].yg, why)) {
            std::fprintf(stderr, "blobgpu: life %zu: %s\n", i, why.c_str());
            return false;
        }
        if (a.tiles_w != b.tiles_w || a.tiles_h != b.tiles_h || a.sensor_x != b.sensor_x ||
            a.sensor_y != b.sensor_y || a.sensor_w != b.sensor_w || a.sensor_h != b.sensor_h ||
            a.motor_x != b.motor_x || a.motor_y != b.motor_y || a.motor_w != b.motor_w ||
            a.motor_h != b.motor_h || a.sensor_stride != b.sensor_stride ||
            a.motor_stride != b.motor_stride) {
            std::fprintf(stderr, "blobgpu: life %zu has another sheet; a batch shares one\n", i);
            return false;
        }
    }
    return true;
}

// the memory engine's batch: births on the CPU; checking, the CPU lives beside them
static bool memory_open(GpuContext& g, GpuBatch& b, const GpuFlags& f,
                        std::vector<HostWorld>& worlds, const std::vector<GpuLifeSpec>& lives,
                        std::vector<CpuLife>& cpu) {
    if (!gpu_memory_open(g, b, f, worlds, lives, BLOBGPU_SHADER_DIR "/memory_life.comp.spv") ||
        !gpu_memory_upload(g, b, f, worlds, lives)) {
        return false;
    }
    if (!f.check) {
        return true;
    }
    cpu.resize(lives.size());
    for (size_t i = 0; i < lives.size(); i++) {
        if (!cpu_life_start(cpu[i], lives[i], worlds[(size_t)lives[i].world].flags)) {
            return false;
        }
    }
    // the newborn stores
    for (GpuGroup& group : b.groups) {
        for (int i = 0; i < group.count; i++) {
            std::string why;
            if (!gpu_memory_matches(g, b, group, i, *cpu[(size_t)(group.first + i)].blob, why)) {
                std::fprintf(stderr, "check: life %d differs at birth: %s\n", group.first + i,
                             why.c_str());
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char** argv) {
    GpuFlags f;
    Genome genome;
    MinimalGenome mg;
    MemoryGenome yg;
    if (!gpu_flags_parse(argc, argv, f, genome, mg, yg)) {
        return 1;
    }
    const bool memory = gpu_memory_engine(f);
    // the Echo family runs in the shader unless --host or --tasks says otherwise
    std::vector<HostWorld> worlds;
    GpuSettings settings;
    std::vector<GpuLifeSpec> lives;
    if (!gpu_lives_list(f, genome, mg, yg, 1, settings, lives)) {
        return 1;
    }
    f.host = f.host || memory || !gpu_world_in_shader(f.run.task);
    for (const GpuLifeSpec& life : lives) {
        f.host = f.host || gpu_needs_host(life.mg);
    }
    if (f.host && !host_worlds_parse(f.tasks, f.run_args, f.run, worlds)) {
        return 1;
    }
    int world_count = worlds.empty() ? 1 : (int)worlds.size();
    settings = GpuSettings{};
    if (!gpu_lives_list(f, genome, mg, yg, world_count, settings, lives)) {
        return 1;
    }
    std::vector<std::string> names;
    for (const HostWorld& w : worlds) {
        names.push_back(w.name);
    }
    std::vector<std::vector<double>> scores(lives.size());
    auto start = std::chrono::steady_clock::now();
    int64_t ticks = 0;
    double tick_seconds = 0;

    if (f.device == GpuDevice::cpu) {
        // the CPU engine, one life after another, for the speed table
        cpu_run(f, lives, worlds, scores, ticks, tick_seconds);
        std::printf("cpu: %zu lives, %lld ticks in %.2f s ticking: %.0f life-ticks/s\n",
                    lives.size(), (long long)ticks, tick_seconds, (double)ticks / tick_seconds);
    } else {
        if (memory ? !memory_supported(lives) : !gpu_run_supported(lives)) {
            return 1;
        }
        GpuContext g;
        GpuBatch b;
        std::vector<CpuLife> cpu;
        std::string spv = f.shader.empty() ? BLOBGPU_SHADER_DIR "/minimal_life.comp.spv" : f.shader;
        const char* birth_spv = BLOBGPU_SHADER_DIR "/minimal_birth.comp.spv";
        bool ok = gpu_open(g, f.device == GpuDevice::lavapipe, f.validate);
        if (ok && memory) {
            ok = memory_open(g, b, f, worlds, lives, cpu);
        } else if (ok) {
            ok = (f.host ? host_run_open(g, b, f, worlds, lives, spv.c_str(), birth_spv)
                         : gpu_run_open(g, b, f, lives, spv.c_str(), birth_spv)) &&
                 gpu_run_upload(g, b, f, lives, f.check ? &cpu : nullptr,
                                f.host ? &worlds : nullptr);
        }
        double setup = seconds_since(start);
        std::printf("%s: %zu lives in %zu groups, %.0f KB per life, setup %.2f s%s\n",
                    g.name.c_str(), lives.size(), b.groups.size(),
                    (double)(memory ? gpu_memory_life_bytes(b.shape) : gpu_life_bytes(b.shape)) /
                        1024.0,
                    setup, f.host ? ", host worlds" : "");

        // checking: the newborn sheets first, before any tick
        for (GpuGroup& group : b.groups) {
            ok = ok && (!f.check || memory || gpu_run_check(g, b, group, cpu, 0, f.host));
        }
        int dispatches = 0;
        double longest = 0;
        if (ok && f.host) {
            HostRunStats hs;
            ok = host_run(g, b, f, worlds, lives, scores, f.check ? &cpu : nullptr, hs);
            ticks = hs.ticks;
            tick_seconds = hs.seconds;
            dispatches = hs.dispatches;
            longest = hs.longest;
            const HostTimes& t = hs.times;
            std::printf("host worlds: %lld exchanges; card %.2f s, host %.2f s wall (thread-s: "
                        "world %.2f, exchange %.2f)\n",
                        (long long)t.exchanges, t.card, hs.seconds - t.card, t.world, t.exchange);
        } else if (ok) {
            ticks = shader_world_loop(g, b, f, cpu, tick_seconds, dispatches, longest);
            ok = ticks >= 0 && gpu_run_scores(g, b, scores);
        }
        if (ok && f.check) {
            for (size_t i = 0; i < lives.size(); i++) {
                ok = ok && cpu[i].scores == scores[i];
                cpu_life_free(cpu[i]);
            }
            // spike and noise maps every tick; rings, masks, thresholds, task every dispatch
            std::printf(ok ? "check: identical to the CPU engine\n"
                           : "check: window scores differ\n");
        }
        std::printf("%s: %lld ticks in %.2f s: %.0f life-ticks/s, %d dispatches, longest "
                    "%.0f ms, %d validation messages\n",
                    ok ? "gpu" : "gpu FAILED", (long long)ticks, tick_seconds,
                    (double)ticks / tick_seconds, dispatches, longest * 1000.0, gpu_messages());
        gpu_batch_close(g, b);
        gpu_close(g);
        if (!ok) {
            return 1;
        }
    }

    double total = seconds_since(start);
    std::printf("batch: %zu lives in %.2f s, %.2f lives/s\n", lives.size(), total,
                (double)lives.size() / total);
    if (!f.tasks.empty()) {
        gpu_write_worlds(f, settings, names, lives, scores);
    } else {
        gpu_write_lives(f, settings, lives, scores);
    }
    if (!f.windows.empty()) {
        gpu_write_windows(f, lives, scores, names);
    }
    std::printf("rows: %s\n", f.out.c_str());
    return 0;
}
