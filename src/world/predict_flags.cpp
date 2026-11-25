#include "world/predict_settings.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// idea 162's flags, and the skill report (predict_settings.hpp; the cells: run/predict.hpp)

const char* const predict_flags_usage =
    "every line is a target (idea 162; needs --g.learn_mode 5 and free motor cells):\n"
    "  --predict H,H,..     predictor cells for every input line at these horizons in\n"
    "                       steps (1..15, at most 4), e.g. 1,2,4,8; 0 is off (default)\n"
    "  --predict-press 1    the sheet's last press comes back as one line per button\n"
    "  --predict-errors 1   the shortest horizon's errors come back as two lines per\n"
    "                       predicted line (said on and was off, said off and was on)\n"
    "  --predict-cells N    most motor cells per answer group (default 4)\n"
    "  --predict-shift S    chance of a verdict's move 2^-S (default: the readout's)\n"
    "  --predict-score-from N   first step the skill is tallied over (default: last quarter)\n";

static int parse_horizons(const char* value, PredictSettings& s) {
    s.horizons = 0;
    const char* at = value;
    while (*at) {
        char* end = nullptr;
        long h = std::strtol(at, &end, 10);
        if (end == at || h < 0 || h >= PREDICT_RING) {
            std::fprintf(stderr, "blobrun: --predict takes horizons 1..%d, like 1,2,4,8\n",
                         PREDICT_RING - 1);
            return -1;
        }
        if (h > 0) {
            if (s.horizons == PREDICT_MAX_HORIZONS) {
                std::fprintf(stderr, "blobrun: --predict takes at most %d horizons\n",
                             PREDICT_MAX_HORIZONS);
                return -1;
            }
            // kept in ascending order: the first is the shortest (the error lines' horizon)
            int i = s.horizons++;
            for (; i > 0 && s.horizon[i - 1] > h; i--) {
                s.horizon[i] = s.horizon[i - 1];
            }
            s.horizon[i] = (int)h;
        }
        at = *end == ',' ? end + 1 : end;
        if (*end && *end != ',') {
            std::fprintf(stderr, "blobrun: --predict: bad list '%s'\n", value);
            return -1;
        }
    }
    return 1;
}

int predict_parse_flag(const char* flag, const char* value, PredictSettings& s) {
    if (std::strncmp(flag, "--predict", 9) != 0) {
        return 0;
    }
    if (std::strcmp(flag, "--predict") == 0) {
        return parse_horizons(value, s);
    }
    struct Number {
        const char* flag;
        int* field;
    };
    const Number numbers[] = {{"--predict-press", &s.press},
                              {"--predict-errors", &s.errors},
                              {"--predict-cells", &s.cells},
                              {"--predict-shift", &s.shift}};
    for (const Number& n : numbers) {
        if (std::strcmp(flag, n.flag) == 0) {
            *n.field = std::atoi(value);
            return 1;
        }
    }
    if (std::strcmp(flag, "--predict-score-from") == 0) {
        s.score_from = std::atoll(value);
        return 1;
    }
    return 0;
}

static double share(int64_t a, int64_t b) {
    return b > 0 ? (double)a / (double)b : 0.0;
}

// the best constant per line: its more common value over the scored steps. right shares
// over all line-steps, on on-steps and on off-steps
struct Common {
    double all = 0, on = 0, off = 0;
};

static Common common_share(const PredictTally& t, int targets) {
    int64_t right = 0, on_right = 0, off_right = 0;
    for (int l = 0; l < targets; l++) {
        int64_t on = t.line_on[l], off = t.steps - on;
        right += on > off ? on : off;
        on_right += on > off ? on : 0;
        off_right += on > off ? 0 : off;
    }
    return {share(right, t.steps * targets), share(on_right, t.on),
            share(off_right, t.total - t.on)};
}

// the "same as now" guess's right share on off-steps: off now and h before
static double same_off(const PredictTally& t) {
    return share(t.same - t.same_on, t.total - t.on);
}

void predict_print(const PredictScore& s, const char* who, FILE* out) {
    if (s.horizons < 1) {
        return;
    }
    std::fprintf(out,
                 "%-9s predict: %d lines, %d cells per group, %lld verdicts. share of "
                 "line-steps right over the scored steps:\n",
                 who, s.targets, s.cells_per_group, (long long)s.verdicts);
    std::fprintf(out, "%-9s   %3s %7s %7s %7s %6s %8s %7s %7s %7s  %8s %7s %7s\n", who, "h",
                 "right", "same", "common", "tied", "changed", "right", "on", "off", "balanced",
                 "same", "common");
    for (int i = 0; i < s.horizons; i++) {
        const PredictTally& t = s.tally[i];
        Common c = common_share(t, s.targets);
        double on = share(t.on_right, t.on), off = share(t.off_right, t.total - t.on);
        double same_on = share(t.same_on, t.on);
        std::fprintf(out,
                     "%-9s   %3d %7.4f %7.4f %7.4f %6.3f %8.4f %7.4f %7.4f %7.4f  %8.4f %7.4f "
                     "%7.4f\n",
                     who, s.horizon[i], share(t.right, t.total), share(t.same, t.total), c.all,
                     share(t.tied, t.total), share(t.changed, t.total),
                     share(t.changed_right, t.changed), on, off, (on + off) / 2,
                     (same_on + same_off(t)) / 2, (c.on + c.off) / 2);
    }
}

void predict_write_sum(const PredictScore& s, const char* who, FILE* sum) {
    if (s.horizons < 1 || !sum) {
        return;
    }
    std::fprintf(sum, "%s,pred_lines,%d\n%s,pred_cells,%d\n%s,pred_verdicts,%lld\n", who, s.targets,
                 who, s.cells_per_group, who, (long long)s.verdicts);
    for (int i = 0; i < s.horizons; i++) {
        const PredictTally& t = s.tally[i];
        Common c = common_share(t, s.targets);
        const struct {
            const char* key;
            double value;
        } rows[] = {
            {"right", share(t.right, t.total)},
            {"same", share(t.same, t.total)},
            {"common", c.all},
            {"common_balanced", (c.on + c.off) / 2},
            {"same_balanced", (share(t.same_on, t.on) + same_off(t)) / 2},
            {"balanced", (share(t.on_right, t.on) + share(t.off_right, t.total - t.on)) / 2},
            {"tied", share(t.tied, t.total)},
            {"changed", share(t.changed, t.total)},
            {"changed_right", share(t.changed_right, t.changed)},
            {"on", share(t.on, t.total)},
            {"on_right", share(t.on_right, t.on)},
            {"off_right", share(t.off_right, t.total - t.on)},
        };
        for (const auto& r : rows) {
            std::fprintf(sum, "%s,pred_h%d_%s,%.6g\n", who, s.horizon[i], r.key, r.value);
        }
    }
}
