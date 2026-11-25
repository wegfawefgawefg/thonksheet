// blobmnist online: the sheet's own readout learning from good and bad, one digit at a time,
// tested with learning frozen at checkpoints (digits seen, or seconds of training).
//   --checks 1000,5000,20000,60000    digits seen at which to test (the last ends the run)
//   --check-seconds 60                also test once training time passes these seconds
//   --test-count N                    test digits per check (default all 10,000)
//   --pretrain N                      N digits of reward-free wiring by co-activity first
//   --shift 1                         at the end, the test digits moved 1 and 2 pixels
//   --val 1                           train on 0-49,999, score on the validation slice
//   --gap N                           quiet ticks after every digit (default 0)
//   --life N                          N digits with no reward under the genome's own rules
//                                     first (how the output gap job measures the lived
//                                     sheet); --life-freeze 1 then turns competitive
//                                     learning off, so the readout learns on a frozen inside
//   --pair 1|2 --pair-mix 0|1         pairs of digits, two buttons (blobmnist ceiling)
// the rest are blobrun's flags (--seed, --ticks, --motor-spread, --g.name value, ...).

#include "mnist/mnist_run.hpp"
#include "world/mnist_data.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

static const char* const online_own[] = {
    "--checks", "--check-seconds", "--test-count", "--pretrain",   "--shift", "--val",
    "--gap",    "--diag",          "--life",       "--life-freeze", "--pair",  "--pair-mix"};

static std::vector<int64_t> number_list(const char* text) {
    std::vector<int64_t> out;
    while (text && *text) {
        char* end = nullptr;
        long long v = std::strtoll(text, &end, 10);
        if (end == text) {
            break;
        }
        out.push_back(v);
        text = *end == ',' ? end + 1 : end;
    }
    return out;
}

// per button: its cells' mean spikes per digit on its own digits and on the others, mean
// connections, mean threshold, and the share of its connections that come from sensor cells
static void diagnose(Sheet& s) {
    Rows counts;
    sheet_eval(s, 0, 0, 1000, &counts);
    const Blob& b = *s.blob;
    std::printf("%6s %6s %8s %8s %8s %8s %8s\n", "button", "cells", "own", "others", "fan in",
                "thresh", "sensor");
    for (int line = 0; line < s.tt->plug.motor_lines; line++) {
        const int* cells = nullptr;
        int n = plug_motor_cells(s.tt->plug, line, &cells);
        double own = 0, other = 0, fan = 0, th = 0, sens = 0;
        int own_n = 0, other_n = 0;
        for (int r = 0; r < counts.n; r++) {
            double sum = 0;
            for (int k = 0; k < n; k++) {
                sum += counts.v[(size_t)r * counts.d + cells[k]];
            }
            (counts.label[(size_t)r] == line ? own : other) += sum / n;
            (counts.label[(size_t)r] == line ? own_n : other_n)++;
        }
        for (int k = 0; k < n; k++) {
            int c = cells[k];
            fan += b.fan_in[c];
            th += b.threshold[c];
            int wired = 0, from_sensor = 0;
            for (int slot = 0; slot < SLOTS; slot++) {
                int64_t w = blob_weight_index(c, slot);
                uint64_t conn = b.w0[w] | b.w1[w];
                uint32_t src = slot < LOCAL_SLOTS
                                   ? b.local_tile[(c / TILE_NEURONS) * LOCAL_SLOTS + slot]
                                   : b.far_tile[c * FAR_SLOTS + slot - LOCAL_SLOTS];
                wired += __builtin_popcountll(conn);
                from_sensor += __builtin_popcountll(conn & b.sensor[src]);
            }
            sens += wired ? (double)from_sensor / wired : 0;
        }
        // the first cell's hidden counters: how many above, at and below zero, and the extremes
        int above = 0, zero = 0, below = 0, top = -128, low = 127;
        int row = b.readout_row ? b.readout_row[cells[0]] : -1;
        for (int j = 0; row >= 0 && j < SLOTS * TILE_NEURONS; j++) {
            int v = b.readout_evidence[(size_t)row * SLOTS * TILE_NEURONS + j];
            above += v > 0;
            zero += v == 0;
            below += v < 0;
            top = v > top ? v : top;
            low = v < low ? v : low;
        }
        std::printf("%6d %6d %8.2f %8.2f %8.1f %8.1f %8.2f   counters +%d 0:%d -%d [%d, %d]\n",
                    line, n, own_n ? own / own_n : 0, other_n ? other / other_n : 0, fan / n,
                    th / n, sens / n, above, zero, below, low, top);
    }
}

// the share right on the moved test digits, averaged over the four directions
static double moved_score(Sheet& s, int by, int count) {
    double sum = 0;
    for (int dir = 0; dir < SHIFT_DIRECTIONS; dir++) {
        int dx = 0, dy = 0;
        shift_direction(dir, by, dx, dy);
        sum += sheet_eval(s, dx, dy, count, nullptr);
    }
    return sum / SHIFT_DIRECTIONS;
}

