#include "gpu/gpu_flags.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

static const char* usage =
    "usage: blobgpu [flags] [blobrun flags]\n"
    "  --lives N          lives, seeds seed-from .. seed-from + N - 1 (default 1)\n"
    "  --seed-from S      first seed (default 1)\n"
    "  --tiles T          sheet of T x T tiles (default: the genome's, 8)\n"
    "  --twin             also each life with learning off\n"
    "  --settings PATH    csv, one row per life: seed and any minimal (m.name or name) or\n"
    "                     sheet (g.name) numbers; overrides --lives\n"
    "  --out PATH         one row per life: settings, late, early (default out/gpu/lives.csv)\n"
    "  --windows PATH     one row per life per window\n"
    "  --device D         gpu (default), lavapipe (software Vulkan), cpu (the CPU engine, one\n"
    "                     thread, for the speed table)\n"
    "  --validate         Vulkan validation layers on\n"
    "  --check            run the CPU engine alongside, compare every tick (small batches)\n"
    "  --workgroup N      threads per life (default 256)\n"
    "  --group N          lives per dispatch at most (default: as many as one buffer holds)\n"
    "  --birth-threads N  CPU threads making the lives (default 2)\n"
    "  --wave N           ask for subgroups of N lanes (32 or 64 on the GPU; default: driver)\n"
    "  --dispatch-ms N    aim for submits about N ms long (default 50)\n"
    "  --max-ticks N      ticks per dispatch at most (default 4096)\n"
    "  --check-ticks N    ticks per dispatch while checking (default 16, at most 31)\n"
    "  --shader PATH      another build of minimal_life.comp (.spv), for speed experiments\n"
    "blobrun flags: --task, --k, --delay, --steps, --ticks, --window, --edge, --sensor-side,\n"
    "  --motor-side, --reward-ticks, --rest-ticks, --cue-offset, --g.name V, --m.name V\n"
    "tasks on the GPU: echo, shuffled-echo, delayed-echo, both-not-either\n";

static bool is(const char* a, const char* b) {
    return std::strcmp(a, b) == 0;
}

bool gpu_flags_parse(int argc, char** argv, GpuFlags& f, Genome& genome, MinimalGenome& mg) {
    // blobgpu's own flags out; the rest go to blobrun's parser
    std::vector<char*> rest = {argv[0]};
    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        const char* v = i + 1 < argc ? argv[i + 1] : nullptr;
        bool flag_only = is(a, "--twin") || is(a, "--check") || is(a, "--validate");
        if (is(a, "--help") || is(a, "-h")) {
            std::fputs(usage, stderr);
            return false;
        }
        if (flag_only) {
            f.twin |= is(a, "--twin");
            f.check |= is(a, "--check");
            f.validate |= is(a, "--validate");
            continue;
        }
        bool mine = is(a, "--lives") || is(a, "--seed-from") || is(a, "--tiles") ||
                    is(a, "--settings") || is(a, "--out") || is(a, "--windows") ||
                    is(a, "--device") || is(a, "--workgroup") || is(a, "--wave") ||
                    is(a, "--birth-threads") || is(a, "--group") || is(a, "--dispatch-ms") ||
                    is(a, "--max-ticks") || is(a, "--check-ticks") || is(a, "--shader");
        if (!mine) {
            rest.push_back(argv[i]);
            continue;
        }
        if (!v) {
            std::fprintf(stderr, "blobgpu: %s needs a value\n", a);
            return false;
        }
        i++;
        if (is(a, "--settings")) {
            f.settings = v;
        } else if (is(a, "--shader")) {
            f.shader = v;
        } else if (is(a, "--out")) {
            f.out = v;
        } else if (is(a, "--windows")) {
            f.windows = v;
        } else if (is(a, "--device")) {
            if (is(v, "gpu") || is(v, "lavapipe") || is(v, "cpu")) {
                f.device = is(v, "gpu") ? GpuDevice::gpu
                                        : (is(v, "cpu") ? GpuDevice::cpu : GpuDevice::lavapipe);
            } else {
                std::fprintf(stderr, "blobgpu: --device is gpu, lavapipe or cpu\n");
                return false;
            }
        } else {
            long long n = std::strtoll(v, nullptr, 10);
            if (is(a, "--lives")) {
                f.lives = (int)n;
            } else if (is(a, "--seed-from")) {
                f.seed_from = (uint64_t)n;
            } else if (is(a, "--tiles")) {
                f.tiles = (int)n;
            } else if (is(a, "--workgroup")) {
                f.workgroup = (int)n;
            } else if (is(a, "--group")) {
                f.group = (int)n;
            } else if (is(a, "--birth-threads")) {
                f.birth_threads = n < 1 ? 1 : (int)n;
            } else if (is(a, "--wave")) {
                f.wave = (int)n;
            } else if (is(a, "--dispatch-ms")) {
                f.dispatch_ms = (int)n;
            } else if (is(a, "--max-ticks")) {
                f.max_ticks = (int)n;
            } else {
                f.check_ticks = (int)n;
            }
        }
    }
    int count = (int)rest.size();
    apply_genome_args(count, rest.data(), f.run.genome);
    if (!run_flags_parse(count, rest.data(), f.run)) {
        return false;
    }
    apply_minimal_args(count, rest.data(), mg);
    if (f.tiles > 0) {
        f.run.genome.tiles_w = f.tiles;
        f.run.genome.tiles_h = f.tiles;
    }
    genome = f.run.genome;
    // the GPU makes keyed words; the CPU engine keeps what it was asked for
    if (f.device != GpuDevice::cpu || f.check) {
        mg.rng_mode = 1;
    }
    f.check_ticks = f.check_ticks < 1 ? 1 : (f.check_ticks > 31 ? 31 : f.check_ticks);
    f.max_ticks = f.max_ticks < 1 ? 1 : f.max_ticks;
    if (f.lives < 1 || f.workgroup < 64 || f.workgroup > 1024 || f.workgroup % 64 != 0) {
        std::fprintf(stderr, "blobgpu: --lives at least 1, --workgroup 64..1024 in 64s\n");
        return false;
    }
    if (f.out.empty()) {
        f.out = "out/gpu/lives.csv";
    }
    return true;
}

