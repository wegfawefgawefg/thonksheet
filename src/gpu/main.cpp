// blobgpu: many minimal-engine lives at once on a Vulkan compute device, headless (no window,
// no surface, ever). Each life is born on the CPU, uploaded, and stepped by the shader
// shaders/gpu/minimal_life.comp together with its task. One row per life comes out.
// docs/results-2026-11-25-gpu.md; scripts/gpu.sh runs it.

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

int main(int argc, char** argv) {
    GpuFlags f;
    Genome genome;
    MinimalGenome mg;
    if (!gpu_flags_parse(argc, argv, f, genome, mg)) {
        return 1;
    }
    GpuSettings settings;
    std::vector<GpuLifeSpec> lives;
    if (!gpu_lives_list(f, genome, mg, settings, lives)) {
        return 1;
    }
    std::vector<std::vector<double>> scores(lives.size());
    auto start = std::chrono::steady_clock::now();
    int64_t ticks = 0;
    double tick_seconds = 0;

    if (f.device == GpuDevice::cpu) {
        // the CPU engine, one life after another, for the speed table
        cpu_run(f, lives, scores, ticks, tick_seconds);
        std::printf("cpu: %zu lives, %lld ticks in %.2f s ticking: %.0f life-ticks/s\n",
                    lives.size(), (long long)ticks, tick_seconds, (double)ticks / tick_seconds);
    } else {
        if (!gpu_run_supported(lives)) {
            return 1;
        }
        GpuContext g;
        GpuBatch b;
        std::vector<CpuLife> cpu;
        bool ok = gpu_open(g, f.device == GpuDevice::lavapipe, f.validate) &&
                  gpu_run_open(g, b, f, lives, BLOBGPU_SHADER_DIR "/minimal_life.comp.spv") &&
                  gpu_run_upload(g, b, f, lives, f.check ? &cpu : nullptr);
        double setup = seconds_since(start);
        std::printf("%s: %zu lives in %zu groups, %.0f KB per life, setup %.2f s\n", g.name.c_str(),
                    lives.size(), b.groups.size(), (double)gpu_life_bytes(b.shape) / 1024.0, setup);

        // step every group until every life is done. dispatches start at one tick and grow
        // toward --dispatch-ms each; a dispatch never runs more than --max-ticks ticks.
        int per_dispatch = f.check ? f.check_ticks : 1;
        std::vector<GpuLife> states;
        auto loop_start = std::chrono::steady_clock::now();
        int dispatches = 0;
        double longest = 0;
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
        tick_seconds = seconds_since(loop_start);
        ok = ok && gpu_run_scores(g, b, scores);
        for (GpuGroup& group : b.groups) {
            std::vector<GpuLife> s((size_t)group.count);
            if (ok && gpu_group_lives(g, group, s.data())) {
                for (const GpuLife& L : s) {
                    ticks += (int64_t)L.tick;
                }
            }
        }
        if (ok && f.check) {
            for (size_t i = 0; i < lives.size(); i++) {
                ok = ok && cpu[i].scores == scores[i];
                cpu_life_free(cpu[i]);
            }
            std::printf(ok ? "check: every tick of every life identical to the CPU engine "
                             "(spike and noise maps, masks, thresholds, task, scores)\n"
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
    gpu_write_lives(f, settings, lives, scores);
    if (!f.windows.empty()) {
        gpu_write_windows(f, lives, scores);
    }
    std::printf("rows: %s\n", f.out.c_str());
    return 0;
}
