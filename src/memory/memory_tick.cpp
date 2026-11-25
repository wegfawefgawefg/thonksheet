#include "memory/memory.hpp"

#include <bit>
#include <cstring>

// one tick: read what the world wrote, cut events, make a frame when the address changes,
// fire the chosen button's motor cells, and predict the next input.

static uint64_t mix(uint64_t h, uint64_t v) {
    h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
    return h * 0xff51afd7ed558ccdull;
}

static uint64_t hash_input(const MemoryBlob& m, const uint64_t* in) {
    uint64_t h = 0x6d656d;
    for (int w = 0; w < m.in_words; w++) {
        h = mix(h, in[w]);
    }
    return h;
}

// the input that ended becomes the newest past event
static void push_event(MemoryBlob& m) {
    for (int j = MEM_DEPTH_MAX - 1; j > 0; j--) {
        std::memcpy(m.past_code[j], m.past_code[j - 1], sizeof(m.past_code[j]));
        m.past_level[j] = m.past_level[j - 1];
        m.past_in_hash[j] = m.past_in_hash[j - 1];
    }
    std::memcpy(m.past_code[0], m.code_now, sizeof(m.code_now));
    m.past_level[0] = memory_level(m.tick - m.now_start);
    m.past_in_hash[0] = hash_input(m, m.in_now);
    m.stats.events++;
}

// the next-input store: learn what followed the last tick's key, then look up this one's
static void predict(MemoryBlob& m, const uint64_t* in) {
    Blob& b = m.shell;
    uint64_t mask = ((uint64_t)1 << m.mg.predict_bits) - 1;
    if (m.predict_have && m.mg.learning_on) {
        uint64_t e = m.predict_key & mask;
        m.predict_tag[e] = m.predict_key | 1;
        std::memcpy(m.predict_in + e * (size_t)m.in_words, in, sizeof(uint64_t) * m.in_words);
    }
    int64_t in_event = m.tick - m.now_start;
    uint64_t key = mix(mix(hash_input(m, m.in_now), m.past_in_hash[0]),
                       (uint64_t)(in_event < 255 ? in_event : 255));
    m.predict_key = key;
    m.predict_have = true;
    uint64_t e = key & mask;
    bool hit = m.predict_tag[e] == (key | 1);
    m.stats.predict_hits += hit;
    m.stats.predict_misses += !hit;
    const uint64_t* guess = hit ? m.predict_in + e * (size_t)m.in_words : m.in_now;
    for (int w = 0; w < m.in_words; w++) {
        b.predicted[m.in_tile[w]] = guess[w];
    }
}

void memory_tick(Blob& blob) {
    MemoryBlob& m = memory_of(blob);
    Blob& b = m.shell;
    m.tick++;
    b.now = (b.now + 1) & (HISTORY - 1);
    uint64_t* map = b.spikes[b.now];
    for (int t = 0; t <= b.tile_count; t++) {
        map[t] = 0;
    }

    if (m.inner) {
        memory_sheet_tick(m);
    }

    // what is sensed, and how much of it the last prediction got wrong
    uint64_t in[MEM_IN_WORDS];
    bool changed = false;
    uint32_t surprise = 0;
    for (int w = 0; w < m.in_words; w++) {
        int t = m.in_tile[w];
        uint64_t attached = b.sense_attached[t] & b.sensor[t];
        in[w] = b.sense_in[t] & attached;
        surprise += std::popcount((b.predicted[t] ^ in[w]) & attached);
        changed |= in[w] != m.in_now[w];
        map[t] = in[w];
    }
    if (changed) {
        push_event(m);
        std::memcpy(m.in_now, in, sizeof(uint64_t) * m.in_words);
        if (!m.inner) {
            memory_code_of_input(m, m.code_now);
        }
        m.now_start = m.tick;
    }

    // a new frame when the address changes (a new event, or now's length moved a bucket),
    // and after a reward, which redraws ties and exploring
    int level = memory_level(m.tick - m.now_start);
    if (changed || level != m.level_now || m.frame_count == 0 || m.frame_due) {
        m.level_now = level;
        m.frame_due = false;
        if (m.inner) {
            memory_sheet_code(m, m.code_now); // the sheet moves on inside an event
        }
        memory_new_frame(m);
    } else {
        m.frames[(m.frame_count - 1) & (MEM_FRAMES - 1)].last_tick = m.tick;
    }

    // fire the chosen button's attached cells, or the one cell a new button tries
    if (m.drive == -1 && m.new_cell >= 0) {
        int n = m.motor_neuron[m.new_cell];
        map[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
    } else if (m.drive >= 0) {
        for (int c = 0; c < m.motor_cells; c++) {
            if (m.unit_of[c] == m.drive) {
                int n = m.motor_neuron[c];
                map[n / TILE_NEURONS] |= (1ull << (n % TILE_NEURONS)) &
                                         b.motor_attached[n / TILE_NEURONS];
            }
        }
    }

    if (m.predict_tag) {
        predict(m, in);
    } else {
        for (int w = 0; w < m.in_words; w++) {
            b.predicted[m.in_tile[w]] = in[w];
        }
    }

    uint32_t fired = 0;
    for (int t = 0; t < b.tile_count; t++) {
        fired += std::popcount(map[t]);
    }
    b.stats.tick++;
    b.stats.fired = fired;
    b.stats.surprise = surprise;
}
