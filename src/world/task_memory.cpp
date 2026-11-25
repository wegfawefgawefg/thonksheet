#include "world/task_script.hpp"

// longer memory (tier 4 and 9: holding things over steps, and output over several steps).
// a press is one button per step, as everywhere: output that is a sequence is a run of
// answer steps, each judged against its own item, each rewarded at once.
//
// copy    k symbol lines and a go line. a trial: a blank step, 2 to `length` symbols one a
//         step, then the go line for as many steps; on the i-th go step press the i-th
//         symbol's button. the go line looks the same on every go step, so the sheet has to
//         keep its own place in the sequence.
// recall  k symbol lines, a mark line and the ask. a blank step, the target with the mark,
//         `fill` steps each showing a symbol drawn at random (the target too, by chance),
//         then the ask: press the marked symbol's button.
// n-back  k symbol lines, a stream with no blank steps. each step press button 1 if this
//         symbol is the one `back` steps ago, else button 0. half the steps match; a
//         non-match is any other symbol, so this symbol alone tells nothing.

constexpr int BACK_KEPT = 8;

void memory_reset(Task& task) {
    TaskSettings& s = task.settings;
    int most = (SCRIPT_MAX - 1) / 2;
    s.length = s.length < 2 ? 2 : (s.length > most ? most : s.length);
    for (int i = 0; i < 16; i++) {
        task.memory[i] = 0;
    }
}

TaskShape memory_shape(const Task& task) {
    TaskShape shape;
    int k = task.settings.k;
    switch (task.kind) {
    case TaskKind::copy:
        shape.singles = k + 1;
        shape.motors = k;
        break;
    case TaskKind::recall:
        shape.singles = k + 2;
        shape.motors = k;
        break;
    case TaskKind::n_back:
        shape.singles = k;
        shape.motors = 2;
        break;
    default:
        break;
    }
    return shape;
}

void memory_trial(Task& task) {
    const TaskSettings& s = task.settings;
    WorldRandom& r = task.random;
    int k = s.k;
    uint8_t flags = SCRIPT_SCORED | SCRIPT_REWARDED | SCRIPT_PROBE;
    switch (task.kind) {
    case TaskKind::copy: {
        int len = 2 + world_random_below(r, s.length - 1);
        int symbols[SCRIPT_MAX];
        script_add(task, 0, -1, 0);
        for (int i = 0; i < len; i++) {
            symbols[i] = world_random_below(r, k);
            script_add(task, line_bit(symbols[i]), -1, 0);
        }
        for (int i = 0; i < len; i++) {
            script_add(task, line_bit(k), symbols[i], flags);
        }
        break;
    }
    case TaskKind::recall: {
        int target = world_random_below(r, k);
        script_add(task, 0, -1, 0);
        script_add(task, line_bit(target) | line_bit(k), -1, 0);
        for (int i = 0; i < s.fill; i++) {
            script_add(task, line_bit(world_random_below(r, k)), -1, 0);
        }
        script_add(task, line_bit(k + 1), target, flags);
        break;
    }
    case TaskKind::n_back: {
        // memory[i]: the symbol i + 1 steps ago; memory[BACK_KEPT]: steps seen
        int* m = task.memory;
        bool ready = m[BACK_KEPT] >= s.back;
        int symbol = world_random_below(r, k);
        int want = -1;
        if (ready) {
            int then = m[s.back - 1];
            bool match = world_random_below(r, 2) == 0;
            if (match) {
                symbol = then;
            } else {
                symbol = (then + 1 + world_random_below(r, k - 1)) % k;
            }
            want = match ? 1 : 0;
        }
        script_add(task, line_bit(symbol), want, ready ? flags : 0);
        for (int i = BACK_KEPT - 1; i > 0; i--) {
            m[i] = m[i - 1];
        }
        m[0] = symbol;
        m[BACK_KEPT] += m[BACK_KEPT] < BACK_KEPT;
        break;
    }
    default:
        break;
    }
}
