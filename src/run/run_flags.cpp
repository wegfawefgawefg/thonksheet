#include "run/run_flags.hpp"

#include "world/mnist_data.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static const char* usage =
    "usage: blobrun [flags]\n"
    "  --task NAME        echo, shuffled-echo, delayed-echo, go-no-go, beat, sequence,\n"
    "                     catch, cartpole, order, both-not-either, gap, place, mnist,\n"
    "                     two-step, distract, serial-add, majority\n"
    "                     (default echo)\n"
    "  --seed N           blob seed (default 1)\n"
    "  --task-seed N      task seed (default: same as --seed)\n"
    "  --steps N          life length in task steps (default 20000)\n"
    "  --ticks N          blob ticks per task step (default 8)\n"
    "  --window N         task steps per score window (default 200)\n"
    "  --edge N           windows averaged for early and late (default: a tenth)\n"
    "  --out PATH         csv path (default out/<task>-s<seed>.csv)\n"
    "  --no-baselines     skip the learning-off and coin-flip runs\n"
    "  --show-plug        print where each line sits on the sheet\n"
    "  --engine NAME      blob (default), minimal, memory, mlp, gru or vm; --m.name value\n"
    "                     sets a minimal number, --y.name a memory number, --n.name a\n"
    "                     reference network number (mlp, gru: the fixed reference), --v.name\n"
    "                     a vm number\n"
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
    "  --catch-size W H   catch: sheet size (default 6 6)\n"
    "  --number-code C    how a number becomes lines: bar (default) or place (one line on)\n"
    "  --cart-reward R    cartpole: sparse (default), alive, or lean (good when it ends a\n"
    "                     step nearer upright and centred, bad when further)\n"
    "  --cue-offset N     echo family: cues and buttons start at line N (default 0)\n"
    "  --order-gap N      order: steps the first cue is on alone (default 1)\n"
    "  --gap-short N      gap: blank steps between blinks, short (default 1)\n"
    "  --gap-long N       gap: blank steps between blinks, long (default 3)\n"
    "  --place-size N     place: the sheet is N x N (default 4)\n"
    "  --place-hold N     place: places never rewarded, the scored ones (default 3)\n"
    "  --distract N       distract: distracting events between cue and ask (default 4)\n"
    "  --add-bias N       serial-add: chance x16 a pair passes the carry on (default 12)\n"
    "  --maj-hold N       majority: patterns of 32 never rewarded, the scored ones (default 6)\n"
    "  --mnist-code C     mnist: t14 (default), g14 or t28 (src/world/mnist_data.hpp)\n"
    "  --mnist-first N    mnist: first training digit used (default 0)\n"
    "  --mnist-count N    mnist: training digits used from there (default 0: the rest)\n"
    "  --distance N       plug: cue lines and motor lines N tiles apart (default: packed,\n"
    "                     1 tile with the default genome)\n"
    "  --motor-spread N   plug: N motor cells per button, dealt like a checkerboard over\n"
    "                     the motor region, nearest the cues first; more than fit means\n"
    "                     all of them (default 0: one group)\n"
    "  --motor-spread-even 1  the N cells spread evenly from nearest to farthest instead\n"
    "life protocols (one at a time; without them, one task for the whole life):\n"
    "  --retention TASK   --task for --steps, then TASK for --steps (its cues and buttons\n"
    "                     after A's, cue offset + k), then --task again for --retest steps\n"
    "  --idle N           --task for --steps, then N ticks unplugged, then --retest steps\n"
    "  --retest N         steps of the retest (default steps / 4)\n"
    "  --capacity         echo with 2, 4, 8, 16 cues, a life each; largest k above halfway\n"
    "                     from chance (1/k) to perfect\n"
    "  --reach            how many tiles a cue can be read from activity, no reward\n"
    "  --reach-settle N   reach: ticks before measuring (default 30000)\n"
    "  --reach-blocks N   reach: blocks of 20 ticks measured (default 4000)\n"
    "  --speed            ticks per second at 8x8, 16x16, 32x32 tiles; bytes per neuron\n"
    "  --speed-ms N       speed: time at each size (default 2000)\n"
    "  --frozen-readout   only connections into motor neurons learn (genome learn_mode 1)\n"
    "  --alive-ticks N    end of life: ticks counted for rate, silent, stuck (default 16384)\n"
    "  --rhythm-ticks N   end of life: idle ticks recorded for the rhythm (default 16384)\n"
    "Each life also writes <csv stem>-sum.csv: driver,key,value for every number printed.\n";

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
        if (std::strcmp(flag, "--capacity") == 0) {
            flags.protocol = Protocol::capacity;
            continue;
        }
        if (std::strcmp(flag, "--reach") == 0) {
            flags.protocol = Protocol::reach;
            continue;
        }
        if (std::strcmp(flag, "--speed") == 0) {
            flags.protocol = Protocol::speed;
            continue;
        }
        if (std::strcmp(flag, "--frozen-readout") == 0) {
            flags.frozen_readout = true;
            continue;
        }
        if (std::strcmp(flag, "--table") == 0) {
            flags.table = true;
            continue;
        }
        if (i + 1 >= argc) {
            std::fprintf(stderr, "blobrun: %s needs a value\n%s", flag, usage);
            return false;
        }
        const char* value = argv[++i];
        if (std::strncmp(flag, "--g.", 4) == 0 || std::strncmp(flag, "--m.", 4) == 0 ||
            std::strncmp(flag, "--t.", 4) == 0 || std::strncmp(flag, "--n.", 4) == 0 ||
            std::strncmp(flag, "--y.", 4) == 0 || std::strncmp(flag, "--v.", 4) == 0) {
            continue; // a genome number: apply_genome_args, apply_minimal_args,
                      // table_genome_args, apply_nn_args, apply_memory_args, apply_vm_args
        }
        if (std::strcmp(flag, "--engine") == 0) {
            flags.engine = value;
            continue;
        }
        if (std::strcmp(flag, "--number-code") == 0) {
            if (std::strcmp(value, "bar") == 0) {
                flags.plug.number_code = 0;
            } else if (std::strcmp(value, "place") == 0) {
                flags.plug.number_code = 1;
            } else {
                std::fprintf(stderr, "blobrun: --number-code is bar or place\n");
                return false;
            }
            continue;
        }
        if (std::strcmp(flag, "--cart-reward") == 0) {
            if (std::strcmp(value, "sparse") == 0) {
                flags.task_settings.cart_reward = 0;
            } else if (std::strcmp(value, "alive") == 0) {
                flags.task_settings.cart_reward = 1;
            } else if (std::strcmp(value, "lean") == 0) {
                flags.task_settings.cart_reward = 2;
            } else {
                std::fprintf(stderr, "blobrun: --cart-reward is sparse, alive or lean\n");
                return false;
            }
            continue;
        }
        if (std::strcmp(flag, "--mnist-code") == 0) {
            if (!mnist_code_from_name(value, flags.task_settings.mnist_code)) {
                std::fprintf(stderr, "blobrun: --mnist-code is t14, g14 or t28\n");
                return false;
            }
            continue;
        }
        if (std::strcmp(flag, "--task") == 0) {
            if (!task_from_name(value, flags.task)) {
                std::fprintf(stderr, "blobrun: no task '%s'\n%s", value, usage);
                return false;
            }
            continue;
        }
        if (std::strcmp(flag, "--retention") == 0) {
            if (!task_from_name(value, flags.task_b)) {
                std::fprintf(stderr, "blobrun: no task '%s'\n%s", value, usage);
                return false;
            }
            flags.protocol = Protocol::retention;
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
        } else if (std::strcmp(flag, "--cue-offset") == 0) {
            flags.task_settings.cue_offset = (int)v;
        } else if (std::strcmp(flag, "--order-gap") == 0) {
            flags.task_settings.order_gap = (int)v;
        } else if (std::strcmp(flag, "--gap-short") == 0) {
            flags.task_settings.gap_short = (int)v;
        } else if (std::strcmp(flag, "--gap-long") == 0) {
            flags.task_settings.gap_long = (int)v;
        } else if (std::strcmp(flag, "--place-size") == 0) {
            flags.task_settings.place_size = (int)v;
        } else if (std::strcmp(flag, "--place-hold") == 0) {
            flags.task_settings.place_hold = (int)v;
        } else if (std::strcmp(flag, "--distract") == 0) {
            flags.task_settings.distract = (int)v;
        } else if (std::strcmp(flag, "--add-bias") == 0) {
            flags.task_settings.add_bias = (int)v;
        } else if (std::strcmp(flag, "--maj-hold") == 0) {
            flags.task_settings.maj_hold = (int)v;
        } else if (std::strcmp(flag, "--mnist-first") == 0) {
            flags.task_settings.mnist_first = (int)v;
        } else if (std::strcmp(flag, "--mnist-count") == 0) {
            flags.task_settings.mnist_count = (int)v;
        } else if (std::strcmp(flag, "--distance") == 0) {
            flags.plug.distance = (int)v;
        } else if (std::strcmp(flag, "--motor-spread") == 0) {
            flags.plug.motor_spread = (int)v;
        } else if (std::strcmp(flag, "--motor-spread-even") == 0) {
            flags.plug.motor_spread_even = (int)v;
        } else if (std::strcmp(flag, "--idle") == 0) {
            flags.idle_ticks = v;
            flags.protocol = Protocol::idle;
        } else if (std::strcmp(flag, "--retest") == 0) {
            flags.retest = v;
        } else if (std::strcmp(flag, "--reach-settle") == 0) {
            flags.reach_settle = (int)v;
        } else if (std::strcmp(flag, "--reach-blocks") == 0) {
            flags.reach_blocks = (int)v;
        } else if (std::strcmp(flag, "--speed-ms") == 0) {
            flags.speed_ms = (int)v;
        } else if (std::strcmp(flag, "--alive-ticks") == 0) {
            flags.alive_ticks = (int)v;
        } else if (std::strcmp(flag, "--rhythm-ticks") == 0) {
            flags.rhythm_ticks = (int)v;
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
    if (flags.retest <= 0) {
        flags.retest = flags.steps / 4 > flags.window ? flags.steps / 4 : flags.window;
    }
    return true;
}
