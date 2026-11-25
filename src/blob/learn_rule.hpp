#pragma once

#include "blob/blob.hpp"

#include <cstdint>

// idea 120: the learning rule as a table (learn_rule.cpp, learn_rule_table.cpp). at a moment,
// for each (neuron, source) pair, ten yes-or-no facts pick one entry of a table, and the entry
// says how that pair's evidence counter moves: up, down, toward zero or not at all, with one
// of four chances. the whole rule is a string of hex digits; docs/results-2026-11-25-rule-
// tables.md has the format.
//
// the facts, as bits of the entry index:
//   0 the source fired (reward: once per tick it fired in the window; own spike: recently)
//   1 the source fired exactly one slot delay before the neuron's last spike (causal)
//   2 the source is inhibitory (far slots count as excitatory)
//   3 the source is connected
//   4 the source's counter is above zero (direct read-off: the connection is strong)
//   5 the neuron fired recently (reward: in the window; own spike: in the ticks before it)
//   6 the neuron is a button's cell (the plug's mark)
//   7 a button's cell: of the button just pressed. any other neuron: its last spike was noise
//   8 the moment: a reward just arrived (1) or the neuron just fired (0)
//   9 bad: the reward was bad (reward moment) or the last reward was bad (own spike)
// sensor cells are never touched: the world drives them.

constexpr int RULE_FACTS = 10;
constexpr int RULE_ENTRIES = 1 << RULE_FACTS;
constexpr int RULE_SOURCE_FACTS = 5;
constexpr int RULE_GROUPS = RULE_ENTRIES >> RULE_SOURCE_FACTS; // moment and neuron facts
constexpr int RULE_HEADER = 8;                                 // hex digits before the entries
constexpr int RULE_HEX = RULE_HEADER + RULE_ENTRIES;

enum RuleAction : uint8_t { rule_nothing = 0, rule_up = 1, rule_down = 2, rule_decay = 3 };
enum RuleReadoff : uint8_t {
    readoff_evidence = 0, // counters, a cell wires the top of its budget at strength 4
    readoff_graded = 1,   // the same, strength by counter (1..3: 1, 4: 4, 5+: 5)
    readoff_direct = 2,   // no counters: the connection's level is the counter (0, 1, 4, 5)
};

// entries that share an action and chance merged over facts they do not depend on: a pair
// matches when (facts & care) == value. never merged over fact 0, which is counted per tick
struct RuleCube {
    uint8_t care, value, action, chance;
};

struct RuleTable {
    int loaded = 0;
    uint8_t shift[4] = {};            // the four chances, 2^-shift
    int window = 15;                  // reward moment: the ticks before it that count (1..15)
    int readoff = 0;                  // RuleReadoff
    int balance = 0;                  // 1: other buttons' cells move less, by their number
    uint8_t entry[RULE_ENTRIES] = {}; // action << 2 | chance
    // compiled: per group (index >> 5) the cubes over the five source facts
    int cubes[RULE_GROUPS] = {};
    RuleCube cube[RULE_GROUPS][32] = {};
    uint8_t needs_top[RULE_GROUPS] = {};
    int any_reward = 0, any_spike = 0;
};

// parse a hex string (whitespace and # comments skipped) into a compiled table
bool rule_parse(const char* text, RuleTable& rule, char* why, int why_len);
// "--rule-file path" or "--rule-hex digits" on the command line: load the table every blob
// with genome.rule_on uses. exits with a message if it does not parse
void rule_table_args(int argc, char** argv);
const RuleTable& rule_table();

// the table rule, once per tick: the reward moment (valence != 0) and own-spike moments.
// scope 0 every non-sensor neuron, 1 button cells only, 2 every neuron but button cells
void learn_rule(Blob& blob, int valence, int scope);

// learn_evidence.cpp: a neuron's counter row (made on first use; null if it has none), and
// the read-off at a given strength (0 all at 1, 1 all at 4, 2 by counter)
int8_t* evidence_row(Blob& blob, int n);
void evidence_rewire_as(Blob& blob, int n, int strength);
