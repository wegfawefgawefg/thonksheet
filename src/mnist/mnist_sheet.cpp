#include "mnist/mnist_run.hpp"
#include "world/mnist_data.hpp"

#include <cstdio>
#include <cstring>

// the mnist task on a real blob, a digit at a time, through the same ticker blobrun uses.
// one digit is one step of --ticks ticks; then --gap quiet ticks with every line off, in
// training and in tests alike. the reward lands after the step's last tick, as in blobrun.

void shift_direction(int dir, int by, int& dx, int& dy) {
    static const int step[SHIFT_DIRECTIONS][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    dx = step[dir][0] * by;
    dy = step[dir][1] * by;
}

bool sheet_start(Sheet& s, int argc, char** argv, const char* const* own, int own_count) {
    // own flags out, the rest to blobrun's parser (argv[1] is the command word)
    std::vector<char*> rest = {argv[0]};
    for (int i = 2; i < argc; i++) {
        bool mine = false;
        for (int k = 0; k < own_count; k++) {
            mine = mine || (std::strcmp(argv[i], own[k]) == 0 && i + 1 < argc);
        }
        if (mine) {
            i++;
            continue;
        }
        rest.push_back(argv[i]);
    }
    s.flags = RunFlags{};
    s.flags.plug.ticks_per_step = 20;
    s.flags.plug.sensor_side = 1;
    s.flags.plug.motor_side = 3;
    apply_genome_args((int)rest.size(), rest.data(), s.flags.genome);
    if (!run_flags_parse((int)rest.size(), rest.data(), s.flags)) {
        return false;
    }
    s.flags.task = TaskKind::mnist;
    // --pair 1|2 [--pair-mix 1]: two half-size digits, two buttons (mnist_data.hpp)
    s.flags.task_settings.mnist_pair = int_flag(argc, argv, "--pair", 0);
    s.flags.task_settings.mnist_mix = int_flag(argc, argv, "--pair-mix", 0);
    s.flags.plug.rest_ticks = 0; // the gap is ours, after every digit
    s.gap = int_flag(argc, argv, "--gap", 0);
    bool val = int_flag(argc, argv, "--val", 0) != 0;
    s.train_first = 0;
    s.train_count = val ? 50000 : 60000;
    s.eval_test = val ? 0 : 1;
    s.eval_first = val ? 50000 : 0;
    s.eval_count = 10000;
    s.flags.task_settings.mnist_first = s.train_first;
    s.flags.task_settings.mnist_count = s.train_count;
    if (!mnist_data()) {
        return false;
    }
    s.blob = blob_birth(s.flags.genome, s.flags.seed);
    static TaskTicker ticker;
    s.tt = &ticker;
    if (!ticker_start(*s.tt, *s.blob, s.flags)) {
        blob_free(s.blob);
        s.blob = nullptr;
        return false;
    }
    return true;
}

void sheet_free(Sheet& s) {
    if (s.blob) {
        blob_free(s.blob);
    }
    s.blob = nullptr;
}

// one digit: tick until the step is answered, then the gap. 1 if the answer was right
static int one_digit(Sheet& s, bool learn, Rows* counts, std::vector<uint8_t>& c) {
    TaskTicker& tt = *s.tt;
    Blob& b = *s.blob;
    if (counts) {
        std::memset(c.data(), 0, c.size());
    }
    if (s.images) {
        s.images->push_back(tt.task.image);
    }
    if (s.partners) {
        s.partners->push_back(tt.task.partner);
    }
    if (s.shuffle) {
        shuffle_draw(s);
    }
    if (s.reset_digit) {
        repeat_digit_start(s);
    }
    for (int tick = 0;; tick++) {
        ticker_before_tick(tt, b);
        if (s.shuffle) {
            shuffle_apply(s);
        }
        if (s.input_every > 1 && tick % s.input_every != 0) {
            repeat_input_off(s);
        }
        if (s.inh_fast) {
            repeat_inh_fast(s);
        }
        blob_tick(b);
        s.ticks++;
        for (int t = 0; counts && tick >= s.count_from && t < b.tile_count; t++) {
            uint64_t spikes = b.spikes[b.now][t];
            while (spikes) {
                uint8_t& n = c[(size_t)(t * TILE_NEURONS + __builtin_ctzll(spikes))];
                n = n < 255 ? n + 1 : n;
                spikes &= spikes - 1;
            }
        }
        if (ticker_after_tick(tt, b)) {
            break;
        }
    }
    s.presses[tt.move >= 0 && tt.move < 10 ? tt.move : 10]++;
    if (learn && tt.valence != 0) {
        plug_mark_acted(tt.plug, b, tt.move);
        plug_mark_vote(tt.plug, b, tt.move);
        plug_hop_lines(tt.plug, b, tt.move, tt.right_move, 0, 0, 0);
        int valence = tt.valence;
        if (s.coin_reward) {
            // the control: good or bad by a coin (splitmix64 step), whatever was pressed
            uint64_t z = (s.coin_state += 0x9e3779b97f4a7c15ull);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
            valence = ((z ^ (z >> 31)) & 1) ? 1 : -1;
        }
        blob_reward(b, valence);
    }
    if (counts) {
        rows_add(*counts, c.data(), tt.right_move);
    }
    if (counts && s.outside) {
        outside_digit(s, c.data(), tt.right_move);
    }
    tt.rest_left = 0;
    if (s.gap > 0) {
        plug_quiet_lines(tt.plug);
        for (int g = 0; g < s.gap; g++) {
            plug_write(tt.plug, b);
            if (s.inh_fast) {
                repeat_inh_fast(s);
            }
            blob_tick(b);
            s.ticks++;
        }
    }
    return tt.move >= 0 && tt.move == tt.right_move;
}

// --life-moved (mnist_moved.cpp): one digit shown for several steps, a pixel further each
// step along its own direction; before each step but the first the new frame grades the
// guess made after the step before
static int moved_digit(Sheet& s, bool learn, Rows* counts, std::vector<uint8_t>& c) {
    Task& task = s.tt->task;
    int64_t shown = task.shown;
    int image = task.image, cue = task.cue, partner = task.partner;
    int steps = moved_steps(s), right = 0;
    for (int j = 0; j < steps; j++) {
        task.shown = shown;
        task.image = image;
        task.cue = cue;
        task.partner = partner;
        shift_direction(image % SHIFT_DIRECTIONS, j - steps / 2, task.settings.mnist_dx,
                        task.settings.mnist_dy);
        moved_grade(s);
        right = one_digit(s, learn, counts, c);
        moved_keep(s, c.data(), j + 1 < steps);
    }
    task.settings.mnist_dx = task.settings.mnist_dy = 0;
    return right;
}

int64_t sheet_train(Sheet& s, int64_t digits, bool learn, Rows* counts) {
    std::vector<uint8_t> c(counts ? (size_t)s.blob->neuron_count : 0);
    if (counts) {
        counts->d = s.blob->neuron_count;
    }
    int64_t right = 0;
    for (int64_t i = 0; i < digits; i++) {
        bool moved = s.moved && counts;
        right += moved ? moved_digit(s, learn, counts, c) : one_digit(s, learn, counts, c);
    }
    return right;
}

void sheet_pretrain(Sheet& s, int64_t digits) {
    Genome& g = s.blob->genome;
    int mode = g.learn_mode, on = g.learning_on;
    g.learn_mode = 0;
    g.learning_on = 1;
    sheet_train(s, digits, false, nullptr);
    g.learn_mode = mode;
    g.learning_on = on;
}

double sheet_eval(Sheet& s, int dx, int dy, int count, Rows* counts) {
    TaskTicker& tt = *s.tt;
    Task saved = tt.task;
    int learning = s.blob->genome.learning_on;
    s.blob->genome.learning_on = 0;
    count = count <= 0 || count > s.eval_count ? s.eval_count : count;
    mnist_restart(tt.task, s.eval_test, s.eval_first, s.eval_count, dx, dy);
    std::vector<uint8_t> c(counts ? (size_t)s.blob->neuron_count : 0);
    if (counts) {
        counts->d = s.blob->neuron_count;
    }
    int64_t right = 0;
    for (int i = 0; i < count; i++) {
        right += one_digit(s, false, counts, c);
    }
    tt.task = saved;
    s.blob->genome.learning_on = learning;
    return (double)right / count;
}
