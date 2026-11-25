#include "blob/random_words.hpp"
#include "memory/memory.hpp"

#include <bit>
#include <cstdlib>

// the combination: a sheet that mixes in front of the memory that stores. a minimal-engine
// sheet is born beside the memory, the world's sensor bits are copied onto it every tick,
// and the memory's code of now is made from the sheet's recent spikes near the sensors
// instead of from the sensor cells themselves.

#ifdef MEMORY_HAS_SHEET
#include "minimal/minimal.hpp"

bool memory_sheet_birth(MemoryBlob& m, const Genome& genome, uint64_t seed) {
    MinimalGenome mg;
    mg.learning_on = m.mg.sheet == 2 ? 1 : 0;
    m.inner = minimal_birth(genome, mg, seed ^ 0x5368656574ull); // "Sheet"
    const Blob& b = *m.inner;
    // the tiles within sheet_reach of a sensor tile
    int reach = m.mg.sheet_reach;
    for (int t = 0; t < b.tile_count && m.sheet_words < MEM_IN_WORDS; t++) {
        int tx = t % b.tiles_w, ty = t / b.tiles_w;
        bool near = false;
        for (int u = 0; u < b.tile_count && !near; u++) {
            int dx = u % b.tiles_w - tx, dy = u / b.tiles_w - ty;
            near = b.sensor[u] && dx <= reach && dx >= -reach && dy <= reach && dy >= -reach;
        }
        if (near) {
            m.sheet_tile[m.sheet_words++] = t;
        }
    }
    // each code bit is the parity of `taps` random cells of those tiles
    int cells = m.sheet_words * 64;
    m.sheet_rows = (uint64_t*)std::calloc((size_t)cells * MEM_FIELD_WORDS, sizeof(uint64_t));
    for (int j = 0; j < MEM_FIELD_BITS; j++) {
        for (int k = 0; k < m.mg.taps; k++) {
            int c = (int)rng_below(m.rng, (uint32_t)cells);
            m.sheet_rows[(size_t)c * MEM_FIELD_WORDS + j / 64] |= 1ull << (j % 64);
        }
    }
    return true;
}

void memory_sheet_tick(MemoryBlob& m) {
    Blob& in = *m.inner;
    const Blob& b = m.shell;
    for (int t = 0; t < b.tile_count; t++) {
        in.sense_in[t] = b.sense_in[t];
        in.sense_attached[t] = b.sense_attached[t];
    }
    minimal_tick(in);
    // a cell is on if it fired in the last sheet_ticks ticks
    for (int w = 0; w < m.sheet_words; w++) {
        uint64_t on = 0;
        for (int age = 0; age < m.mg.sheet_ticks && age < HISTORY; age++) {
            on |= in.spikes[(in.now - age) & (HISTORY - 1)][m.sheet_tile[w]];
        }
        m.sheet_now[w] = on;
    }
}

void memory_sheet_code(MemoryBlob& m, uint64_t* out) {
    for (int w = 0; w < MEM_FIELD_WORDS; w++) {
        out[w] = 0;
    }
    for (int sw = 0; sw < m.sheet_words; sw++) {
        uint64_t on = m.sheet_now[sw];
        while (on) {
            int c = sw * 64 + std::countr_zero(on);
            on &= on - 1;
            const uint64_t* row = m.sheet_rows + (size_t)c * MEM_FIELD_WORDS;
            for (int w = 0; w < MEM_FIELD_WORDS; w++) {
                out[w] ^= row[w];
            }
        }
    }
}

void memory_sheet_reward(MemoryBlob& m, int valence) {
    minimal_reward(*m.inner, valence);
}

void memory_sheet_free(MemoryBlob& m) {
    minimal_free(m.inner);
    std::free(m.sheet_rows);
}

size_t memory_sheet_bytes(const MemoryBlob& m) {
    return minimal_bytes(minimal_of(*m.inner)) +
           sizeof(uint64_t) * MEM_FIELD_WORDS * 64 * (size_t)m.sheet_words;
}

#else

bool memory_sheet_birth(MemoryBlob&, const Genome&, uint64_t) {
    return false;
}
void memory_sheet_tick(MemoryBlob&) {
}
void memory_sheet_code(MemoryBlob&, uint64_t*) {
}
void memory_sheet_reward(MemoryBlob&, int) {
}
void memory_sheet_free(MemoryBlob&) {
}
size_t memory_sheet_bytes(const MemoryBlob&) {
    return 0;
}

#endif
