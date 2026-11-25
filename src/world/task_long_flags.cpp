#include "world/task_long.hpp"

#include <cstring>

// the long-memory worlds' numbers on the blobrun command line (task_long.hpp)

const char* const long_usage =
    "long memory (hold, pairs, once, rare, drift; docs/curriculum.md \"Long memory\"):\n"
    "  --long             the long-life protocol: per-bucket scores per window, and the\n"
    "                     forgetting table at the end (life_long.cpp)\n"
    "  --hold-steps N     hold: blank steps between cue and ask (default 1)\n"
    "  --hold-cues N      hold: cues and buttons (default 2)\n"
    "  --hold-noise N     hold: 0 none, 1 distractor lines in the gap, 2 the cue lines\n"
    "                     themselves in the gap with the real cue marked (default 0)\n"
    "  --noise-shift N    hold: a distractor flashes per gap step with chance 2^-N (2)\n"
    "  --lines N --on N   pattern worlds: N lines, patterns of `on` lit (28, 6)\n"
    "  --buttons N        pattern worlds: buttons (default 4)\n"
    "  --pair-every N     pairs, once: steps between new pairs (default 400)\n"
    "  --pair-live N      pairs: newest pairs taught side by side (default 4)\n"
    "  --age-base N       pairs, once: first age asked; then x10 four times (default 100)\n"
    "  --probe-reward N   pairs: 1 rewards the probes (default 0)\n"
    "  --rare-common N --rare-count N --rare-every N   rare: (8, 4, 1000)\n"
    "  --drift-pairs N --drift-at N                    drift: (8, 100000)\n";

bool long_flag(const char* flag, int64_t v, LongSettings& ls) {
    struct Field {
        const char* name;
        int LongSettings::*field;
    };
    static const Field fields[] = {
        {"--hold-steps", &LongSettings::hold_steps},
        {"--hold-cues", &LongSettings::hold_cues},
        {"--hold-noise", &LongSettings::hold_noise},
        {"--noise-shift", &LongSettings::noise_shift},
        {"--lines", &LongSettings::lines},
        {"--on", &LongSettings::on},
        {"--buttons", &LongSettings::buttons},
        {"--pair-every", &LongSettings::pair_every},
        {"--pair-live", &LongSettings::pair_live},
        {"--age-base", &LongSettings::age_base},
        {"--probe-reward", &LongSettings::probe_reward},
        {"--rare-common", &LongSettings::rare_common},
        {"--rare-count", &LongSettings::rare_count},
        {"--rare-every", &LongSettings::rare_every},
        {"--drift-pairs", &LongSettings::drift_pairs},
    };
    for (const Field& f : fields) {
        if (std::strcmp(flag, f.name) == 0) {
            ls.*f.field = (int)v;
            return true;
        }
    }
    if (std::strcmp(flag, "--drift-at") == 0) {
        ls.drift_at = v;
        return true;
    }
    return false;
}
