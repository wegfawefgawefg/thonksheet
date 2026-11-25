#include "world/task_long.hpp"

#include "world/task.hpp"

// long-horizon worlds (task_long.hpp, docs/curriculum.md "Long memory"). every step is
// built by prepare() before it is sensed: which lines are lit, the right button, whether the
// press is graded, and which bucket of the forgetting table it counts in.

// buckets. pairs: 0..4 the five ages, 5 teaching. once: 0..4 the ages, 6 the showing step,
// 7 hint-only steps. rare: 0 common, 1 rare. drift: 0 kept pairs, 1 changed pairs. hold: 0.
constexpr int BUCKET_TEACH = 5, BUCKET_SHOW = 6, BUCKET_HINT = 7;

bool long_kind(const Task& task) {
    return task.kind == TaskKind::hold || task.kind == TaskKind::pairs ||
           task.kind == TaskKind::once || task.kind == TaskKind::rare ||
           task.kind == TaskKind::drift;
}

static int clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static uint64_t mix(uint64_t z) {
    z += 0x9e3779b97f4a7c15ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

static uint64_t choose(int n, int k) {
    if (k < 0 || k > n) {
        return 0;
    }
    uint64_t c = 1;
    for (int i = 1; i <= k; i++) {
        c = c * (uint64_t)(n - k + i) / (uint64_t)i;
    }
    return c;
}

static uint64_t gcd(uint64_t a, uint64_t b) {
    while (b) {
        uint64_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}

int64_t long_age(const LongSettings& s, int j) {
    int64_t age = s.age_base;
    for (int i = 0; i < j; i++) {
        age *= 10;
    }
    return age;
}

const char* long_bucket_name(const Task& task, int j) {
    static const char* const ages[LONG_AGES] = {"age0", "age1", "age2", "age3", "age4"};
    switch (task.kind) {
    case TaskKind::hold:
        return j == 0 ? "hold" : "";
    case TaskKind::pairs:
        return j < LONG_AGES ? ages[j] : (j == BUCKET_TEACH ? "teach" : "");
    case TaskKind::once:
        return j < LONG_AGES ? ages[j]
                             : (j == BUCKET_SHOW   ? "show"
                                : j == BUCKET_HINT ? "hint"
                                                   : "");
    case TaskKind::rare:
        return j == 0 ? "common" : (j == 1 ? "rare" : "");
    case TaskKind::drift:
        return j == 0 ? "kept" : (j == 1 ? "changed" : "");
    default:
        return "";
    }
}

// pattern r of the universe as lit lines (the r-th `on`-subset of `lines`, combinadic order)
static uint32_t unrank(uint64_t r, int lines, int on) {
    uint32_t bits = 0;
    int k = on;
    for (int i = lines - 1; i >= 0 && k > 0; i--) {
        uint64_t c = choose(i, k);
        if (r >= c) {
            bits |= 1u << i;
            r -= c;
            k--;
        }
    }
    return bits;
}

// pair i's pattern number: a fixed shuffle of the universe, so no two pairs share a pattern
// until the universe is used up
static uint64_t pair_pattern(const LongState& st, int64_t i) {
    uint64_t u = st.universe;
    return ((uint64_t)i % u * (st.stride % u) + st.offset) % u;
}

static int pattern_button(const Task& task, uint64_t pattern) {
    return (int)(mix(pattern ^ task.longs.salt) % (uint64_t)task.settings.longs.buttons);
}

// show pair i, graded, counted in `bucket`
static void show_pair(Task& task, int64_t i, int bucket, bool rewarded) {
    LongState& st = task.longs;
    uint64_t p = pair_pattern(st, i);
    st.lines_now = unrank(p, task.settings.longs.lines, task.settings.longs.on);
    st.answer = pattern_button(task, p);
    st.bucket = bucket;
    st.rewarded = rewarded;
}

// pairs and once: the age bucket whose probe falls on step s, and the pair it asks; -1 if none.
// pair i is asked for age j at step i * every + age_j + j + 1, so no two land together when
// the ages are multiples of `every`
static int probe_at(const Task& task, int64_t s, int64_t& pair) {
    const LongSettings& ls = task.settings.longs;
    for (int j = 0; j < LONG_AGES; j++) {
        int64_t d = s - long_age(ls, j) - (j + 1);
        if (d < 0 || d % ls.pair_every != 0) {
            continue;
        }
        pair = d / ls.pair_every;
        // once: each pair is asked at one age only, so its single showing is all it had
        if (task.kind == TaskKind::once && pair % LONG_AGES != j) {
            continue;
        }
        return j;
    }
    return -1;
}

static void prepare_hold(Task& task) {
    LongState& st = task.longs;
    const LongSettings& ls = task.settings.longs;
    WorldRandom& r = task.random;
    int k = ls.hold_cues;
    int ask = k, extra = k + 1; // mode 1: distractor lines from extra; mode 2: the mark line
    st.lines_now = 0;
    st.answer = -1;
    st.bucket = -1;
    st.rewarded = false;
    if (st.trial_step >= st.trial_len) {
        st.trial_step = 0;
        st.trial_len = ls.hold_steps + 3;
    }
    int64_t t = st.trial_step;
    if (t == 0) {
        task.cue = world_random_below(r, k); // step 0 is the blank that parts trials
    } else if (t == 1) {
        st.lines_now = (1u << task.cue) | (ls.hold_noise == 2 ? 1u << extra : 0);
    } else if (t < st.trial_len - 1) {
        // the gap: a distractor now and then
        if (ls.hold_noise > 0 && world_random_below(r, 1 << ls.noise_shift) == 0) {
            int d = world_random_below(r, k);
            st.lines_now = ls.hold_noise == 1 ? 1u << (extra + d) : 1u << d;
        }
    } else {
        st.lines_now = 1u << ask;
        st.answer = task.cue;
        st.bucket = 0;
        st.rewarded = true;
    }
}

static void prepare_pattern(Task& task) {
    LongState& st = task.longs;
    const LongSettings& ls = task.settings.longs;
    WorldRandom& r = task.random;
    int64_t s = task.steps;
    int64_t pair = 0;
    switch (task.kind) {
    case TaskKind::pairs: {
        int j = probe_at(task, s, pair);
        if (s % ls.pair_every != 0 && j >= 0) {
            show_pair(task, pair, j, ls.probe_reward != 0);
            break;
        }
        // teach one of the newest pair_live pairs (the newest on its first step)
        int64_t newest = s / ls.pair_every;
        int64_t back = s % ls.pair_every == 0 ? 0 : world_random_below(r, ls.pair_live);
        show_pair(task, newest - back < 0 ? 0 : newest - back, BUCKET_TEACH, true);
        break;
    }
    case TaskKind::once: {
        int j = probe_at(task, s, pair);
        if (s % ls.pair_every == 0) {
            // the one showing: the pattern with its answer on the hint lines
            show_pair(task, s / ls.pair_every, BUCKET_SHOW, true);
            st.lines_now |= 1u << (ls.lines + st.answer);
        } else if (j >= 0) {
            show_pair(task, pair, j, true);
        } else {
            // hint alone keeps "hint line b means button b" learned
            st.answer = world_random_below(r, ls.buttons);
            st.lines_now = 1u << (ls.lines + st.answer);
            st.bucket = BUCKET_HINT;
            st.rewarded = true;
        }
        break;
    }
    case TaskKind::rare: {
        bool rare = world_random_below(r, ls.rare_every) == 0;
        int64_t i = rare ? ls.rare_common + world_random_below(r, ls.rare_count)
                         : world_random_below(r, ls.rare_common);
        show_pair(task, i, rare ? 1 : 0, true);
        break;
    }
    case TaskKind::drift: {
        int64_t i = world_random_below(r, ls.drift_pairs);
        show_pair(task, i, (int)(i & 1), true);
        if ((i & 1) && s >= ls.drift_at) {
            st.answer = (st.answer + 1) % ls.buttons;
        }
        break;
    }
    default:
        break;
    }
    task.cue = st.answer < 0 ? 0 : st.answer;
}

static void prepare(Task& task) {
    if (task.kind == TaskKind::hold) {
        prepare_hold(task);
    } else {
        prepare_pattern(task);
    }
}

void long_reset(Task& task) {
    LongSettings& ls = task.settings.longs;
    LongState& st = task.longs;
    ls.hold_cues = clamp(ls.hold_cues, 2, 8);
    ls.hold_steps = ls.hold_steps < 0 ? 0 : ls.hold_steps;
    ls.noise_shift = clamp(ls.noise_shift, 0, 16);
    ls.buttons = clamp(ls.buttons, 2, 8);
    // once also needs a hint line per button
    int room = TASK_MAX_SINGLES - (task.kind == TaskKind::once ? ls.buttons : 0);
    ls.lines = clamp(ls.lines, 2, room);
    ls.on = clamp(ls.on, 1, ls.lines - 1);
    ls.pair_every = ls.pair_every < LONG_AGES + 2 ? LONG_AGES + 2 : ls.pair_every;
    ls.pair_live = ls.pair_live < 1 ? 1 : ls.pair_live;
    ls.age_base = ls.age_base < 1 ? 1 : ls.age_base;
    ls.rare_common = ls.rare_common < 1 ? 1 : ls.rare_common;
    ls.rare_count = ls.rare_count < 1 ? 1 : ls.rare_count;
    ls.rare_every = ls.rare_every < 2 ? 2 : ls.rare_every;
    ls.drift_pairs = ls.drift_pairs < 2 ? 2 : ls.drift_pairs;

    st = LongState{};
    st.universe = choose(ls.lines, ls.on);
    st.salt = world_random_word(task.random);
    st.offset = world_random_word(task.random) % st.universe;
    // a stride with no factor in common with the universe makes the shuffle a permutation
    st.stride = (world_random_word(task.random) % st.universe) | 1;
    while (gcd(st.stride, st.universe) != 1) {
        st.stride += 2;
    }
    prepare(task);
}

TaskShape long_shape(const Task& task) {
    const LongSettings& ls = task.settings.longs;
    TaskShape shape;
    switch (task.kind) {
    case TaskKind::hold:
        // cues, ask, then distractor lines (mode 1) or the mark line (mode 2)
        shape.singles = ls.hold_cues + 1 + (ls.hold_noise == 1 ? ls.hold_cues : 0) +
                        (ls.hold_noise == 2 ? 1 : 0);
        shape.motors = ls.hold_cues;
        break;
    case TaskKind::once:
        shape.singles = ls.lines + ls.buttons;
        shape.motors = ls.buttons;
        break;
    default:
        shape.singles = ls.lines;
        shape.motors = ls.buttons;
        break;
    }
    return shape;
}

void long_sense(const Task& task, TaskSense& sense) {
    uint32_t lines = task.longs.lines_now;
    for (int i = 0; i < TASK_MAX_SINGLES; i++) {
        sense.singles[i] = (lines >> i) & 1;
    }
}

int long_perfect_move(const Task& task) {
    return task.longs.answer;
}

int long_step(Task& task, const TaskStepIn& in) {
    LongState& st = task.longs;
    int valence = 0;
    if (st.answer >= 0) {
        bool right = in.move == st.answer;
        if (st.rewarded) {
            valence = in.move < 0 ? 0 : (right ? 1 : -1);
        }
        if (st.bucket >= 0) {
            st.asked[st.bucket]++;
            st.right[st.bucket] += right;
            st.wasked[st.bucket]++;
            st.wright[st.bucket] += right;
        }
        // the headline window score: holds and probes for hold, pairs and once; the rare
        // patterns for rare; every pair for drift
        bool headline = task.kind == TaskKind::rare    ? st.bucket == 1
                        : task.kind == TaskKind::drift ? true
                                                       : st.bucket >= 0 && st.bucket < LONG_AGES;
        if (headline) {
            task.tally[1]++;
            task.tally[0] += right;
        }
    }
    // the next step
    st.trial_step++;
    task.steps++;
    prepare(task);
    task.steps--; // task_step counts the step itself
    return valence;
}

void long_window_clear(Task& task) {
    for (int j = 0; j < LONG_BUCKETS; j++) {
        task.longs.wright[j] = 0;
        task.longs.wasked[j] = 0;
    }
}
