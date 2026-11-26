#include "mnist/mnist_run.hpp"
#include "world/mnist_data.hpp"

#include <cmath>
#include <cstdio>

// what a reward-free life did to the inside (docs/results-2026-11-25-compete.md): how alive
// the hidden cells are, and where their inputs come from. "hidden" is every cell that is not
// a sensor, motor or pacemaker cell; the wiring numbers are over the excitatory ones.

static bool bit_at(const uint64_t* words, int n) {
    return (words[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
}

static bool hidden(const Blob& b, int n) {
    return !bit_at(b.sensor, n) && !bit_at(b.motor, n) && !bit_at(b.pacemaker, n);
}

// how alive the hidden cells were over some digits' spike counts
void activity_report(const Sheet& s, const Rows& rows, const char* label) {
    const Blob& b = *s.blob;
    std::vector<double> sum((size_t)b.neuron_count, 0.0);
    for (int r = 0; r < rows.n; r++) {
        const uint8_t* x = &rows.v[(size_t)r * rows.d];
        for (int n = 0; n < b.neuron_count; n++) {
            sum[(size_t)n] += x[n];
        }
    }
    int cells = 0, silent = 0, busy = 0, dead_tiles = 0;
    double total = 0;
    for (int n = 0; n < b.neuron_count; n++) {
        if (hidden(b, n)) {
            cells++;
            total += sum[(size_t)n];
        }
    }
    double mean = cells > 0 ? total / cells : 0.0;
    for (int t = 0; t < b.tile_count; t++) {
        double tile = 0;
        for (int n = t * TILE_NEURONS; n < (t + 1) * TILE_NEURONS; n++) {
            if (!hidden(b, n)) {
                continue;
            }
            silent += sum[(size_t)n] == 0;
            busy += sum[(size_t)n] > 10 * mean;
            tile += sum[(size_t)n];
        }
        dead_tiles += tile == 0;
    }
    int ticks = s.flags.plug.ticks_per_step + s.gap;
    double rate = rows.n > 0 && ticks > 0 ? 1000.0 * mean / ((double)rows.n * ticks) : 0.0;
    std::printf("%-24s hidden %d cells over %d digits: %.1f spikes per 1000 cell-ticks, silent "
                "%.3f, over 10x the mean %.3f, tiles with no hidden spike %d\n",
                label, cells, rows.n, rate, cells ? (double)silent / cells : 0.0,
                cells ? (double)busy / cells : 0.0, dead_tiles);
}

// the pixels' correlation over the first 10,000 training digits, in the sheet's own code
static const std::vector<float>& pixel_correlation(int code, int& d) {
    static std::vector<float> corr;
    static int side = 0;
    int w = 0, h = 0;
    mnist_code_size(code, w, h);
    d = w * h;
    if (!corr.empty() && side == d) {
        return corr;
    }
    side = d;
    const Mnist* data = mnist_data();
    const int count = 10000, words = (count + 63) / 64;
    std::vector<uint64_t> on((size_t)d * words, 0);
    std::vector<uint8_t> bits((size_t)d);
    std::vector<double> ones((size_t)d, 0.0);
    for (int i = 0; i < count; i++) {
        mnist_encode(&data->train.pixels[(size_t)i * MNIST_PIXELS], code, 0, 0, bits.data());
        for (int p = 0; p < d; p++) {
            if (bits[(size_t)p]) {
                on[(size_t)p * words + i / 64] |= 1ull << (i % 64);
                ones[(size_t)p] += 1;
            }
        }
    }
    corr.assign((size_t)d * d, 0.0f);
    for (int i = 0; i < d; i++) {
        for (int j = i + 1; j < d; j++) {
            double both = 0;
            for (int k = 0; k < words; k++) {
                both += __builtin_popcountll(on[(size_t)i * words + k] & on[(size_t)j * words + k]);
            }
            double ni = ones[(size_t)i], nj = ones[(size_t)j];
            double den = std::sqrt(ni * (count - ni) * nj * (count - nj));
            float c = den > 0 ? (float)((count * both - ni * nj) / den) : 0.0f;
            corr[(size_t)i * d + j] = corr[(size_t)j * d + i] = c;
        }
    }
    return corr;
}

void wiring_snapshot(const Sheet& s, std::vector<uint64_t>& born) {
    const Blob& b = *s.blob;
    size_t words = (size_t)b.neuron_count * SLOTS;
    born.resize(words);
    for (size_t i = 0; i < words; i++) {
        born[i] = b.w0[i] | b.w1[i];
    }
}

// where the hidden excitatory cells' inputs come from: how many are pixels, how alike the
// pixels a cell listens to are on real digits (mean correlation over pairs), how far they
// sit from their middle, and how many inputs are still those it was born with
void wiring_report(const Sheet& s, const std::vector<uint64_t>& born, const char* label) {
    const Blob& b = *s.blob;
    const Plug& plug = s.tt->plug;
    int code = s.flags.task_settings.mnist_code, d = 0, w = 0, h = 0;
    mnist_code_size(code, w, h);
    const std::vector<float>& corr = pixel_correlation(code, d);
    // each pixel cell's pixel, -1 for every other cell
    std::vector<int> pixel_of((size_t)b.neuron_count, -1);
    int group = plug_sensor_group(plug);
    for (int line = plug.first_sheet; line < plug.first_number; line++) {
        for (int k = 0; k < group; k++) {
            pixel_of[(size_t)plug.sensor_neuron[line * PLUG_MAX_GROUP + k]] = line - plug.first_sheet;
        }
    }
    double cells = 0, inputs = 0, strong = 0, pixels = 0, kept = 0, shaped = 0, alike = 0,
           spread = 0;
    std::vector<int> mine;
    for (int n = 0; n < b.neuron_count; n++) {
        if (!hidden(b, n) || bit_at(b.inhibitory, n)) {
            continue;
        }
        cells++;
        mine.clear();
        for (int k = 0; k < SLOTS; k++) {
            uint32_t src = k < LOCAL_SLOTS ? b.local_tile[(n / TILE_NEURONS) * LOCAL_SLOTS + k]
                                           : b.far_tile[n * FAR_SLOTS + k - LOCAL_SLOTS];
            if (src == (uint32_t)b.tile_count) {
                continue;
            }
            int64_t wi = blob_weight_index(n, k);
            uint64_t wired = b.w0[wi] | b.w1[wi];
            inputs += __builtin_popcountll(wired);
            strong += __builtin_popcountll(b.w1[wi]);
            kept += born.empty() ? 0 : __builtin_popcountll(wired & born[(size_t)wi]);
            for (uint64_t m = wired; m; m &= m - 1) {
                int p = pixel_of[(size_t)(src * TILE_NEURONS + __builtin_ctzll(m))];
                if (p >= 0) {
                    mine.push_back(p);
                }
            }
        }
        pixels += (double)mine.size();
        if (mine.size() < 2) {
            continue;
        }
        shaped++;
        double c = 0, cx = 0, cy = 0, r = 0;
        int pairs = 0;
        for (size_t i = 0; i < mine.size(); i++) {
            cx += mine[i] % w;
            cy += mine[i] / w;
            for (size_t j = i + 1; j < mine.size(); j++) {
                c += corr[(size_t)mine[i] * d + (size_t)mine[j]];
                pairs++;
            }
        }
        cx /= (double)mine.size();
        cy /= (double)mine.size();
        for (int p : mine) {
            r += std::sqrt((p % w - cx) * (p % w - cx) + (p / w - cy) * (p / w - cy));
        }
        alike += c / pairs;
        spread += r / (double)mine.size();
    }
    std::printf("%-24s hidden excitatory %.0f: inputs %.1f (strong %.1f), pixels %.1f (%.3f of "
                "inputs), born inputs kept %.3f; with 2+ pixels (%.0f): pixel likeness %.4f, "
                "spread %.2f px\n",
                label, cells, inputs / cells, strong / cells, pixels / cells,
                inputs > 0 ? pixels / inputs : 0.0, inputs > 0 && !born.empty() ? kept / inputs : 1.0,
                shaped, shaped > 0 ? alike / shaped : 0.0, shaped > 0 ? spread / shaped : 0.0);
}

// a reward-free life under the genome's own rules, in chunks, with how alive and how wired
// the hidden cells are after each
void life_run(Sheet& s, int64_t digits, int chunks, const std::vector<uint64_t>& born) {
    chunks = chunks < 1 ? 1 : chunks;
    int64_t done = 0;
    for (int c = 0; c < chunks; c++) {
        int64_t part = (digits * (c + 1)) / chunks - done;
        Rows rows;
        double t0 = now_seconds();
        int64_t ticks0 = s.ticks;
        int64_t right = sheet_train(s, part, s.teach > 0, &rows);
        double secs = now_seconds() - t0;
        done += part;
        char label[64];
        std::snprintf(label, sizeof label, "life %lld (%.0f t/s)", (long long)done,
                      (double)(s.ticks - ticks0) / secs);
        activity_report(s, rows, label);
        wiring_report(s, born, label);
        if (s.outside) {
            std::printf("%-24s outside output right %.3f\n", label, outside_right(*s.outside));
        }
        if (s.teach > 0 || s.outside) {
            uint64_t c[7];
            blob_hop_counts(*s.blob, c);
            double cell_ticks = c[6] > 0 ? (double)c[6] : 1.0;
            std::printf("%-24s readout right %.3f; hop: verdicts %llu, cells taught %llu, "
                        "counter steps %llu, rewires %llu; near and not fired %.4f, near and "
                        "fired %.4f of learner cell-ticks in the windows\n",
                        label, part > 0 ? (double)right / (double)part : 0.0,
                        (unsigned long long)c[0], (unsigned long long)c[1],
                        (unsigned long long)c[2], (unsigned long long)c[3], c[4] / cell_ticks,
                        c[5] / cell_ticks);
        }
        if (s.blob->genome.out_on) {
            uint64_t c[4];
            blob_output_counts(*s.blob, c);
            std::printf("%-24s own output: verdicts %llu, moved on %llu, lines moved %llu\n",
                        label, (unsigned long long)c[0], (unsigned long long)c[1],
                        (unsigned long long)c[2]);
        }
        std::fflush(stdout);
    }
}
