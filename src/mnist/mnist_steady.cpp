#include "mnist/mnist_run.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// a steadier sheet for the life only (idea 174, docs/results-2026-11-26-steady.md,
// scripts/steady.sh). blobmnist ceiling flags, each off unless given; the read after the life
// is the same as without them:
//   --life-ticks N    the life's digits are shown N ticks (the read keeps --ticks)
//   --life-settle N   before the life, N digits with learning off (the life's ticks and, with
//                     --life-shuffle, its shuffled digits), so regulation settles first
//   --life-set A=V,B=W   genome numbers for the life only, e.g.
//                     threshold_reg_on=0,sector_bias_max=32; put back after it (a sector bias
//                     past the restored bound is clamped to it)
//   --life-target P   every sector's target rate is P percent of its own for the life only
//                     (the rate is held by the sector bias alone when thresholds are frozen,
//                     and the sector counts its motor cells too; this brings the hidden cells
//                     back to the rate they have under per-cell regulation)

static void set_ticks(Sheet& s, int ticks) {
    s.flags.plug.ticks_per_step = ticks;
    s.tt->ticks_per_step = ticks;
    s.tt->plug.settings.ticks_per_step = ticks;
}

// "a=1,b=2" as "--g.a 1 --g.b 2" for the genome's own parser
static void apply_life_set(Genome& g, const char* text) {
    std::vector<std::string> words;
    std::string item;
    for (const char* p = text;; p++) {
        if (*p == ',' || *p == 0) {
            size_t eq = item.find('=');
            if (eq == std::string::npos) {
                std::fprintf(stderr, "--life-set: want name=value, got '%s'\n", item.c_str());
                std::exit(1);
            }
            words.push_back("--g." + item.substr(0, eq));
            words.push_back(item.substr(eq + 1));
            item.clear();
            if (*p == 0) {
                break;
            }
            continue;
        }
        item += *p;
    }
    std::vector<char*> argv;
    for (std::string& w : words) {
        argv.push_back(&w[0]);
    }
    apply_genome_args((int)argv.size(), argv.data(), g);
}

void steady_life(Sheet& s, int argc, char** argv, int64_t digits,
                 const std::vector<uint64_t>& born) {
    Blob& b = *s.blob;
    int read_ticks = s.flags.plug.ticks_per_step;
    int life_ticks = int_flag(argc, argv, "--life-ticks", read_ticks);
    int settle = int_flag(argc, argv, "--life-settle", 0);
    const char* set = text_flag(argc, argv, "--life-set", nullptr);
    int target = int_flag(argc, argv, "--life-target", 100);
    set_ticks(s, life_ticks);
    if (settle > 0) {
        int on = b.genome.learning_on;
        b.genome.learning_on = 0;
        sheet_train(s, settle, false, nullptr);
        b.genome.learning_on = on;
    }
    Genome saved = b.genome;
    if (set) {
        apply_life_set(b.genome, set);
    }
    std::vector<uint32_t> targets(b.sector_target, b.sector_target + b.sector_count);
    for (int k = 0; k < b.sector_count; k++) {
        b.sector_target[k] = (uint32_t)((uint64_t)targets[(size_t)k] * (uint64_t)target / 100);
    }
    if (life_ticks != read_ticks || settle > 0 || set || target != 100) {
        std::printf("steady: life of %lld digits shown %d ticks, %d settling digits first, "
                    "life-only genome %s, sector targets %d%%\n",
                    (long long)digits, life_ticks, settle, set ? set : "-", target);
    }
    life_run(s, digits, 10, born);
    b.genome = saved;
    for (int k = 0; k < b.sector_count; k++) {
        b.sector_target[k] = targets[(size_t)k];
    }
    for (int k = 0; k < b.sector_count; k++) {
        int bias = b.sector_bias[k], max = b.genome.sector_bias_max;
        b.sector_bias[k] = (int16_t)(bias > max ? max : (bias < -max ? -max : bias));
    }
    set_ticks(s, read_ticks);
}
