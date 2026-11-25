#include "world/task_script.hpp"

// no teacher at all (tier 10). no good or bad, no blank steps: a stream with structure. the
// sheet's own predictions of its sensor cells give the world's score; blobprobe linear reads
// the answer from activity on the steps marked for it (the wanted button is the label; the
// buttons are never rewarded). symbols are 2-of-4 patterns on 4 lines, each line lit by half
// the symbols, so "all off" or "same as before" predicts half the cells right.
//
// predict-next  k symbols (at most 6). the next symbol is a fixed function of the last one
//               (`markov_order` 1) or of the last two (2; then the last one alone never
//               fixes it) 7 times in 8, any other symbol otherwise. score: share of sensor
//               cells predicted right on the step's first tick. probe label: the next symbol.
// fill-in       k patterns on `item_lines` lines, half lit, drawn per life. one step in 4 the
//               second half of the lines is blanked. score: on blanked steps, how often a
//               hidden cell's own prediction is on when it should be, less how often when it
//               should not (0 for any blind guess, 1 for a perfect fill). probe label: which
//               pattern, on blanked steps.
// odd-event     k symbols in a fixed cycle (drawn per life); one step in `rare` shows another
//               symbol instead. score: wrong predictions per sensor cell on those steps less
//               on regular ones (0 when the break is not noticed). probe label: break (1) or
//               not (0), on each break and on the regular step one cycle after it.

static const int symbol_code[6] = {0x3, 0xc, 0x5, 0xa, 0x6, 0x9};

void unsupervised_reset(Task& task) {
    TaskSettings& s = task.settings;
    WorldRandom& r = task.random;
    int k = s.k;
    if (task.kind != TaskKind::fill_in) {
        s.k = k = k > 6 ? 6 : k;
    }
    for (int i = 0; i < 16; i++) {
        task.memory[i] = 0;
    }
    switch (task.kind) {
    case TaskKind::predict_next:
        s.markov_order = s.markov_order == 2 ? 2 : 1;
        if (s.markov_order == 1) {
            for (int c = 0; c < k; c++) {
                task.table_next[c] = (c + 1 + world_random_below(r, k - 1)) % k;
            }
        } else {
            // per last symbol, the one before must matter: at least two different nexts
            for (int c = 0; c < k; c++) {
                bool varies = false;
                while (!varies) {
                    for (int p = 0; p < k; p++) {
                        task.table_next[p * k + c] = world_random_below(r, k);
                    }
                    for (int p = 1; p < k; p++) {
                        varies = varies || task.table_next[p * k + c] != task.table_next[c];
                    }
                }
            }
        }
        task.memory[0] = world_random_below(r, k);
        task.memory[1] = world_random_below(r, k);
        break;
    case TaskKind::fill_in: {
        s.item_lines = s.item_lines > 16 ? 16 : s.item_lines;
        int n = s.item_lines;
        // distinct first halves when there are enough of them to go round
        bool distinct = n / 2 <= 5 && k <= (1 << (n / 2)) / 2;
        uint64_t seen_halves = 0;
        task.item_count = k;
        for (int c = 0; c < k; c++) {
            uint32_t p = 0, half = 0;
            do {
                p = random_pattern(r, n, n / 2);
                half = p & ((1u << (n / 2)) - 1);
            } while (distinct && ((seen_halves >> half) & 1));
            seen_halves |= 1ull << (half & 63);
            task.item_pool[c] = (int)p;
        }
        break;
    }
    case TaskKind::odd_event:
        for (int i = 0; i < k; i++) {
            task.table_next[i] = i;
        }
        for (int i = k - 1; i > 0; i--) {
            int j = world_random_below(r, i + 1);
            int swap = task.table_next[i];
            task.table_next[i] = task.table_next[j];
            task.table_next[j] = swap;
        }
        task.memory[1] = -1; // no regular step owed to the probe
        break;
    default:
        break;
    }
}

TaskShape unsupervised_shape(const Task& task) {
    TaskShape shape;
    shape.uses_reward = false;
    shape.singles = task.kind == TaskKind::fill_in ? task.settings.item_lines : 4;
    shape.motors = task.kind == TaskKind::odd_event ? 2 : task.settings.k;
    return shape;
}

void unsupervised_trial(Task& task) {
    const TaskSettings& s = task.settings;
    WorldRandom& r = task.random;
    int k = s.k;
    int* m = task.memory;
    switch (task.kind) {
    case TaskKind::predict_next: {
        // m[0] this symbol, m[1] the one before
        int cur = m[0], prev = m[1];
        int likely = s.markov_order == 1 ? task.table_next[cur] : task.table_next[prev * k + cur];
        int next =
            world_random_below(r, 8) < 7 ? likely : (likely + 1 + world_random_below(r, k - 1)) % k;
        script_add(task, (uint32_t)symbol_code[cur], next, SCRIPT_PROBE);
        m[1] = cur;
        m[0] = next;
        break;
    }
    case TaskKind::fill_in: {
        int n = s.item_lines, c = world_random_below(r, k);
        uint32_t p = (uint32_t)task.item_pool[c];
        uint32_t low = (1u << (n / 2)) - 1, high = ((1u << n) - 1) & ~low;
        if (world_random_below(r, 4) == 0) {
            int at = script_add(task, p & low, c, SCRIPT_PROBE);
            task.script.hidden[at] = high;
            task.script.hidden_on[at] = p & high;
        } else {
            script_add(task, p, -1, 0);
        }
        break;
    }
    case TaskKind::odd_event: {
        // m[0] the place in the cycle, m[1] the place whose next regular step the probe keeps
        int at_cycle = m[0];
        int regular = task.table_next[at_cycle];
        bool odd = world_random_below(r, s.rare) == 0;
        if (odd) {
            int other = (regular + 1 + world_random_below(r, k - 1)) % k;
            script_add(task, (uint32_t)symbol_code[other], 1, SCRIPT_SCORED | SCRIPT_PROBE);
            m[1] = m[1] < 0 ? at_cycle : m[1];
        } else if (m[1] == at_cycle) {
            script_add(task, (uint32_t)symbol_code[regular], 0, SCRIPT_PROBE);
            m[1] = -1;
        } else {
            script_add(task, (uint32_t)symbol_code[regular], -1, 0);
        }
        m[0] = (at_cycle + 1) % k;
        break;
    }
    default:
        break;
    }
}
