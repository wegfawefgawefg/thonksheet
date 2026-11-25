// the Echo family on the GPU: the task (src/world/task_reflex.cpp, task_feature.cpp), the
// plug's lines and choice (src/world/plug.cpp) and the ticker (src/run/task_ticker.cpp).
// run by one thread per life on its own copy of the life's state.

// the task's dice: splitmix64 (src/world/world_random.hpp)
int world_below(inout Life L, int n) {
    L.world += GOLDEN;
    uint64_t z = mix64(L.world);
    return int(((z >> 32) * uint64_t(n)) >> 32);
}

// lines the task shows on this step, a bit per plug line (task_sense + plug_set_lines)
uint task_lines(Life L) {
    int first = P.first_single + P.cue_offset;
    if (P.task == TASK_ECHO || P.task == TASK_SHUFFLED_ECHO) {
        return 1u << (first + L.cue);
    }
    if (P.task == TASK_DELAYED_ECHO) {
        if (L.phase == 0) {
            return 1u << (first + L.cue);
        }
        return L.phase == P.delay + 1 ? 1u << (first + P.k) : 0u;
    }
    // both-not-either: a blank step, then A, B and ask together
    return L.phase == 1 ? uint(L.trial) << P.first_single : 0u;
}

// one answer to a cue: the matching button good, another bad, no move a miss
int judge_answer(inout Life L, int move) {
    L.chances++;
    if (move < 0) {
        return 0;
    }
    if (move == L.button_of[L.cue]) {
        L.right++;
        return 1;
    }
    return -1;
}

// apply the move, advance one step, return +1, -1 or 0 (task_step)
int task_step(inout Life L, int move) {
    int valence = 0;
    if (P.task == TASK_ECHO || P.task == TASK_SHUFFLED_ECHO) {
        valence = judge_answer(L, move);
        L.cue = world_below(L, P.k);
    } else if (P.task == TASK_DELAYED_ECHO) {
        if (L.phase == P.delay + 1) {
            valence = judge_answer(L, move);
            L.cue = world_below(L, P.k);
            L.phase = 0;
        } else {
            L.phase++;
        }
    } else if (L.phase < 1) {
        L.phase++; // both-not-either: presses before the ask are ignored
    } else {
        bool right = move == L.cue;
        valence = move < 0 ? 0 : (right ? 1 : -1);
        L.chances++;
        L.right += right ? 1 : 0;
        int a = world_below(L, 2);
        int b = world_below(L, 2);
        L.trial = a | (b << 1) | 4;
        L.cue = a ^ b;
        L.phase = 0;
    }
    return valence;
}

// the motor line with the most spikes this step, or -1 on a tie or none (plug_choice)
int plug_choice(int counts[MAX_MOTORS]) {
    int best = -1, best_count = 0;
    bool tie = false;
    for (int line = 0; line < P.motor_lines; line++) {
        int c = counts[line];
        if (c > best_count) {
            best = line;
            best_count = c;
            tie = false;
        } else if (c == best_count && c > 0) {
            tie = true;
        }
    }
    return tie ? -1 : best;
}
