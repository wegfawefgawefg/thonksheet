#include "world/task_script.hpp"

// chance and context (tier 9). each is echo-shaped: one step per try, k cue lines, k
// buttons, scored on the true answer.
//
// noisy-echo   echo, but good and bad tell the truth only `right_pct` times in 100: a right
//              press is rewarded that often and punished otherwise, a wrong one the other
//              way round.
// rare-cue     echo with two cues; cue 1 comes one try in `rare`. scored balanced: the mean
//              of the hit rate on each cue, so always pressing button 0 scores 0.5.
// noisy-lines  k fixed patterns on `item_lines` lines, half lit (for two, one is the other
//              inverted), drawn per life; each line is flipped with chance `flip_pct` in 100
//              on every step. press the pattern's button.
// context      k cues and two context lines. under context 0 cue i means button i, under
//              context 1 button (i + 1) mod k. with `context_early` the context is shown
//              alone a step before the cue, and the try is those two steps.
// switch-rule  echo whose rule moves on every `switch_every` tries with no signal: cue i
//              means button (i + shift) mod k, and the shift goes up by one.

void chance_reset(Task& task) {
    TaskSettings& s = task.settings;
    s.right_pct = s.right_pct < 0 ? 0 : (s.right_pct > 100 ? 100 : s.right_pct);
    s.flip_pct = s.flip_pct < 0 ? 0 : (s.flip_pct > 50 ? 50 : s.flip_pct);
    if (task.kind == TaskKind::noisy_lines) {
        int n = s.item_lines;
        task.item_count = s.k;
        for (int c = 0; c < s.k; c++) {
            bool fresh = false;
            while (!fresh) {
                uint32_t p = random_pattern(task.random, n, n / 2);
                if (c == 1 && s.k == 2) {
                    p = ~(uint32_t)task.item_pool[0] & ((1u << n) - 1);
                }
                fresh = true;
                for (int d = 0; d < c; d++) {
                    fresh = fresh && task.item_pool[d] != (int)p;
                }
                task.item_pool[c] = (int)p;
            }
        }
    }
    task.memory[0] = 0; // switch: the shift
    task.memory[1] = 0; // switch: tries since it moved
}

TaskShape chance_shape(const Task& task) {
    TaskShape shape;
    int k = task.settings.k;
    shape.singles = k;
    shape.motors = k;
    if (task.kind == TaskKind::noisy_lines) {
        shape.singles = task.settings.item_lines;
    }
    if (task.kind == TaskKind::context) {
        shape.singles = k + 2;
    }
    return shape;
}

int chance_valence(Task& task, bool right, int move, int valence) {
    if (move < 0) {
        return 0;
    }
    bool truthful = world_random_below(task.random, 100) < task.settings.right_pct;
    return truthful ? valence : (right ? -1 : 1);
}

void chance_trial(Task& task) {
    const TaskSettings& s = task.settings;
    WorldRandom& r = task.random;
    int k = s.k;
    uint8_t flags = SCRIPT_SCORED | SCRIPT_REWARDED | SCRIPT_PROBE;
    int cue = world_random_below(r, k);
    switch (task.kind) {
    case TaskKind::noisy_echo:
        script_add(task, line_bit(cue), cue, flags);
        break;
    case TaskKind::rare_cue: {
        // the probe keeps every rare try and the common try after each, so its share right
        // is balanced too (memory[2]: common tries owed to it)
        cue = world_random_below(r, s.rare) == 0 ? 1 : 0;
        bool keep = cue == 1 || task.memory[2] > 0;
        task.memory[2] += cue == 1 ? 1 : (keep ? -1 : 0);
        script_add(task, line_bit(cue), cue, keep ? flags : (flags & ~SCRIPT_PROBE));
        break;
    }
    case TaskKind::noisy_lines: {
        uint32_t lines = (uint32_t)task.item_pool[cue];
        for (int i = 0; i < s.item_lines; i++) {
            if (world_random_below(r, 100) < s.flip_pct) {
                lines ^= line_bit(i);
            }
        }
        script_add(task, lines, cue, flags);
        break;
    }
    case TaskKind::context: {
        int ctx = world_random_below(r, 2);
        int want = ctx == 0 ? cue : (cue + 1) % k;
        if (s.context_early) {
            script_add(task, line_bit(k + ctx), -1, 0);
            script_add(task, line_bit(cue), want, flags);
        } else {
            script_add(task, line_bit(cue) | line_bit(k + ctx), want, flags);
        }
        break;
    }
    case TaskKind::switch_rule:
        if (task.memory[1] >= s.switch_every) {
            task.memory[0] = (task.memory[0] + 1) % k;
            task.memory[1] = 0;
        }
        task.memory[1]++;
        script_add(task, line_bit(cue), (cue + task.memory[0]) % k, flags);
        break;
    default:
        break;
    }
}
