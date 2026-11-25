#include "blob/tick_steps.hpp"
#include "mnist/mnist_run.hpp"

#include <cstdio>
#include <cstring>

// the inside's ladder (docs/results-2026-11-25-inside-ladder.md, scripts/ladder2.sh): an
// offline model given the cells' constraints one at a time, trained by backprop as a
// measuring stick. this file is its plug into the real sheet: the born sheet written out (so
// the model has the real cells, kinds, reach and born wiring), a model's wiring written into
// a sheet (the transplant), and the spike counts a reader sees (so the model's reader can be
// checked against the tree's). all three are off unless their flag is given.

static bool bit_at(const uint64_t* words, int n) {
    return (words[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
}

// one line per neuron: number, x, y, kind bits (1 sensor, 2 motor, 4 inhibitory, 8
// pacemaker), pixel line (-1 none), threshold, leak shift, cooldown, the two far tiles (-1
// none), then every wired source as neuron:strength
void sheet_describe(const Sheet& s, const char* path) {
    const Blob& b = *s.blob;
    const Plug& plug = s.tt->plug;
    std::vector<int> pixel_of((size_t)b.neuron_count, -1);
    int group = plug_sensor_group(plug);
    for (int line = plug.first_sheet; line < plug.first_number; line++) {
        for (int k = 0; k < group; k++) {
            pixel_of[(size_t)plug.sensor_neuron[line * PLUG_MAX_GROUP + k]] =
                line - plug.first_sheet;
        }
    }
    FILE* f = std::fopen(path, "w");
    if (!f) {
        std::fprintf(stderr, "sheet-describe: cannot write %s\n", path);
        return;
    }
    std::fprintf(f, "sheet %d %d %d\n", b.neuron_count, b.tiles_w, b.tiles_h);
    for (int n = 0; n < b.neuron_count; n++) {
        int kind = bit_at(b.sensor, n) | bit_at(b.motor, n) << 1 | bit_at(b.inhibitory, n) << 2 |
                   bit_at(b.pacemaker, n) << 3;
        int far[FAR_SLOTS];
        for (int k = 0; k < FAR_SLOTS; k++) {
            uint32_t t = b.far_tile[n * FAR_SLOTS + k];
            far[k] = t == (uint32_t)b.tile_count ? -1 : (int)t;
        }
        std::fprintf(f, "%d %d %d %d %d %d %d %d %d %d", n, blob_neuron_x(b, n),
                     blob_neuron_y(b, n), kind, pixel_of[(size_t)n], b.threshold[n],
                     b.leak_shift[n], b.cooldown_len[n], far[0], far[1]);
        for (int k = 0; k < SLOTS; k++) {
            uint32_t src = slot_source(b, n, k);
            if (src == (uint32_t)b.tile_count) {
                continue;
            }
            for (int bit = 0; bit < TILE_NEURONS; bit++) {
                int w = blob_weight(b, n, k, bit);
                if (w > 0) {
                    std::fprintf(f, " %d:%d", (int)src * TILE_NEURONS + bit, w);
                }
            }
        }
        std::fputc('\n', f);
    }
    std::fclose(f);
}

// a model's wiring into the sheet: each line "n src:strength ...", strength 1, 4 or 5. a
// listed neuron loses all it had and takes exactly these; a source outside its slots (not in
// its 9 local tiles or its far tiles) is skipped and counted. a line "t n value" sets a
// threshold. returns false if the file cannot be read
bool sheet_load_wiring(Sheet& s, const char* path) {
    Blob& b = *s.blob;
    FILE* f = std::fopen(path, "r");
    if (!f) {
        std::fprintf(stderr, "wiring-load: cannot read %s\n", path);
        return false;
    }
    static char line[1 << 16];
    int64_t cells = 0, wired = 0, skipped = 0, thresholds = 0;
    while (std::fgets(line, sizeof line, f)) {
        char* p = line;
        int n = 0, used = 0;
        if (line[0] == 't') {
            int v = 0;
            if (std::sscanf(line + 1, "%d %d", &n, &v) == 2 && n >= 0 && n < b.neuron_count) {
                b.threshold[n] = (int16_t)v;
                thresholds++;
            }
            continue;
        }
        if (std::sscanf(p, "%d%n", &n, &used) != 1 || n < 0 || n >= b.neuron_count) {
            continue;
        }
        p += used;
        for (int k = 0; k < SLOTS; k++) {
            b.w0[blob_weight_index(n, k)] = 0;
            b.w1[blob_weight_index(n, k)] = 0;
        }
        cells++;
        int src = 0, w = 0;
        while (std::sscanf(p, " %d:%d%n", &src, &w, &used) == 2) {
            p += used;
            int slot = -1;
            for (int k = 0; k < SLOTS && slot < 0; k++) {
                slot = (int)slot_source(b, n, k) == src / TILE_NEURONS ? k : -1;
            }
            if (slot < 0 || src == n || (w != 1 && w != 4 && w != 5)) {
                skipped++;
                continue;
            }
            uint64_t bit = 1ull << (src % TILE_NEURONS);
            int64_t wi = blob_weight_index(n, slot);
            b.w0[wi] |= (w & 1) ? bit : 0;
            b.w1[wi] |= w >= 4 ? bit : 0;
            wired++;
        }
        b.fan_in[n] = (uint16_t)count_fan_in(b, n);
    }
    std::fclose(f);
    std::printf("wiring-load: %s: %lld cells rewired, %lld connections, %lld outside the cells' "
                "slots skipped, %lld thresholds set\n",
                path, (long long)cells, (long long)wired, (long long)skipped,
                (long long)thresholds);
    return true;
}

// the reader's rows, only the given columns: "rows N D" then for each row its digit's index
// and its partner's (two bytes each, low first; the partner is 0 off the pair test), its
// label byte and D count bytes; train first, then test
void rows_write(const RowsOut& train, const RowsOut& test, const std::vector<int>& cols,
                const char* path) {
    FILE* f = std::fopen(path, "wb");
    if (!f) {
        std::fprintf(stderr, "rows-out: cannot write %s\n", path);
        return;
    }
    std::vector<uint8_t> row(cols.size() + 5);
    const RowsOut* sets[2] = {&train, &test};
    for (const RowsOut* r : sets) {
        const Rows& rows = *r->rows;
        std::fprintf(f, "rows %d %zu\n", rows.n, cols.size());
        for (int i = 0; i < rows.n; i++) {
            int image = i < (int)r->images->size() ? (*r->images)[(size_t)i] : 0;
            int partner = i < (int)r->partners->size() ? (*r->partners)[(size_t)i] : 0;
            row[0] = (uint8_t)(image & 255);
            row[1] = (uint8_t)(image >> 8);
            row[2] = (uint8_t)(partner & 255);
            row[3] = (uint8_t)(partner >> 8);
            row[4] = rows.label[(size_t)i];
            for (size_t j = 0; j < cols.size(); j++) {
                row[j + 5] = rows.v[(size_t)i * rows.d + (size_t)cols[j]];
            }
            std::fwrite(row.data(), 1, row.size(), f);
        }
    }
    std::fclose(f);
}
