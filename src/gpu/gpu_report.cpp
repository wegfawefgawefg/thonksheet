#include "gpu/gpu_report.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

static FILE* open_out(const std::string& path) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) {
        std::fprintf(stderr, "blobgpu: cannot write %s\n", path.c_str());
    }
    return f;
}

void gpu_early_late(const GpuFlags& f, const std::vector<double>& s, double& early, double& late) {
    size_t windows = s.size();
    size_t edge = f.run.edge > 0 ? (size_t)f.run.edge : windows / 10;
    edge = edge < 1 ? 1 : (edge > windows ? windows : edge);
    early = late = 0.0;
    // summed in window order, as blobrun does, so the same numbers print the same
    for (size_t i = 0; i < edge && i < windows; i++) {
        early += s[i];
        late += s[windows - edge + i];
    }
    early = edge && windows ? early / (double)edge : 0.0;
    late = edge && windows ? late / (double)edge : 0.0;
}

bool gpu_write_lives(const GpuFlags& f, const GpuSettings& settings,
                     const std::vector<GpuLifeSpec>& lives,
                     const std::vector<std::vector<double>>& scores) {
    FILE* out = open_out(f.out);
    if (!out) {
        return false;
    }
    size_t learning = f.twin ? lives.size() / 2 : lives.size();
    // the settings' own columns first, then the seed unless they had it
    bool echo = !settings.columns.empty(), has_seed = false;
    for (const std::string& c : settings.columns) {
        std::fprintf(out, "%s,", c.c_str());
        has_seed = has_seed || c == "seed";
    }
    std::fprintf(out, has_seed ? "late,early" : "seed,late,early");
    std::fprintf(out, f.twin ? ",twin_late,twin_early\n" : "\n");
    for (size_t i = 0; i < learning; i++) {
        if (echo) {
            for (const std::string& cell : settings.rows[i]) {
                std::fprintf(out, "%s,", cell.c_str());
            }
        }
        double early = 0, late = 0;
        gpu_early_late(f, scores[i], early, late);
        if (!has_seed) {
            std::fprintf(out, "%llu,", (unsigned long long)lives[i].seed);
        }
        std::fprintf(out, "%.6f,%.6f", late, early);
        if (f.twin) {
            gpu_early_late(f, scores[learning + i], early, late);
            std::fprintf(out, ",%.6f,%.6f", late, early);
        }
        std::fprintf(out, "\n");
    }
    std::fclose(out);
    return true;
}

bool gpu_write_windows(const GpuFlags& f, const std::vector<GpuLifeSpec>& lives,
                       const std::vector<std::vector<double>>& scores) {
    FILE* out = open_out(f.windows);
    if (!out) {
        return false;
    }
    size_t learning = f.twin ? lives.size() / 2 : lives.size();
    std::fprintf(out, "seed,twin,window,end_step,score\n");
    for (size_t i = 0; i < lives.size(); i++) {
        for (size_t w = 0; w < scores[i].size(); w++) {
            std::fprintf(out, "%llu,%d,%zu,%lld,%.6f\n", (unsigned long long)lives[i].seed,
                         i >= learning ? 1 : 0, w, (long long)((w + 1) * (size_t)f.run.window),
                         scores[i][w]);
        }
    }
    std::fclose(out);
    return true;
}
