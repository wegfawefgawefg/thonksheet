#include "run/run_flags.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static const char* usage =
    "usage: blobrun [flags]\n"
    "  --task NAME        echo, shuffled-echo, delayed-echo, go-no-go, beat, sequence,\n"
    "                     catch, cartpole (default echo)\n"
    "  --seed N           blob seed (default 1)\n"
    "  --task-seed N      task seed (default: same as --seed)\n"
    "  --steps N          life length in task steps (default 20000)\n"
    "  --ticks N          blob ticks per task step (default 8)\n"
    "  --window N         task steps per score window (default 200)\n"
    "  --edge N           windows averaged for early and late (default: a tenth)\n"
    "  --out PATH         csv path (default out/<task>-s<seed>.csv)\n"
    "  --no-baselines     skip the learning-off and coin-flip runs\n"
    "  --show-plug        print where each line sits on the sheet\n"
    "  --engine NAME      blob (default) or minimal; --m.name value sets a minimal number\n"
    "plug:\n"
    "  --sensor-side N    a sensor line drives N x N neurons (default 2)\n"
    "  --motor-side N     a motor line is read from N x N neurons (default 2)\n"
    "  --reward-ticks N   ticks the good/bad line stays on (default 2)\n"
    "  --rest-ticks N     quiet ticks after each rewarded step (default 0)\n"
    "task:\n"
    "  --k N              echo family: cues and buttons (default 2)\n"
    "  --delay N          delayed echo: blank steps before the ask (default 2)\n"
    "  --period N         beat: press every N steps (default 4)\n"
    "  --seq-lines N      sequence: lines (default 8)\n"
    "  --seq-length N     sequence: steps before it repeats (default 4)\n"
    "  --catch-size W H   catch: sheet size (default 6 6)\n";

static bool read_int(const char* text, int64_t& out) {
    char* end = nullptr;
    long long v = std::strtoll(text, &end, 10);
    if (end == text || *end != '\0') {
        std::fprintf(stderr, "blobrun: '%s' is not a whole number\n", text);
        return false;
    }
    out = v;
    return true;
}

bool run_flags_parse(int argc, char** argv, RunFlags& flags) {
    for (int i = 1; i < argc; i++) {
        const char* flag = argv[i];
        if (std::strcmp(flag, "--help") == 0 || std::strcmp(flag, "-h") == 0) {
            std::fputs(usage, stderr);
            return false;
        }
        if (std::strcmp(flag, "--no-baselines") == 0) {
            flags.baselines = false;
            continue;
        }
        if (std::strcmp(flag, "--show-plug") == 0) {
            flags.show_plug = true;
            continue;
        }
        if (i + 1 >= argc) {
            std::fprintf(stderr, "blobrun: %s needs a value\n%s", flag, usage);
            return false;
        }
        const char* value = argv[++i];
        if (std::strncmp(flag, "--g.", 4) == 0 || std::strncmp(flag, "--m.", 4) == 0) {
            continue; // a genome number, read by apply_genome_args / apply_minimal_args
        }
        if (std::strcmp(flag, "--engine") == 0) {
            flags.engine = value;
            continue;
        }
        if (std::strcmp(flag, "--task") == 0) {
            if (!task_from_name(value, flags.task)) {
                std::fprintf(stderr, "blobrun: no task '%s'\n%s", value, usage);
                return false;
            }
            continue;
        }
        if (std::strcmp(flag, "--out") == 0) {
            flags.out = value;
            continue;
        }
        int64_t v = 0;
        if (!read_int(value, v)) {
            return false;
        }
        if (std::strcmp(flag, "--seed") == 0) {
            flags.seed = (uint64_t)v;
        } else if (std::strcmp(flag, "--task-seed") == 0) {
            flags.task_seed = (uint64_t)v;
        } else if (std::strcmp(flag, "--steps") == 0) {
            flags.steps = v;
        } else if (std::strcmp(flag, "--ticks") == 0) {
            flags.plug.ticks_per_step = (int)v;
        } else if (std::strcmp(flag, "--window") == 0) {
            flags.window = (int)v;
        } else if (std::strcmp(flag, "--edge") == 0) {
            flags.edge = (int)v;
        } else if (std::strcmp(flag, "--sensor-side") == 0) {
            flags.plug.sensor_side = (int)v;
        } else if (std::strcmp(flag, "--motor-side") == 0) {
            flags.plug.motor_side = (int)v;
        } else if (std::strcmp(flag, "--reward-ticks") == 0) {
            flags.plug.reward_line_ticks = (int)v;
        } else if (std::strcmp(flag, "--rest-ticks") == 0) {
            flags.plug.rest_ticks = (int)v;
        } else if (std::strcmp(flag, "--k") == 0) {
            flags.task_settings.k = (int)v;
        } else if (std::strcmp(flag, "--delay") == 0) {
            flags.task_settings.delay = (int)v;
        } else if (std::strcmp(flag, "--period") == 0) {
            flags.task_settings.period = (int)v;
        } else if (std::strcmp(flag, "--seq-lines") == 0) {
            flags.task_settings.seq_lines = (int)v;
        } else if (std::strcmp(flag, "--seq-length") == 0) {
            flags.task_settings.seq_length = (int)v;
        } else if (std::strcmp(flag, "--catch-size") == 0) {
            int64_t h = 0;
            if (i + 1 >= argc || !read_int(argv[++i], h)) {
                std::fprintf(stderr, "blobrun: --catch-size needs W H\n");
                return false;
            }
            flags.task_settings.catch_w = (int)v;
            flags.task_settings.catch_h = (int)h;
        } else {
            std::fprintf(stderr, "blobrun: unknown flag %s\n%s", flag, usage);
            return false;
        }
    }
    if (flags.steps < 1 || flags.window < 1 || flags.plug.ticks_per_step < 1) {
        std::fprintf(stderr, "blobrun: --steps, --window and --ticks must be at least 1\n");
        return false;
    }
    if (flags.steps < flags.window) {
        std::fprintf(stderr, "blobrun: --steps is shorter than one window\n");
        return false;
    }
    if (flags.task_seed == 0) {
        flags.task_seed = flags.seed;
    }
    return true;
}
