#pragma once

#include "world/task.hpp"

// tiers 9 and 10 (docs/curriculum.md, "The rest of the map"). Every such task is a trial
// script: a list of steps, each with the single lines on, the sheet bits on, the button
// wanted (-1: presses ignored) and flags. task_script.cpp runs the script; one file per
// family draws the trials:
//   task_relation.cpp      same, same-then, odd-one, bigger, analogy
//   task_sheet_number.cpp  how-many, more-fewer, parts
//   task_grammar.cpp       grammar, brackets
//   task_chance.cpp        noisy-echo, rare-cue, noisy-lines, context, switch-rule
//   task_memory.cpp        copy, recall, n-back
//   task_unsupervised.cpp  predict-next, fill-in, odd-event (no reward)

// a fresh, empty trial
inline void script_clear(Task& task) {
    task.script = Task::Script{};
    task.phase = 0;
}

// one more step; returns its index
inline int script_add(Task& task, uint32_t lines, int want, uint8_t flags) {
    Task::Script& s = task.script;
    int at = s.len < SCRIPT_MAX ? s.len++ : SCRIPT_MAX - 1;
    s.lines[at] = lines;
    s.want[at] = (int8_t)want;
    s.flags[at] = flags;
    return at;
}

// light sheet bit i (row-major) at one step
inline void script_sheet_on(Task& task, int step, int i) {
    task.script.sheet[step][i / 64] |= 1ull << (i % 64);
}

inline uint32_t line_bit(int line) {
    return 1u << line;
}

// a random w-of-n pattern as line bits
uint32_t random_pattern(WorldRandom& r, int n, int w);
// every w-of-n pattern, at most `most`, into out; returns how many
int all_patterns(int n, int w, int* out, int most);

void relation_reset(Task& task);
void relation_trial(Task& task);
TaskShape relation_shape(const Task& task);

void sheet_number_reset(Task& task);
void sheet_number_trial(Task& task);
TaskShape sheet_number_shape(const Task& task);

void grammar_reset(Task& task);
void grammar_trial(Task& task);
TaskShape grammar_shape(const Task& task);

void chance_reset(Task& task);
void chance_trial(Task& task);
TaskShape chance_shape(const Task& task);
// noisy echo: the reward may lie
int chance_valence(Task& task, bool right, int move, int valence);

void memory_reset(Task& task);
void memory_trial(Task& task);
TaskShape memory_shape(const Task& task);

void unsupervised_reset(Task& task);
void unsupervised_trial(Task& task);
TaskShape unsupervised_shape(const Task& task);
