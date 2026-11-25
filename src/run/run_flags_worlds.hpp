#pragma once

#include "world/task.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>

// the more-worlds flags (idea 63), kept apart so run_flags.cpp stays short.
// a header (included by run_flags.cpp only) so the build lists that name run_flags.cpp
// need no change

inline bool parse_world_word(const char* flag, const char* value, TaskSettings& s) {
    struct Word {
        const char* flag;
        const char* value;
        int* field;
        int number;
    };
    const Word words[] = {
        {"--cart-start", "narrow", &s.cart_start, 0},
        {"--cart-start", "wide", &s.cart_start, 1},
        {"--cart-integrator", "euler", &s.cart_semi, 0},
        {"--cart-integrator", "semi", &s.cart_semi, 1},
        {"--track-reward", "shaped", &s.track_reward, 0},
        {"--track-reward", "arrive", &s.track_reward, 1},
        {"--classic-reward", "energy", &s.classic_reward, 0},
        {"--classic-reward", "goal", &s.classic_reward, 1},
    };
    for (const Word& w : words) {
        if (std::strcmp(flag, w.flag) == 0 && std::strcmp(value, w.value) == 0) {
            *w.field = w.number;
            return true;
        }
    }
    std::fprintf(stderr, "blobrun: %s does not take '%s' (blobrun --help)\n", flag, value);
    return false;
}

inline bool parse_world_number(const char* flag, int64_t v, TaskSettings& s) {
    struct Number {
        const char* flag;
        int* field;
    };
    const Number numbers[] = {
        {"--bandit-good", &s.bandit_good},     {"--bandit-poor", &s.bandit_poor},
        {"--switch-every", &s.switch_every},   {"--chain-cue", &s.chain_cue},
        {"--track-size", &s.track_size},       {"--forage-size", &s.forage_size},
        {"--forage-warmer", &s.forage_warmer},
    };
    for (const Number& n : numbers) {
        if (std::strcmp(flag, n.flag) == 0) {
            *n.field = (int)v;
            return true;
        }
    }
    return false;
}
