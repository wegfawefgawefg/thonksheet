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
                       const std::vector<std::vector<double>>& scores,
                       const std::vector<std::string>& worlds) {
    FILE* out = open_out(f.windows);
    if (!out) {
        return false;
    }
    size_t count = worlds.size() > 1 ? worlds.size() : 1;
    size_t per_world = lives.size() / count;
    size_t learning = f.twin ? per_world / 2 : per_world;
    std::fprintf(out, count > 1 ? "seed,twin,window,end_step,score,world\n"
                                : "seed,twin,window,end_step,score\n");
    for (size_t i = 0; i < lives.size(); i++) {
        for (size_t w = 0; w < scores[i].size(); w++) {
            std::fprintf(out, "%llu,%d,%zu,%lld,%.6f", (unsigned long long)lives[i].seed,
                         i % per_world >= learning ? 1 : 0, w,
                         (long long)((w + 1) * (size_t)f.run.window), scores[i][w]);
            if (count > 1) {
                std::fprintf(out, ",%s", worlds[(size_t)lives[i].world].c_str());
            }
            std::fprintf(out, "\n");
        }
    }
    std::fclose(out);
    return true;
}

// a world's name as a column: echo:k=8 is echo_k8
static std::string column_name(const std::string& world) {
    std::string out;
    for (char c : world) {
        if (c != '=') {
            out += c == ':' || c == '/' ? '_' : c;
        }
    }
    return out;
}

bool gpu_write_worlds(const GpuFlags& f, const GpuSettings& settings,
                      const std::vector<std::string>& worlds, const std::vector<GpuLifeSpec>& lives,
                      const std::vector<std::vector<double>>& scores) {
    FILE* out = open_out(f.out);
    if (!out) {
        return false;
    }
    size_t per_world = lives.size() / worlds.size();
    size_t rows = f.twin ? per_world / 2 : per_world;
    bool has_seed = false;
    for (const std::string& c : settings.columns) {
        std::fprintf(out, "%s,", c.c_str());
        has_seed = has_seed || c == "seed";
    }
    std::fprintf(out, has_seed ? "" : "seed,");
    for (size_t w = 0; w < worlds.size(); w++) {
        std::string name = column_name(worlds[w]);
        std::fprintf(out, "%slate_%s", w ? "," : "", name.c_str());
        if (f.twin) {
            std::fprintf(out, ",twin_late_%s", name.c_str());
        }
    }
    std::fprintf(out, "\n");
    for (size_t r = 0; r < rows; r++) {
        if (!settings.columns.empty()) {
            for (const std::string& cell : settings.rows[r]) {
                std::fprintf(out, "%s,", cell.c_str());
            }
        }
        if (!has_seed) {
            std::fprintf(out, "%llu,", (unsigned long long)lives[r].seed);
        }
        for (size_t w = 0; w < worlds.size(); w++) {
            double early = 0, late = 0;
            gpu_early_late(f, scores[w * per_world + r], early, late);
            std::fprintf(out, "%s%.6f", w ? "," : "", late);
            if (f.twin) {
                gpu_early_late(f, scores[w * per_world + rows + r], early, late);
                std::fprintf(out, ",%.6f", late);
            }
        }
        std::fprintf(out, "\n");
    }
    std::fclose(out);
    return true;
}
