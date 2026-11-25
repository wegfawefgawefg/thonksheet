#include "state.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static const char* const usage =
    "blobview: watch a blob tick\n"
    "  --fake [mixed|healthy|dying|saturating|cycling]  the viewer's own fake blob\n"
    "  --real                 an engine (the default when built with one)\n"
    "  --engine minimal       the minimal engine (--m.NAME N sets its numbers)\n"
    "  --table                table neurons (--t.NAME N sets their numbers)\n"
    "  --tiles W H            sheet size in tiles (8x8 neurons each)\n"
    "  --seed N\n"
    "  --capture out.png      no window: run --until N ticks, draw one frame to a PNG\n"
    "  --until N              (default 2000)\n"
    "  --task NAME [flags]    run a task through the world plug, with blobrun's task, plug\n"
    "                         and window flags (blobrun --help); --ticks is ticks per step\n"
    "  --no-twin              no learning-off twin for the score plot (coin flips still run)\n"
    "  --g.NAME N             a genome number, as for blobrun\n"
    "  --probe FEATURE        the readability probe is the world: which, order, xor, gap or\n"
    "                         hold on one tile, measured again and again, the sheet tinted\n"
    "  --measure FEATURE      capture: measure FEATURE's readability once at the end\n"
    "  --probe-tries N        tries per measurement (default 1000)\n"
    "  --probe-settle N       ticks of probe input before counting (default 300)\n"
    "  --probe-cue X Y        the cue tile (default: the task's first cue line, else 1 1)\n"
    "  --hold-gap N           hold: ticks from the cue going off to the reading (default 20)\n"
    "  --size W H             window or capture size (default 1600 1000)\n"
    "  --select N | --select-cell X Y   a neuron picked at the start\n"
    "  --select-button B      a cell of button B picked at the start (the readout)\n"
    "  --raster neurons|tiles a raster row per neuron or per tile (default: by size)\n"
    "  --paused | --slow TPS | --turbo  start mode (default full speed, drawn)\n"
    "  --input                random bits on the sensor neurons (real engine)\n"
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

static int feature_arg(const char* name) {
    Feature f = Feature::which;
    if (!feature_from_name(name, f)) {
        std::fprintf(stderr, "blobview: a feature is which, order, xor, gap or hold\n");
        std::exit(1);
    }
    return static_cast<int>(f);
}

// the flags blobview does not know go to blobrun's parser, so a task and an engine are set
// up exactly as blobrun sets them up. Genome numbers (--g.NAME, --m.NAME, --t.NAME) apply
// with or without a task.
static bool run_flags(CliOptions& o, int argc, char** argv, std::vector<char*>& run_args) {
    apply_genome_args(argc, argv, o.run.genome);
#if BLOB_HAS_MINIMAL
    apply_minimal_args(argc, argv, o.minimal);
#endif
#if BLOB_HAS_TABLE
    table_genome_args(argc, argv, o.table);
#endif
    for (size_t i = 1; !o.task && i < run_args.size(); ++i) {
        if (std::strcmp(run_args[i], "--ticks") == 0) {
            std::fprintf(stderr, "blobview: --ticks is ticks per task step (as in blobrun); "
                                 "a capture runs --until N ticks\n");
            return false;
        }
    }
    if (!run_flags_parse(static_cast<int>(run_args.size()), run_args.data(), o.run)) {
        return false;
    }
    if (o.run.engine != "blob" && o.run.engine != "minimal") {
        std::fprintf(stderr,
                     "blobview: no engine '%s' (blob or minimal; --table for table "
                     "neurons)\n",
                     o.run.engine.c_str());
        return false;
    }
    if (o.run.frozen_readout) {
        o.run.genome.learn_mode = 1; // as blobrun's --frozen-readout
    }
    o.twin = o.twin && o.run.baselines;
    return true;
}

bool parse_cli(CliOptions& o, int argc, char** argv) {
#if BLOB_HAS_CORE
    o.fake = false;
#endif
    int select_x = -1, select_y = -1;
    std::vector<char*> run_args = {argv[0]};
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
            run_args.push_back(argv[i]);
            o.seed = std::strtoull(next(), nullptr, 10);
            run_args.push_back(argv[i]);
        } else if (std::strcmp(a, "--capture") == 0) {
            o.capture = next();
        } else if (std::strcmp(a, "--until") == 0) {
            o.until = std::atoll(next());
        } else if (std::strcmp(a, "--no-twin") == 0) {
            o.twin = false;
        } else if (std::strcmp(a, "--probe") == 0) {
            o.probe.feature = feature_arg(next());
        } else if (std::strcmp(a, "--measure") == 0) {
            o.measure = feature_arg(next());
        } else if (std::strcmp(a, "--probe-tries") == 0) {
            o.probe.tries = std::max(20, std::atoi(next()));
        } else if (std::strcmp(a, "--probe-settle") == 0) {
            o.probe.settle = std::max(0, std::atoi(next()));
        } else if (std::strcmp(a, "--probe-cue") == 0) {
            o.probe.cue_x = std::atoi(next());
            o.probe.cue_y = std::atoi(next());
        } else if (std::strcmp(a, "--hold-gap") == 0) {
            o.probe.hold_gap = std::max(0, std::atoi(next()));
        } else if (std::strcmp(a, "--size") == 0) {
            o.width = std::atoi(next());
            o.height = std::atoi(next());
        } else if (std::strcmp(a, "--select") == 0) {
            o.select = std::atoi(next());
        } else if (std::strcmp(a, "--select-cell") == 0) {
            select_x = std::atoi(next());
            select_y = std::atoi(next());
        } else if (std::strcmp(a, "--select-button") == 0) {
            o.select_button = std::atoi(next());
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
            // blobrun's flags and their values, in order, for run_flags_parse
            o.task = o.task || std::strcmp(a, "--task") == 0;
            run_args.push_back(argv[i]);
        }
    }
    if (!run_flags(o, argc, argv, run_args)) {
        return false;
    }
    const SimSettings settings = sim_settings(o);
    if (select_x >= 0) {
        const int tw = settings.engine.genome.tiles_w;
        o.select = ((select_y / 8) * tw + select_x / 8) * 64 + (select_y % 8) * 8 + select_x % 8;
    }
    return true;
}

SimSettings sim_settings(const CliOptions& o) {
    SimSettings s;
    s.engine.engine = o.fake                      ? ENGINE_FAKE
                      : o.run.table               ? ENGINE_TABLE
                      : o.run.engine == "minimal" ? ENGINE_MINIMAL
                                                  : ENGINE_BLOB;
    s.engine.genome = o.run.genome;
    s.engine.minimal = o.minimal;
    s.engine.table = o.table;
    s.scene = o.scene;
    s.seed = o.seed;
    s.task = o.task;
    s.twin = o.twin;
    s.run = o.run;
    s.probe = o.probe;
    if (o.tiles_w > 0 && o.tiles_h > 0) {
        s.engine.genome.tiles_w = o.tiles_w;
        s.engine.genome.tiles_h = o.tiles_h;
    }
    s.run.genome = s.engine.genome;
    return s;
}

void set_mode(State& s, int mode) {
    s.mode = mode;
    s.sim.shared.mode = mode;
}
