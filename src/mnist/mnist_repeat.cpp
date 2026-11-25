#include "blob/tick_steps.hpp"
#include "mnist/mnist_run.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

// why the sheet does not answer the same digit the same way twice (idea 168,
// docs/results-2026-11-26-repeatable.md, scripts/repeat.sh). probes of blobmnist ceiling,
// every one off unless its flag is given, none on the tick's path:
//   --twice 1         after the test digits, one more training digit, then the same test
//                     digits again; a line with each cell's correlation with itself between
//                     the two passes (mean over the non-sensor cells that spiked in both)
//   --rec-cut 1|2     before settling, the cells' wiring cut: 1 no excitation from other
//                     cells (pixels and inhibitory cells kept); 2 pixels only
//   --delays-one 1    before settling, every slot's delay 1 (local and far)
//   --pace-off 1      before settling, the pacemakers become ordinary cells
//   --read-freeze M   after settling: 1 thresholds stop moving, 2 the sector bias, 3 both
//   --reset-digit M   at each digit's start: 1 potentials and cooldowns cleared, 2 and every
//                     spike in the ring (a diagnostic of carry-over, not something a sheet does)
//   --input-every N   a pixel line reaches the sheet only on every Nth tick of a digit
//   --count-from K    a digit's counts start at its tick K (the reader sees the rest)
//   --inh-fast 1      inhibition with no delay: before every tick each older map of the spike
//                     ring takes the newest map's inhibitory spikes, so a slot of any delay
//                     reads its source's inhibition one tick old (excitation keeps its delay;
//                     learning is off while the sheet is read, and nothing else reads old maps)

