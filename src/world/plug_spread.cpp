#include "world/plug.hpp"

#include <cstdio>

// --motor-spread N (idea 58): instead of one small group per button, every button gets N
// motor cells spread over the motor region. the region's motor lattice is dealt out like a
// checkerboard, cell (lx, ly) to button (lx + ly) % buttons, so every tile holds cells of
// every button and no place favours one. only cells at least as far from the cues as the
// packed motor lines are used (so --distance still sets the nearest a motor cell gets).
// each button then keeps N of its cells (all it has, if fewer): the nearest N to the cues, or with
// --motor-spread-even 1, N spaced evenly through its list from nearest to farthest.
// nothing here knows the task's answer: it is the same layout for every cue.

static int tile_gap(int cell, int cue_lo, int cue_hi, int dir) {
    return dir > 0 ? cell / 8 - cue_hi / 8 : cue_lo / 8 - cell / 8;
}

bool plug_spread_motors(Plug& plug, const Blob& blob) {
    const PlugSettings& s = plug.settings;
    const Genome& g = blob.genome;
    int lines = plug.motor_lines;
    if (s.motor_spread <= 0 || lines <= 0) {
        return true;
    }
    // the facing axis and direction, as --distance has them
    int dx = (g.motor_x + g.motor_w / 2) - (g.sensor_x + g.sensor_w / 2);
    int dy = (g.motor_y + g.motor_h / 2) - (g.sensor_y + g.sensor_h / 2);
    bool along_y = (dy < 0 ? -dy : dy) >= (dx < 0 ? -dx : dx);
    int dir = (along_y ? dy : dx) >= 0 ? 1 : -1;
    const int* sensor_cell = along_y ? plug.sensor_cell_y : plug.sensor_cell_x;
    const int* motor_cell = along_y ? plug.motor_cell_y : plug.motor_cell_x;
    int first_cue = plug.sensor_lines > plug.first_single ? plug.first_single : 0;
    int sensor_reach = (s.sensor_side - 1) * g.sensor_stride;
    int cue_lo = 1 << 30, cue_hi = -(1 << 30);
    for (int line = first_cue; line < plug.sensor_lines; line++) {
        cue_lo = sensor_cell[line] < cue_lo ? sensor_cell[line] : cue_lo;
        int hi = sensor_cell[line] + sensor_reach;
        cue_hi = hi > cue_hi ? hi : cue_hi;
    }
    // the packed motor lines' gap from the cues: no spread cell sits nearer
    int nearest = 1 << 30;
    int motor_reach = (s.motor_side - 1) * g.motor_stride;
    for (int line = 0; line < lines; line++) {
        int edge = dir > 0 ? motor_cell[line] : motor_cell[line] + motor_reach;
        int gap = tile_gap(edge, cue_lo, cue_hi, dir);
        nearest = gap < nearest ? gap : nearest;
    }

    // every usable lattice cell, nearest tile row (or column) first
    int lat_w = g.motor_w / g.motor_stride, lat_h = g.motor_h / g.motor_stride;
    int max_gap = 64;
    int count[PLUG_MAX_MOTOR_LINES] = {};
    static int dealt[PLUG_MAX_MOTOR_LINES][PLUG_MAX_SPREAD];
    for (int gap = nearest; gap <= max_gap; gap++) {
        for (int ly = 0; ly < lat_h; ly++) {
            for (int lx = 0; lx < lat_w; lx++) {
                int x = g.motor_x + lx * g.motor_stride, y = g.motor_y + ly * g.motor_stride;
                if (tile_gap(along_y ? y : x, cue_lo, cue_hi, dir) != gap) {
                    continue;
                }
                int n = blob_neuron_at(blob, x, y);
                if (!((blob.motor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1)) {
                    continue;
                }
                int line = (lx + ly) % lines;
                if (count[line] < PLUG_MAX_SPREAD) {
                    dealt[line][count[line]++] = n;
                }
            }
        }
    }
    // more than the region holds means all of them (every button gets the same count)
    int want = s.motor_spread;
    for (int line = 0; line < lines; line++) {
        want = count[line] < want ? count[line] : want;
    }
    int used = 0;
    for (int line = 0; line < lines; line++) {
        if (want < 1 || used + want > PLUG_MAX_SPREAD) {
            std::fprintf(stderr, "plug: %d motor cells per button do not fit (%d there)\n", want,
                         count[line]);
            return false;
        }
        plug.spread_first[line] = used;
        for (int i = 0; i < want; i++) {
            int pick = s.motor_spread_even ? (int)((int64_t)i * count[line] / want) : i;
            plug.spread_neuron[used++] = dealt[line][pick];
        }
    }
    plug.spread_first[lines] = used;
    return true;
}

int plug_motor_cells(const Plug& plug, int line, const int** cells) {
    if (plug.settings.motor_spread > 0) {
        *cells = &plug.spread_neuron[plug.spread_first[line]];
        return plug.spread_first[line + 1] - plug.spread_first[line];
    }
    *cells = &plug.motor_neuron[line * PLUG_MAX_GROUP];
    return plug_motor_group(plug);
}
