#include "state.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static const char* const usage =
    "blobview: watch a blob tick\n"
    "  --fake [mixed|healthy|dying|saturating|cycling]  the viewer's own fake blob\n"
    "  --real                 the core's blob (the default when built with it)\n"
    "  --tiles W H            sheet size in tiles (8x8 neurons each)\n"
    "  --seed N\n"
    "  --capture out.png      no window: run --ticks N ticks, draw one frame to a PNG\n"
    "  --ticks N              (default 2000)\n"
    "  --size W H             window or capture size (default 1600 1000)\n"
    "  --select N | --select-cell X Y   a neuron picked at the start\n"
    "  --raster neurons|tiles a raster row per neuron or per tile (default: by size)\n"
    "  --paused | --slow TPS | --turbo  start mode (default full speed, drawn)\n"
    "  --input                random bits on the sensor neurons (real blob)\n"
    "  --frames N             window: quit after N frames\n"
    "  --fullscreen  --validation\n";

static int scene_from_name(const char* name) {
    for (int i = 0; i < FAKE_SCENE_COUNT; ++i) {
        if (std::strcmp(name, fake_scene_names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

bool parse_cli(CliOptions& o, int argc, char** argv) {
#if BLOB_HAS_CORE
    o.fake = false;
#endif
    int select_x = -1, select_y = -1;
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        auto next = [&]() -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "blobview: %s wants a value\n", a);
                std::exit(1);
            }
            return argv[++i];
        };
        if (std::strcmp(a, "--fake") == 0) {
            o.fake = true;
            if (i + 1 < argc && scene_from_name(argv[i + 1]) >= 0) {
                o.scene = static_cast<FakeScene>(scene_from_name(next()));
            }
        } else if (std::strcmp(a, "--real") == 0) {
            o.fake = false;
        } else if (std::strcmp(a, "--tiles") == 0) {
            o.tiles_w = std::atoi(next());
            o.tiles_h = std::atoi(next());
        } else if (std::strcmp(a, "--seed") == 0) {
            o.seed = std::strtoull(next(), nullptr, 10);
        } else if (std::strcmp(a, "--capture") == 0) {
            o.capture = next();
        } else if (std::strcmp(a, "--ticks") == 0) {
            o.ticks = std::atoi(next());
        } else if (std::strcmp(a, "--size") == 0) {
            o.width = std::atoi(next());
            o.height = std::atoi(next());
        } else if (std::strcmp(a, "--select") == 0) {
            o.select = std::atoi(next());
        } else if (std::strcmp(a, "--select-cell") == 0) {
            select_x = std::atoi(next());
            select_y = std::atoi(next());
        } else if (std::strcmp(a, "--raster") == 0) {
            o.raster_rows = std::strcmp(next(), "tiles") == 0 ? 1 : 0;
        } else if (std::strcmp(a, "--paused") == 0) {
            o.mode = RUN_PAUSED;
        } else if (std::strcmp(a, "--slow") == 0) {
            o.mode = RUN_SLOW;
            o.slow_tps = std::atoi(next());
        } else if (std::strcmp(a, "--turbo") == 0) {
            o.mode = RUN_TURBO;
        } else if (std::strcmp(a, "--input") == 0) {
            o.input = true;
        } else if (std::strcmp(a, "--frames") == 0) {
            o.frames = std::atoi(next());
        } else if (std::strcmp(a, "--fullscreen") == 0) {
            o.fullscreen = true;
        } else if (std::strcmp(a, "--validation") == 0) {
            o.validation = true;
        } else if (std::strcmp(a, "--help") == 0 || std::strcmp(a, "-h") == 0) {
            std::fputs(usage, stdout);
            std::exit(0);
        } else {
            std::fprintf(stderr, "blobview: unknown option %s\n%s", a, usage);
            return false;
        }
    }
    const SimSettings settings = sim_settings(o);
    if (select_x >= 0) {
        const int tw = settings.genome.tiles_w;
        o.select = ((select_y / 8) * tw + select_x / 8) * 64 + (select_y % 8) * 8 + select_x % 8;
    }
    return true;
}

SimSettings sim_settings(const CliOptions& o) {
    SimSettings s;
    s.fake = o.fake;
    s.scene = o.scene;
    s.seed = o.seed;
    if (o.tiles_w > 0 && o.tiles_h > 0) {
        s.genome.tiles_w = o.tiles_w;
        s.genome.tiles_h = o.tiles_h;
    }
    return s;
}

void set_mode(State& s, int mode) {
    s.mode = mode;
    s.sim.shared.mode = mode;
}
