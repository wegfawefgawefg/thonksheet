#include "blob/learn_rule.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// idea 120: reading a rule table from its hex string, and compiling it into cubes.
//
// the string: 8 header digits, then 1,024 entry digits, entry i for fact pattern i (bit f of
// i is fact f, learn_rule.hpp). header: the four chance shifts (0..15), the window (1..15, 0
// means 15), the read-off (0 evidence at strength 4, 1 evidence by counter, 2 direct), the
// balance (0 or 1), and a 0. an entry digit is action * 4 + chance: action 0 nothing, 1 up,
// 2 down, 3 toward zero; chance picks one of the four shifts.

static RuleTable loaded_rule;

const RuleTable& rule_table() {
    return loaded_rule;
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

// merge entries of one group that share action and chance and differ in one source fact,
// until nothing merges. fact 0 is never merged: it is counted per tick, not once
static void compile_group(RuleTable& rule, int g) {
    RuleCube* cube = rule.cube[g];
    int count = 0;
    for (int s = 0; s < 32; s++) {
        uint8_t e = rule.entry[(g << RULE_SOURCE_FACTS) | s];
        if ((e >> 2) == rule_nothing) {
            continue;
        }
        cube[count++] = RuleCube{0x1f, (uint8_t)s, (uint8_t)(e >> 2), (uint8_t)(e & 3)};
    }
    bool merged = true;
    while (merged) {
        merged = false;
        for (int i = 0; i < count && !merged; i++) {
            for (int j = i + 1; j < count && !merged; j++) {
                RuleCube& a = cube[i];
                RuleCube& b = cube[j];
                if (a.action != b.action || a.chance != b.chance || a.care != b.care) {
                    continue;
                }
                uint8_t diff = (uint8_t)((a.value ^ b.value) & a.care);
                if (__builtin_popcount(diff) != 1 || diff == 1) {
                    continue;
                }
                a.care = (uint8_t)(a.care & ~diff);
                a.value = (uint8_t)(a.value & ~diff);
                cube[j] = cube[--count];
                merged = true;
            }
        }
    }
    rule.cubes[g] = count;
    rule.needs_top[g] = 0;
    for (int i = 0; i < count; i++) {
        rule.needs_top[g] |= (cube[i].care >> 4) & 1;
    }
}

bool rule_parse(const char* text, RuleTable& rule, char* why, int why_len) {
    rule = RuleTable{};
    int digits[RULE_HEX];
    int n = 0;
    for (const char* p = text; *p; p++) {
        if (*p == '#') {
            while (*p && *p != '\n') {
                p++;
            }
            if (!*p) {
                break;
            }
            continue;
        }
        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            continue;
        }
        int v = hex_value(*p);
        if (v < 0) {
            std::snprintf(why, (size_t)why_len, "'%c' is not a hex digit", *p);
            return false;
        }
        if (n == RULE_HEX) {
            std::snprintf(why, (size_t)why_len, "more than %d hex digits", RULE_HEX);
            return false;
        }
        digits[n++] = v;
    }
    if (n != RULE_HEX) {
        std::snprintf(why, (size_t)why_len, "%d hex digits, want %d", n, RULE_HEX);
        return false;
    }
    for (int k = 0; k < 4; k++) {
        rule.shift[k] = (uint8_t)digits[k];
    }
    rule.window = digits[4] == 0 ? 15 : digits[4];
    rule.readoff = digits[5] > readoff_direct ? readoff_direct : digits[5];
    rule.balance = digits[6] & 1;
    for (int i = 0; i < RULE_ENTRIES; i++) {
        rule.entry[i] = (uint8_t)digits[RULE_HEADER + i];
        if ((rule.entry[i] >> 2) == rule_nothing) {
            continue;
        }
        if ((i >> 8) & 1) {
            rule.any_reward = 1;
        } else {
            rule.any_spike = 1;
        }
    }
    for (int g = 0; g < RULE_GROUPS; g++) {
        compile_group(rule, g);
    }
    rule.loaded = 1;
    return true;
}

static char* read_file(const char* path) {
    FILE* f = std::fopen(path, "rb");
    if (!f) {
        return nullptr;
    }
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    char* text = (char*)std::malloc((size_t)size + 1);
    size_t got = std::fread(text, 1, (size_t)size, f);
    text[got] = '\0';
    std::fclose(f);
    return text;
}

void rule_table_args(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; i++) {
        bool file = std::strcmp(argv[i], "--rule-file") == 0;
        bool hex = std::strcmp(argv[i], "--rule-hex") == 0;
        if (!file && !hex) {
            continue;
        }
        char* text = file ? read_file(argv[i + 1]) : nullptr;
        if (file && !text) {
            std::fprintf(stderr, "rule table: cannot read %s\n", argv[i + 1]);
            std::exit(1);
        }
        char why[128];
        bool ok = rule_parse(file ? text : argv[i + 1], loaded_rule, why, (int)sizeof why);
        std::free(text);
        if (!ok) {
            std::fprintf(stderr, "rule table %s: %s\n", argv[i + 1], why);
            std::exit(1);
        }
    }
}
