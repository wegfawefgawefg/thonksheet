#include "blob/random_words.hpp"
#include "table/table.hpp"

#include <bit>

// one tick of the table sheet: K bit reads and one lookup per neuron, then noise, the
// world's sensor bits, and the rate regulation.

// growth lives in table_learn.cpp
void table_grow(TableBlob& tb);

// the addresses of one tile's 64 neurons, and the word of their outputs: a neuron fires
// when its addressed counter is at or above its sector's bar. input j of a neuron is bit j
// of its address; rows[d] is the spike map d ticks back.
static uint64_t tile_lookup(const TableBlob& tb, int tile, const uint64_t* const* rows,
                            uint8_t* address) {
    const int k_inputs = tb.inputs;
    const int bits = tb.bits;
    const int64_t wpn = tb.words_per_neuron;
    const uint64_t mask = (1ull << bits) - 1;
    const uint64_t bar = tb.bar[tb.tile_sector[tile]];
    const uint32_t* in = &tb.input[(int64_t)tile * TILE_NEURONS * k_inputs];
    const uint64_t* table = &tb.table[(int64_t)tile * TILE_NEURONS * wpn];
    uint8_t local[TILE_NEURONS];
    uint64_t out = 0;
    for (int bit = 0; bit < TILE_NEURONS; bit++) {
        uint32_t addr = 0;
        for (int j = 0; j < k_inputs; j++) {
            uint32_t e = in[j];
            uint32_t src = e >> 5;
            addr |= (uint32_t)((rows[e & 31][src >> 6] >> (src & 63)) & 1) << j;
        }
        in += k_inputs;
        local[bit] = (uint8_t)addr;
        uint32_t at = addr * (uint32_t)bits;
        uint64_t value = (table[bit * wpn + (at >> 6)] >> (at & 63)) & mask;
        out |= (uint64_t)(value >= bar) << bit;
    }
    for (int bit = 0; bit < TILE_NEURONS; bit++) {
        address[tile * TILE_NEURONS + bit] = local[bit];
    }
    return out;
}

// per neuron: a counter that said fire steps down by chance, one that said quiet steps up
// by a smaller chance. only the counter just used moves.
static void regulate_counters(TableBlob& tb, int tile, uint64_t out, const uint8_t* address) {
    uint64_t free_cells = ~tb.shell.sensor[tile];
    uint64_t down = out & free_cells & sparse_word(tb.rng, tb.tg.rate_fire_shift);
    uint64_t up = ~out & free_cells & sparse_word(tb.rng, tb.tg.rate_quiet_shift);
    bool rest_only = tb.tg.rate_rest_only;
    while (down) {
        int n = tile * TILE_NEURONS + __builtin_ctzll(down);
        down &= down - 1;
        if (!rest_only || address[n] == 0) {
            tb.stats.regulated += table_step(tb, n, address[n], -1);
        }
    }
    while (up) {
        int n = tile * TILE_NEURONS + __builtin_ctzll(up);
        up &= up - 1;
        if (!rest_only || address[n] == 0) {
            tb.stats.regulated += table_step(tb, n, address[n], +1);
        }
    }
}

// per sector: at the end of each bar period, move the bar one step toward the target rate
static void regulate_bars(TableBlob& tb) {
    const TableGenome& tg = tb.tg;
    uint64_t period = 1ull << tg.bar_period_shift;
    if (tb.shell.stats.tick % period != 0) {
        return;
    }
    int top = (1 << tb.bits) - 1;
    for (int s = 0; s < tb.sector_count; s++) {
        uint64_t want = (uint64_t)tb.sector_free[s] * period * (uint64_t)tg.target_per_1024;
        uint64_t got = (uint64_t)tb.sector_fired[s] * 1024;
        if (got * 8 > want * 9 && tb.bar[s] < top) {
            tb.bar[s]++;
        } else if (got * 8 < want * 7 && tb.bar[s] > 1) {
            tb.bar[s]--;
        }
        tb.sector_fired[s] = 0;
    }
}

void table_tick(Blob& blob) {
    TableBlob& tb = table_of(blob);
    Blob& b = tb.shell;
    int ring = (b.now + 1) & (HISTORY - 1);
    uint64_t* map = b.spikes[ring];
    uint64_t* flips = tb.flips[ring];
    uint8_t* address = &tb.address[(int64_t)(ring & (TABLE_ADDR_HISTORY - 1)) * b.neuron_count];
    uint32_t fired = 0, surprise = 0;
    bool per_neuron = tb.tg.rate_mode != 1;
    // codes 1..15 read the ring, 16 + k timeline level k (table_memory.cpp)
    const uint64_t* rows[32];
    for (int d = 0; d < 32; d++) {
        rows[d] = b.spikes[(ring - d) & (HISTORY - 1)];
    }
    for (int k = 0; tb.level && k < tb.tg.timeline_levels && k < TABLE_MAX_LEVELS; k++) {
        rows[16 + k] = tb.level + (int64_t)k * (b.tile_count + 1);
    }

    for (int t = 0; t < b.tile_count; t++) {
        uint64_t out = tile_lookup(tb, t, rows, address);
        uint64_t sensor = b.sensor[t];
        uint64_t sensed = b.sense_in[t] & sensor;
        uint64_t flip = sparse_word(tb.rng, tb.tg.noise_shift) & ~sensor;
        b.predicted[t] = out & sensor;
        surprise += std::popcount((b.predicted[t] ^ sensed) & b.sense_attached[t] & sensor);
        // integrator cells fire by their count, not their table; no noise, no rate bar
        uint64_t hold = tb.hold_cell ? tb.hold_cell[t] : 0;
        uint64_t inner = ((out ^ flip) & ~hold) | (hold ? tb.held[t] & hold : 0);
        uint64_t spikes = (inner & ~sensor) | sensed;
        map[t] = spikes;
        flips[t] = flip & ~hold;
        fired += std::popcount(spikes);
        tb.sector_fired[tb.tile_sector[t]] += std::popcount(spikes & ~sensor & ~hold);
        if (per_neuron) {
            regulate_counters(tb, t, out, address);
        }
    }
    b.now = ring;
    b.stats.tick++;
    b.stats.fired = fired;
    b.stats.surprise = surprise;
    table_memory_tick(tb);
    if (tb.tg.rate_mode >= 1) {
        regulate_bars(tb);
    }
    if (tb.tg.learning_on && tb.tg.grow_shift > 0) {
        table_grow(tb);
    }
    if (tb.tg.report) {
        table_report_tick(tb);
    }
}