int mnist_online(int argc, char** argv) {
    Sheet s;
    if (!sheet_start(s, argc, argv, online_own, (int)(sizeof online_own / sizeof *online_own))) {
        return 1;
    }
    std::vector<int64_t> checks =
        number_list(text_flag(argc, argv, "--checks", "1000,5000,20000,60000"));
    std::vector<int64_t> check_secs = number_list(text_flag(argc, argv, "--check-seconds", ""));
    int test_count = int_flag(argc, argv, "--test-count", 10000);
    int64_t pretrain = int_flag(argc, argv, "--pretrain", 0);
    bool shift = int_flag(argc, argv, "--shift", 0) != 0;
    const char* scored = s.eval_test ? "test" : "validation";
    int64_t last = checks.empty() ? 0 : checks.back();

    std::printf("online readout  seed %llu  ticks/digit %d  gap %d  training digits %d  "
                "scored on %s (%d)\n",
                (unsigned long long)s.flags.seed, s.flags.plug.ticks_per_step, s.gap, s.train_count,
                scored, test_count);
    double t0 = now_seconds();
    if (pretrain > 0) {
        sheet_pretrain(s, pretrain);
        std::printf("pretrain: %lld digits of reward-free wiring, %.1f s\n", (long long)pretrain,
                    now_seconds() - t0);
    }
    int64_t life = int_flag(argc, argv, "--life", 0);
    if (life > 0) {
        double a = now_seconds();
        sheet_train(s, life, false, nullptr);
        bool freeze = int_flag(argc, argv, "--life-freeze", 0) != 0;
        if (freeze) {
            s.blob->genome.compete_on = 0;
        }
        std::printf("life: %lld digits with no reward, %.1f s%s\n", (long long)life,
                    now_seconds() - a, freeze ? ", then competition off" : "");
    }
    std::printf("%10s %10s %10s %10s %10s\n", "digits", "train s", scored, "stream", "ticks/s");
    std::fflush(stdout);
    double train_secs = 0;
    int64_t seen = 0, stream_right = 0, stream_n = 0;
    size_t next_check = 0, next_sec = 0;
    while (seen < last || (next_sec < check_secs.size())) {
        // train in chunks of 100 digits, or up to the next digit checkpoint
        int64_t chunk = 100;
        if (next_check < checks.size() && checks[next_check] - seen < chunk) {
            chunk = checks[next_check] - seen;
        }
        bool at_check = false;
        if (chunk > 0) {
            double a = now_seconds();
            stream_right += sheet_train(s, chunk, true, nullptr);
            stream_n += chunk;
            seen += chunk;
            train_secs += now_seconds() - a;
        }
        if (next_check < checks.size() && seen >= checks[next_check]) {
            next_check++;
            at_check = true;
        }
        if (next_sec < check_secs.size() && train_secs >= (double)check_secs[next_sec]) {
            next_sec++;
            at_check = true;
        }
        if (!at_check) {
            if (seen >= last && next_sec >= check_secs.size()) {
                break;
            }
            continue;
        }
        int64_t pressed[MNIST_PRESS_KINDS];
        for (int k = 0; k < MNIST_PRESS_KINDS; k++) {
            pressed[k] = s.presses[k];
            s.presses[k] = 0;
        }
        int64_t ticks0 = s.ticks;
        double a = now_seconds();
        double acc = sheet_eval(s, 0, 0, test_count, nullptr);
        double rate = (double)(s.ticks - ticks0) / (now_seconds() - a);
        std::printf("%10lld %10.1f %10.4f %10.4f %10.0f   presses", (long long)seen, train_secs,
                    acc, stream_n ? (double)stream_right / stream_n : 0.0, rate);
        for (int k = 0; k < MNIST_PRESS_KINDS; k++) {
            std::printf(" %lld", (long long)pressed[k]);
        }
        std::printf(" | test");
        for (int k = 0; k < MNIST_PRESS_KINDS; k++) {
            std::printf(" %lld", (long long)s.presses[k]);
            s.presses[k] = 0;
        }
        std::printf("\n");
        std::fflush(stdout);
        stream_right = stream_n = 0;
        if (seen >= last && next_sec >= check_secs.size()) {
            break;
        }
    }
    if (int_flag(argc, argv, "--diag", 0)) {
        diagnose(s);
    }
    if (shift) {
        double m1 = moved_score(s, 1, test_count), m2 = moved_score(s, 2, test_count);
        std::printf("moved: 1 pixel %.4f  2 pixels %.4f  (mean of right, left, down, up)\n", m1,
                    m2);
    }
    if (s.blob->margin_goods > 0) {
        std::printf("close goods: %u of %u (%.3f)\n", s.blob->margin_thin, s.blob->margin_goods,
                    (double)s.blob->margin_thin / s.blob->margin_goods);
    }
    std::printf("total wall %.1f s\n", now_seconds() - t0);
    sheet_free(s);
    return 0;
}