static bool bit_on(const uint64_t* words, int n) {
    return (words[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
}

// rec-cut: what a non-sensor cell may still hear from each slot's source tile
static void cut_recurrence(Blob& b, int mode) {
    int64_t before = 0, after = 0;
    for (int n = 0; n < b.neuron_count; n++) {
        if (bit_on(b.sensor, n)) {
            continue;
        }
        for (int k = 0; k < SLOTS; k++) {
            uint32_t src = slot_source(b, n, k);
            if (src == (uint32_t)b.tile_count) {
                continue;
            }
            uint64_t keep = b.sensor[src] | (mode == 1 && k < LOCAL_SLOTS ? b.inhibitory[src] : 0);
            int64_t w = blob_weight_index(n, k);
            before += __builtin_popcountll(b.w0[w] | b.w1[w]);
            b.w0[w] &= keep;
            b.w1[w] &= keep;
            after += __builtin_popcountll(b.w0[w] | b.w1[w]);
        }
        b.fan_in[n] = (uint16_t)count_fan_in(b, n);
    }
    std::printf("repeat: rec-cut %d: connections of non-sensor cells %lld -> %lld\n", mode,
                (long long)before, (long long)after);
}

static void delays_one(Blob& b) {
    for (int n = 0; n < b.neuron_count; n++) {
        for (int k = 0; k < SLOTS; k++) {
            b.slot_delay[blob_weight_index(n, k)] = 1;
        }
    }
    // every local delay the same: the tick reads one word per slot for the whole tile
    b.genome.local_delay_min = b.genome.local_delay_max = 1;
    std::printf("repeat: every slot delay 1\n");
}

static void pacemakers_off(Blob& b) {
    const Genome& g = b.genome;
    int count = 0;
    for (int n = 0; n < b.neuron_count; n++) {
        if (!bit_on(b.pacemaker, n)) {
            continue;
        }
        b.pacemaker[n / TILE_NEURONS] &= ~(1ull << (n % TILE_NEURONS));
        b.threshold[n] = (int16_t)((g.threshold_min + g.threshold_max) / 2);
        b.cooldown_len[n] = (uint8_t)g.cooldown_min;
        b.cooldown[n] = 0;
        count++;
    }
    std::printf("repeat: %d pacemakers made ordinary cells\n", count);
}

void repeat_setup(Sheet& s, int argc, char** argv) {
    Blob& b = *s.blob;
    if (int cut = int_flag(argc, argv, "--rec-cut", 0)) {
        cut_recurrence(b, cut);
    }
    if (int_flag(argc, argv, "--delays-one", 0)) {
        delays_one(b);
    }
    if (int_flag(argc, argv, "--pace-off", 0)) {
        pacemakers_off(b);
    }
    s.reset_digit = int_flag(argc, argv, "--reset-digit", 0);
    s.input_every = int_flag(argc, argv, "--input-every", 0);
    s.count_from = int_flag(argc, argv, "--count-from", 0);
    s.inh_fast = int_flag(argc, argv, "--inh-fast", 0);
}

void repeat_after_settle(Sheet& s, int argc, char** argv) {
    int freeze = int_flag(argc, argv, "--read-freeze", 0);
    if (freeze & 1) {
        s.blob->genome.threshold_reg_on = 0;
    }
    if (freeze & 2) {
        s.blob->genome.sector_reg_on = 0;
    }
    if (freeze) {
        std::printf("repeat: after settling, %s frozen\n",
                    freeze == 3 ? "thresholds and sector bias" : (freeze == 1 ? "thresholds"
                                                                               : "sector bias"));
    }
}

void repeat_digit_start(Sheet& s) {
    Blob& b = *s.blob;
    size_t n = (size_t)b.neuron_count;
    std::memset(b.potential, 0, n * sizeof *b.potential);
    std::memset(b.cooldown, 0, n * sizeof *b.cooldown);
    if (s.reset_digit >= 2) {
        std::memset(b.spikes[0], 0, sizeof(uint64_t) * (size_t)b.spike_stride * HISTORY);
    }
}

void repeat_input_off(Sheet& s) {
    Blob& b = *s.blob;
    for (int t = 0; t < b.tile_count; t++) {
        b.sense_in[t] &= ~b.sensor[t];
    }
}

void repeat_inh_fast(Sheet& s) {
    Blob& b = *s.blob;
    const uint64_t* now = b.spikes[b.now];
    for (int h = 0; h < HISTORY; h++) {
        if (h == b.now) {
            continue;
        }
        uint64_t* map = b.spikes[h];
        for (int t = 0; t < b.tile_count; t++) {
            map[t] = (map[t] & ~b.inhibitory[t]) | (now[t] & b.inhibitory[t]);
        }
    }
}

// each column's correlation between two passes over the same digits; columns silent (or
// constant) in either pass are left out. returns the mean, kept the number used
static double self_correlation(const Rows& a, const Rows& b, const std::vector<int>& cols,
                               int& kept) {
    kept = 0;
    double total = 0;
    int n = a.n < b.n ? a.n : b.n;
    for (int c : cols) {
        double sa = 0, sb = 0, saa = 0, sbb = 0, sab = 0;
        for (int r = 0; r < n; r++) {
            double x = a.v[(size_t)r * a.d + (size_t)c], y = b.v[(size_t)r * b.d + (size_t)c];
            sa += x, sb += y, saa += x * x, sbb += y * y, sab += x * y;
        }
        double va = saa - sa * sa / n, vb = sbb - sb * sb / n;
        if (va <= 1e-9 || vb <= 1e-9) {
            continue;
        }
        total += (sab - sa * sb / n) / std::sqrt(va * vb);
        kept++;
    }
    return kept ? total / kept : 0.0;
}

// --twice: the test digits again from another starting state, and the line scripts read
void repeat_twice(Sheet& s, const Rows& test, const std::vector<int>& cols, int test_count) {
    sheet_train(s, 1, false, nullptr);
    Rows again;
    sheet_eval(s, 0, 0, test_count, &again);
    int kept = 0;
    double corr = self_correlation(test, again, cols, kept);
    // activity over the counted ticks of the first pass
    double sum = 0;
    int64_t active = 0;
    for (int r = 0; r < test.n; r++) {
        for (int c : cols) {
            uint8_t x = test.v[(size_t)r * test.d + (size_t)c];
            sum += x;
            active += x > 0;
        }
    }
    double cell_digits = (double)test.n * (double)cols.size();
    int counted = s.flags.plug.ticks_per_step - s.count_from;
    std::printf("repeat: self-correlation %.4f over %d of %zu cells; rate %.1f per 1000 "
                "cell-ticks counted; active %.4f; mean %.3f spikes a digit\n",
                corr, kept, cols.size(), 1000.0 * sum / (cell_digits * counted),
                (double)active / cell_digits, sum / cell_digits);
}
