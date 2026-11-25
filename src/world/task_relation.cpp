#include "world/task_script.hpp"

// relation (tier 9): the answer is about two or more items together, never one alone.
// items are w-of-n patterns (`item_lines` n, `item_on` w), so every item lights the same
// number of lines and total brightness says nothing. with `hold` > 0 that many items are
// drawn per life and never rewarded: a quarter of trials use only them, and only those are
// scored, so a learner has to carry the relation to items it was never taught on.
//
// same        slot A, slot B, ask. one step shows both with the ask. button 0 same, 1 not.
// same-then   one slot, ask. A for a step, then B, then the ask. button 0 same, 1 not.
// odd-one     k slots, ask. all slots show one item but one, which shows another. press
//             the odd slot's button.
// bigger      two rows of `item_lines` levels, one line lit in each (a magnitude), ask.
//             button 0 if the first is bigger, 1 if the second. never equal; with
//             `diff_max` the two are at most that far apart.
// analogy     two slots of `item_lines` lines, ask. A|B, then C|candidate for each of k
//             candidates, then the ask. B is one of k changes of A (shift right by one,
//             mirror, add the lowest unlit line); the candidates are those k changes of C in
//             a random order. press the candidate made by the change that made B.

constexpr int RELATION_TEST_IN = 4; // with held items, one trial in 4 uses only them

static int pick_item(Task& task, bool held) {
    for (int tries = 0; tries < 4096; tries++) {
        int i = world_random_below(task.random, task.item_count);
        if ((bool)((task.held_items >> i) & 1) == held) {
            return i;
        }
    }
    return 0;
}

static int pick_other(Task& task, int not_this, bool held) {
    for (int tries = 0; tries < 4096; tries++) {
        int i = pick_item(task, held);
        if (i != not_this) {
            return i;
        }
    }
    return not_this;
}

static uint32_t add_lowest(uint32_t p, int n) {
    for (int i = 0; i < n; i++) {
        if (!((p >> i) & 1)) {
            return p | (1u << i);
        }
    }
    return p;
}

// analogy's changes of a pattern on n lines: 0 shift right by one (around), 1 mirror,
// 2 add the lowest unlit line
static uint32_t change_of(int which, uint32_t p, int n) {
    uint32_t mask = (1u << n) - 1;
    switch (which) {
    case 0:
        return ((p << 1) | (p >> (n - 1))) & mask;
    case 1: {
        uint32_t out = 0;
        for (int i = 0; i < n; i++) {
            out |= ((p >> i) & 1) << (n - 1 - i);
        }
        return out;
    }
    default:
        return add_lowest(p, n);
    }
}

// every change gives something new and the changes all differ
static bool analogy_fits(uint32_t p, int n, int changes) {
    uint32_t seen[4];
    for (int c = 0; c < changes; c++) {
        seen[c] = change_of(c, p, n);
        if (seen[c] == p) {
            return false;
        }
        for (int d = 0; d < c; d++) {
            if (seen[d] == seen[c]) {
                return false;
            }
        }
    }
    return true;
}

void relation_reset(Task& task) {
    TaskSettings& s = task.settings;
    int n = s.item_lines, w = s.item_on;
    if (task.kind == TaskKind::analogy) {
        s.k = s.k > 3 ? 3 : s.k;
        w = 2;
    }
    if (task.kind == TaskKind::bigger) {
        s.diff_max = s.diff_max <= 0 || s.diff_max >= n ? n - 1 : s.diff_max;
        return;
    }
    task.item_count = all_patterns(n, w, task.item_pool, 64);
    if (task.kind == TaskKind::analogy) {
        // only items every change moves
        int kept = 0;
        for (int i = 0; i < task.item_count; i++) {
            if (analogy_fits((uint32_t)task.item_pool[i], n, s.k)) {
                task.item_pool[kept++] = task.item_pool[i];
            }
        }
        task.item_count = kept;
    }
    // held items: at least 2 so a "different" trial can be made of them alone, and at
    // least 2 left to teach on
    int hold = s.hold;
    hold = hold == 1 ? 2 : hold;
    hold = hold > task.item_count - 2 ? task.item_count - 2 : hold;
    s.hold = hold < 0 ? 0 : hold;
    int left = s.hold;
    while (left > 0) {
        int i = world_random_below(task.random, task.item_count);
        if (!((task.held_items >> i) & 1)) {
            task.held_items |= 1ull << i;
            left--;
        }
    }
}

