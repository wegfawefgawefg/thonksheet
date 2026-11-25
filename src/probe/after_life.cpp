// changes made to a sheet after its life and before a probe reads it (idea 172,
// docs/results-2026-11-26-loops.md): the wiring stays the life's, only the dynamics change.
// Every probe that takes --life (transfer, timewire, looptrace) takes these:
//   --after "<--g.name value ...>"  genome numbers set on the lived sheet (regulation,
//                                   inhibition, the tile's winners, noise, ...)
//   --set-cool N                    every inner cell's cooldown after a spike is N ticks
//   --set-leak N                    every inner cell's leak shift is N (bigger: slower leak)
//   --quiet-pace 1                  pacemakers do not fire
// "inner": not a sensor cell, not a pacemaker. None of these is used unless asked for.

#include "blob/blob.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

// "--g.a 1 --g.b 2" onto a genome
static void genome_words(const char* text, Genome& g) {
    std::vector<std::string> words;
    std::string w;
    for (const char* p = text;; p++) {
        if (*p == ' ' || *p == '\0') {
            if (!w.empty()) {
                words.push_back(w);
            }
            w.clear();
            if (*p == '\0') {
                break;
            }
            continue;
        }
        w += *p;
    }
    std::vector<char*> argv;
    for (std::string& s : words) {
        argv.push_back(s.data());
    }
    apply_genome_args((int)argv.size(), argv.data(), g);
}

void after_life(int argc, char** argv, Blob& blob) {
    const char* after = text_arg(argc, argv, "--after", nullptr);
    if (after) {
        genome_words(after, blob.genome);
        if (blob.genome.tire_jump > 0 && !blob.tire) {
            std::fprintf(stderr, "after_life: tiring needs the sheet born with it\n");
            std::exit(1);
        }
    }
    int cool = int_arg(argc, argv, "--set-cool", -1);
    int leak = int_arg(argc, argv, "--set-leak", -1);
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t inner = ~(blob.sensor[t] | blob.pacemaker[t]);
        for (int b = 0; b < TILE_NEURONS; b++) {
            int n = t * TILE_NEURONS + b;
            if (!((inner >> b) & 1)) {
                continue;
            }
            blob.cooldown_len[n] = cool >= 0 ? (uint8_t)cool : blob.cooldown_len[n];
            blob.leak_shift[n] = leak >= 0 ? (uint8_t)leak : blob.leak_shift[n];
        }
    }
    if (int_arg(argc, argv, "--quiet-pace", 0)) {
        if (!blob.silenced) {
            blob.silenced = (uint64_t*)std::calloc((size_t)blob.tile_count + 1, sizeof(uint64_t));
        }
        for (int t = 0; t < blob.tile_count; t++) {
            blob.silenced[t] |= blob.pacemaker[t];
        }
    }
}
