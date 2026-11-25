#include "blob/random_words.hpp"
#include "vm/vm.hpp"

#include <bit>
#include <cstring>

// one tick of the vm: the world's code into X, every entry's match by AND and popcount, the
// k best fire, their operations change M, their evidence votes O, and the shell's spike map
// shows the result for the plug.

constexpr int HIST = 2048; // match values counted exactly; higher ones share the top bin

// the world's part: OR of the codes of the sensor cells that are on
static void write_input(VmBlob& v) {
    const Blob& b = v.shell;
    uint64_t* x = vm_x(v);
    std::memset(x, 0, sizeof(uint64_t) * (size_t)v.x_words);
    int bits = v.vg.x_bits;
    for (int i = 0; i < v.sensor_cells; i++) {
        int n = v.sensor_cell[i];
        if (!((b.sense_in[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1)) {
            continue;
        }
        const uint16_t* code = v.x_code + (size_t)i * bits;
        for (int k = 0; k < bits; k++) {
            x[code[k] >> 6] |= 1ull << (code[k] & 63);
        }
    }
}

// popcount of address AND state for every entry; returns the bar the k winners clear
static int match_all(VmBlob& v, uint32_t* hist) {
    int E = v.vg.entries;
    int aw = v.a_words;
    const uint64_t* s = v.state;
    std::memset(hist, 0, sizeof(uint32_t) * HIST);
    int xw = v.x_words;
    for (int e = 0; e < E; e++) {
        const uint64_t* a = v.addr + (size_t)e * aw;
        int mx = 0, m = 0;
        for (int w = 0; w < xw; w++) {
            mx += std::popcount(a[w] & s[w]);
        }
        for (int w = xw; w < aw; w++) {
            m += std::popcount(a[w] & s[w]);
        }
        m += mx;
        m = m < HIST - 1 ? m : HIST - 1;
        v.match_x[e] = (uint16_t)mx;
        v.match[e] = (uint16_t)m;
        hist[m]++;
    }
    // the highest bar with at least k entries at or above it
    uint32_t above = 0;
    int bar = HIST - 1;
    for (; bar > 0; bar--) {
        above += hist[bar];
        if (above >= (uint32_t)v.vg.active) {
            break;
        }
    }
    return bar;
}

// the k winners: everything above the bar, then entries at the bar from a random start
static void pick_winners(VmBlob& v, int bar) {
    int E = v.vg.entries;
    int K = v.vg.active;
    int got = 0;
    for (int e = 0; e < E && got < K; e++) {
        if (v.match[e] > bar) {
            v.winners[got++] = e;
        }
    }
    int start = (int)rng_below(v.rng, (uint32_t)E);
    for (int i = 0; i < E && got < K; i++) {
        int e = start + i < E ? start + i : start + i - E;
        if (v.match[e] == bar) {
            v.winners[got++] = e;
        }
    }
    for (; got < K; got++) {
        v.winners[got] = v.winners[0]; // only when fewer than k entries exist at all
    }
}

// set wins where nobody clears, clear wins where nobody sets, a conflict keeps the bit. a
// homeostat thins the sets when M is over its target density and the clears when under.
static void apply_operations(VmBlob& v) {
    uint64_t* m = vm_m(v);
    int mw = v.m_words;
    int64_t target = (int64_t)v.vg.m_target * v.free_cells / 1024;
    bool over = (int64_t)v.stats.m_on > target;
    uint32_t on = 0;
    for (int w = 0; w < mw; w++) {
        uint64_t s = 0, c = 0;
        for (int k = 0; k < v.vg.active; k++) {
            if (v.vg.refract && !v.fresh[k]) {
                continue;
            }
            size_t at = (size_t)v.winners[k] * mw + w;
            s |= v.set[at];
            c |= v.clr[at];
        }
        if (v.event_entry >= 0) {
            s |= v.set[(size_t)v.event_entry * mw + w];
            c |= v.clr[(size_t)v.event_entry * mw + w];
        }
        uint64_t r = rng_next(v.rng);
        if (v.vg.homeostat) {
            s &= over ? r : ~0ull;
            c &= over ? ~0ull : r;
        }
        m[w] = (m[w] & ~(c & ~s)) | (s & ~c);
        on += (uint32_t)std::popcount(m[w]);
    }
    v.stats.m_on = on;
}

// the sliding ring: the last m_slide_words words of M move one bit along, the top bit of
// each word carried into the next, the last word's into the first
static void slide(VmBlob& v) {
    int n = v.vg.m_slide_words < v.m_words ? v.vg.m_slide_words : v.m_words;
    if (n <= 0 || v.free_cells % 64) {
        return; // the ring needs whole words
    }
    uint64_t* ring = vm_m(v) + (v.m_words - n);
    uint64_t carry = ring[n - 1] >> 63;
    for (int w = 0; w < n; w++) {
        uint64_t out = ring[w] >> 63;
        ring[w] = (ring[w] << 1) | carry;
        carry = out;
    }
}

// each motor cell: the winners' evidence summed; on above zero, or by chance
static void vote_output(VmBlob& v) {
    int cells = v.motor_cells;
    std::memset(v.vote, 0, sizeof(int16_t) * (size_t)cells);
    for (int k = 0; k < v.vg.active; k++) {
        const int8_t* row = v.ev + (size_t)v.winners[k] * cells;
        for (int j = 0; j < cells; j++) {
            v.vote[j] = (int16_t)(v.vote[j] + row[j]);
        }
    }
    uint64_t* o = vm_o(v);
    for (int w = 0; w < v.o_words; w++) {
        o[w] = sparse_word(v.rng, v.vg.out_noise_shift);
    }
    // rate code: a cell fires when its vote beats a fresh random bar of out_bar_bits bits,
    // so a stronger vote fires more often and equal votes split by chance instead of tying
    int bits = v.vg.out_bar_bits;
    int per_word = 64 / bits;
    uint64_t r = 0;
    for (int j = 0; j < cells; j++) {
        if ((j % per_word) == 0) {
            r = rng_next(v.rng);
        }
        int bar = (int)(r & ((1ull << bits) - 1));
        r >>= bits;
        if (v.vote[j] > bar) {
            o[j / 64] |= 1ull << (j % 64);
        }
    }
    int tail = cells % 64;
    if (tail) {
        o[v.o_words - 1] &= (1ull << tail) - 1;
    }
}

static void set_cell(uint64_t* map, int n) {
    map[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
}

// the shell's spike map: sensor cells as sensed, motor cells as O, the rest as M
static void show_on_sheet(VmBlob& v) {
    Blob& b = v.shell;
    b.now = (b.now + 1) & (HISTORY - 1);
    uint64_t* map = b.spikes[b.now];
    uint32_t surprise = 0;
    for (int t = 0; t < b.tile_count; t++) {
        uint64_t sensed = b.sense_in[t] & b.sensor[t];
        map[t] = sensed;
        // no prediction of the input yet: every attached sensor spike is a surprise
        surprise += (uint32_t)std::popcount(sensed & b.sense_attached[t]);
    }
    map[b.tile_count] = 0;
    const uint64_t* o = vm_o(v);
    for (int w = 0; w < v.o_words; w++) {
        for (uint64_t bits = o[w]; bits; bits &= bits - 1) {
            set_cell(map, v.motor_cell[w * 64 + std::countr_zero(bits)]);
        }
    }
    const uint64_t* m = vm_m(v);
    for (int w = 0; w < v.m_words; w++) {
        for (uint64_t bits = m[w]; bits; bits &= bits - 1) {
            set_cell(map, v.free_cell[w * 64 + std::countr_zero(bits)]);
        }
    }
    uint32_t fired = 0;
    for (int t = 0; t < b.tile_count; t++) {
        fired += (uint32_t)std::popcount(map[t]);
    }
    b.stats.tick++;
    b.stats.fired = fired;
    b.stats.surprise = surprise;
}

void vm_tick(Blob& blob) {
    VmBlob& v = vm_of(blob);
    uint32_t hist[HIST];
    v.now++;
    write_input(v);
    int bar = match_all(v, hist);
    pick_winners(v, bar);
    for (int k = 0; k < v.vg.active; k++) {
        v.fresh[k] = v.last_fired[v.winners[k]] + 1 != v.now;
        v.last_fired[v.winners[k]] = v.now;
    }
    vm_events(v);
    apply_operations(v);
    slide(v);
    vote_output(v);
    show_on_sheet(v);
    if (v.trace) {
        vm_trace_tick(v);
    }
}