TaskShape relation_shape(const Task& task) {
    TaskShape shape;
    int n = task.settings.item_lines, k = task.settings.k;
    shape.motors = 2;
    switch (task.kind) {
    case TaskKind::same:
    case TaskKind::bigger:
        shape.singles = 2 * n + 1;
        break;
    case TaskKind::same_then:
        shape.singles = n + 1;
        break;
    case TaskKind::odd_one:
        shape.singles = k * n + 1;
        shape.motors = k;
        break;
    case TaskKind::analogy:
        shape.singles = 2 * n + 1;
        shape.motors = k;
        break;
    default:
        break;
    }
    return shape;
}

void relation_trial(Task& task) {
    const TaskSettings& s = task.settings;
    WorldRandom& r = task.random;
    int n = s.item_lines, k = s.k;
    bool test = s.hold > 0 && world_random_below(r, RELATION_TEST_IN) == 0;
    // scored: held-out trials, or every trial when nothing is held out
    uint8_t flags = test ? (SCRIPT_SCORED | SCRIPT_PROBE)
                         : (SCRIPT_REWARDED | (s.hold == 0 ? SCRIPT_SCORED | SCRIPT_PROBE : 0));
    script_add(task, 0, -1, 0); // the blank that parts trials
    switch (task.kind) {
    case TaskKind::same:
    case TaskKind::same_then: {
        int a = pick_item(task, test);
        bool same = world_random_below(r, 2) == 0;
        int b = same ? a : pick_other(task, a, test);
        uint32_t pa = (uint32_t)task.item_pool[a], pb = (uint32_t)task.item_pool[b];
        if (task.kind == TaskKind::same) {
            script_add(task, pa | (pb << n) | line_bit(2 * n), same ? 0 : 1, flags);
        } else {
            script_add(task, pa, -1, 0);
            script_add(task, pb, -1, 0);
            script_add(task, line_bit(n), same ? 0 : 1, flags);
        }
        break;
    }
    case TaskKind::odd_one: {
        int common = pick_item(task, test);
        int odd = pick_other(task, common, test);
        int slot = world_random_below(r, k);
        uint32_t lines = line_bit(k * n);
        for (int i = 0; i < k; i++) {
            lines |= (uint32_t)task.item_pool[i == slot ? odd : common] << (i * n);
        }
        script_add(task, lines, slot, flags);
        break;
    }
    case TaskKind::bigger: {
        int a = 0, b = 0;
        do {
            a = world_random_below(r, n);
            b = world_random_below(r, n);
        } while (a == b || (a > b ? a - b : b - a) > s.diff_max);
        script_add(task, line_bit(a) | line_bit(n + b) | line_bit(2 * n), a > b ? 0 : 1,
                   SCRIPT_SCORED | SCRIPT_REWARDED | SCRIPT_PROBE);
        break;
    }
    case TaskKind::analogy: {
        uint32_t a = (uint32_t)task.item_pool[pick_item(task, false)];
        uint32_t c = (uint32_t)task.item_pool[pick_item(task, test)];
        int rule = world_random_below(r, k);
        int order[3] = {0, 1, 2};
        for (int i = k - 1; i > 0; i--) {
            int j = world_random_below(r, i + 1);
            int swap = order[i];
            order[i] = order[j];
            order[j] = swap;
        }
        script_add(task, a | (change_of(rule, a, n) << n), -1, 0);
        int want = 0;
        for (int i = 0; i < k; i++) {
            script_add(task, c | (change_of(order[i], c, n) << n), -1, 0);
            want = order[i] == rule ? i : want;
        }
        script_add(task, line_bit(2 * n), want, flags);
        break;
    }
    default:
        break;
    }
}
