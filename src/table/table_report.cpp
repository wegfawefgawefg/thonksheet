#include "table/table.hpp"

#include <bit>
#include <cstdio>

// spreads for "does it saturate": counter values over all entries and over the entries
// used in the span, and per-neuron firing rates over the span. sensor neurons left out.

void table_report_tick(TableBlob& tb) {
    const Blob& b = tb.shell;
    const uint64_t* map = b.spikes[b.now];
    const uint8_t* address =
        &tb.address[(int64_t)(b.now & (TABLE_ADDR_HISTORY - 1)) * b.neuron_count];
    for (int t = 0; t < b.tile_count; t++) {
        uint64_t w = map[t];
        while (w) {
            tb.fire_count[t * TILE_NEURONS + __builtin_ctzll(w)]++;
            w &= w - 1;
        }
    }
    for (int n = 0; n < b.neuron_count; n++) {
        int a = address[n];
        tb.visited[(int64_t)n * tb.words_per_plane + (a >> 6)] |= 1ull << (a & 63);
    }
    if (b.stats.tick - tb.report_start >= (uint64_t)tb.tg.report_ticks) {
        char when[32];
        std::snprintf(when, sizeof when, "tick %llu", (unsigned long long)b.stats.tick);
        table_report(tb, when);
    }
}

static bool is_sensor(const Blob& b, int n) {
    return (b.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
}

void table_report(TableBlob& tb, const char* when) {
    const Blob& b = tb.shell;
    uint64_t span = b.stats.tick - tb.report_start;
    if (span == 0) {
        return;
    }
    int levels = 1 << tb.bits;
    int entries = 1 << tb.inputs;
    double all[1 << TABLE_MAX_BITS] = {}, used[1 << TABLE_MAX_BITS] = {};
    double all_total = 0, used_total = 0;
    // firing-rate buckets: 0, under 1/64, under 1/16, under 1/4, under 1/2, 1/2 and up
    double rate_bucket[6] = {};
    double rate_sum = 0, neurons = 0;
    for (int n = 0; n < b.neuron_count; n++) {
        if (is_sensor(b, n)) {
            continue;
        }
        neurons++;
        for (int a = 0; a < entries; a++) {
            int v = table_counter(tb, n, a);
            all[v]++;
            all_total++;
            if ((tb.visited[(int64_t)n * tb.words_per_plane + (a >> 6)] >> (a & 63)) & 1) {
                used[v]++;
                used_total++;
            }
        }
        double rate = (double)tb.fire_count[n] / (double)span;
        rate_sum += rate;
        int bucket = rate == 0         ? 0
                     : rate < 1.0 / 64 ? 1
                     : rate < 1.0 / 16 ? 2
                     : rate < 1.0 / 4  ? 3
                     : rate < 1.0 / 2  ? 4
                                       : 5;
        rate_bucket[bucket]++;
    }
    std::printf("table report %s (span %llu ticks)\n", when, (unsigned long long)span);
    std::printf("  counters, all entries: ");
    for (int v = 0; v < levels; v++) {
        std::printf(" %d:%5.1f%%", v, 100.0 * all[v] / all_total);
    }
    std::printf("\n  counters, used entries:");
    for (int v = 0; v < levels; v++) {
        std::printf(" %d:%5.1f%%", v, used_total > 0 ? 100.0 * used[v] / used_total : 0.0);
    }
    std::printf("   (%.1f entries used per neuron of %d)\n", used_total / neurons, entries);
    std::printf("  firing rate: mean %.4f  silent %.1f%%  <1/64 %.1f%%  <1/16 %.1f%%  <1/4 "
                "%.1f%%  <1/2 %.1f%%  >=1/2 %.1f%%\n",
                rate_sum / neurons, 100 * rate_bucket[0] / neurons, 100 * rate_bucket[1] / neurons,
                100 * rate_bucket[2] / neurons, 100 * rate_bucket[3] / neurons,
                100 * rate_bucket[4] / neurons, 100 * rate_bucket[5] / neurons);
    int bar_lo = 1 << tb.bits, bar_hi = 0;
    double bar_sum = 0;
    for (int s = 0; s < tb.sector_count; s++) {
        bar_lo = tb.bar[s] < bar_lo ? tb.bar[s] : bar_lo;
        bar_hi = tb.bar[s] > bar_hi ? tb.bar[s] : bar_hi;
        bar_sum += tb.bar[s];
    }
    std::printf("  sector bars: mean %.2f, %d to %d\n", bar_sum / tb.sector_count, bar_lo, bar_hi);
    std::printf("  moved so far: regulation %llu, good/bad %llu, inputs swapped %llu\n",
                (unsigned long long)tb.stats.regulated, (unsigned long long)tb.stats.learned,
                (unsigned long long)tb.stats.grown);
    std::fflush(stdout);

    // start the next span
    tb.report_start = b.stats.tick;
    for (int n = 0; n < b.neuron_count; n++) {
        tb.fire_count[n] = 0;
    }
    for (int64_t i = 0; i < (int64_t)b.neuron_count * tb.words_per_plane; i++) {
        tb.visited[i] = 0;
    }
}