static std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> cells;
    std::stringstream in(line);
    std::string cell;
    while (std::getline(in, cell, ',')) {
        while (!cell.empty() && (cell.back() == '\r' || cell.back() == ' ')) {
            cell.pop_back();
        }
        cells.push_back(cell);
    }
    return cells;
}

static bool read_settings(const std::string& path, GpuSettings& s) {
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "blobgpu: cannot read %s\n", path.c_str());
        return false;
    }
    std::string line;
    if (!std::getline(in, line)) {
        std::fprintf(stderr, "blobgpu: %s is empty\n", path.c_str());
        return false;
    }
    s.columns = split_csv(line);
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::vector<std::string> row = split_csv(line);
        if (row.size() != s.columns.size()) {
            std::fprintf(stderr, "blobgpu: %s: a row has %zu cells, the header %zu\n", path.c_str(),
                         row.size(), s.columns.size());
            return false;
        }
        s.rows.push_back(row);
    }
    return true;
}

// one settings row onto a life: seed, g.name through the sheet genome's parser, the rest
// through the minimal genome's
static bool apply_row(const GpuSettings& s, const std::vector<std::string>& row,
                      GpuLifeSpec& life) {
    for (size_t c = 0; c < s.columns.size(); c++) {
        const std::string& name = s.columns[c];
        if (name == "seed") {
            life.seed = std::strtoull(row[c].c_str(), nullptr, 10);
            continue;
        }
        std::string flag =
            name.rfind("g.", 0) == 0 || name.rfind("m.", 0) == 0 ? "--" + name : "--m." + name;
        std::string value = row[c];
        char program[] = "blobgpu";
        char* args[] = {program, flag.data(), value.data()};
        if (flag.rfind("--g.", 0) == 0) {
            apply_genome_args(3, args, life.genome);
        } else {
            apply_minimal_args(3, args, life.mg); // exits on an unknown name
        }
    }
    return true;
}

bool gpu_lives_list(const GpuFlags& f, const Genome& genome, const MinimalGenome& mg,
                    GpuSettings& settings, std::vector<GpuLifeSpec>& lives) {
    lives.clear();
    if (!f.settings.empty()) {
        if (!read_settings(f.settings, settings)) {
            return false;
        }
        for (size_t r = 0; r < settings.rows.size(); r++) {
            GpuLifeSpec life{f.seed_from + r, genome, mg};
            apply_row(settings, settings.rows[r], life);
            if (f.device != GpuDevice::cpu || f.check) {
                life.mg.rng_mode = 1;
            }
            lives.push_back(life);
        }
    } else {
        for (int i = 0; i < f.lives; i++) {
            lives.push_back(GpuLifeSpec{f.seed_from + (uint64_t)i, genome, mg});
        }
    }
    if (lives.empty()) {
        std::fprintf(stderr, "blobgpu: no lives\n");
        return false;
    }
    if (f.twin) {
        size_t learning = lives.size();
        for (size_t i = 0; i < learning; i++) {
            GpuLifeSpec twin = lives[i];
            twin.mg.learning_on = 0;
            lives.push_back(twin);
        }
    }
    return true;
}
