#include "world/plug.hpp"

#include <cstdio>

// shelf packing on a grid of line groups: items go left to right, a new shelf starts below
// the tallest item of the current one when the row is full
struct Shelf {
    int grid_w = 0, grid_h = 0;
    int x = 0, y = 0, height = 0;
};

static bool shelf_place(Shelf& shelf, int w, int h, int& gx, int& gy) {
    if (w > shelf.grid_w) {
        return false;
    }
    if (shelf.x + w > shelf.grid_w) {
        shelf.x = 0;
        shelf.y += shelf.height;
        shelf.height = 0;
    }
    if (shelf.y + h > shelf.grid_h) {
        return false;
    }
    gx = shelf.x;
    gy = shelf.y;
    shelf.x += w;
    shelf.height = h > shelf.height ? h : shelf.height;
    return true;
}

static bool in_mask(const uint64_t* mask, int neuron) {
    return (mask[neuron / TILE_NEURONS] >> (neuron % TILE_NEURONS)) & 1;
}

// the side x side neurons of one group, row by row
static bool fill_group(const Blob& blob, const uint64_t* mask, int cell_x, int cell_y, int side,
                       int stride, int* out) {
    for (int dy = 0; dy < side; dy++) {
        for (int dx = 0; dx < side; dx++) {
            int n = blob_neuron_at(blob, cell_x + dx * stride, cell_y + dy * stride);
            if (!in_mask(mask, n)) {
                return false;
            }
            out[dy * side + dx] = n;
        }
    }
    return true;
}

// sensor and motor cells are lattices (every stride-th cell), and a signal fades about 3x
// per tile, so blocks are packed from the corner of their area nearest the other area:
// sensing and acting then sit a tile or two apart
static int near_origin(int grid, int at, int span, bool from_far_end) {
    return from_far_end ? grid - (at + span) : at;
}

// place a block of w x h sensor lines (row-major) starting at line index `first`
static bool place_sensor_block(Plug& plug, const Blob& blob, Shelf& shelf, int first, int w,
                               int h) {
    int gx = 0, gy = 0;
    if (!shelf_place(shelf, w, h, gx, gy)) {
        return false;
    }
    const Genome& g = blob.genome;
    int side = plug.settings.sensor_side;
    int pitch = side * g.sensor_stride;
    gx = near_origin(shelf.grid_w, gx, w, g.motor_x + g.motor_w / 2 > g.sensor_x + g.sensor_w / 2);
    gy = near_origin(shelf.grid_h, gy, h, g.motor_y + g.motor_h / 2 > g.sensor_y + g.sensor_h / 2);
    for (int ly = 0; ly < h; ly++) {
        for (int lx = 0; lx < w; lx++) {
            int line = first + ly * w + lx;
            plug.sensor_cell_x[line] = g.sensor_x + (gx + lx) * pitch;
            plug.sensor_cell_y[line] = g.sensor_y + (gy + ly) * pitch;
            int* group = &plug.sensor_neuron[line * PLUG_MAX_GROUP];
            if (!fill_group(blob, blob.sensor, plug.sensor_cell_x[line], plug.sensor_cell_y[line],
                            side, g.sensor_stride, group)) {
                std::fprintf(stderr, "plug: sensor line %d lands off the sensor neurons\n", line);
                return false;
            }
        }
    }
    return true;
}

static bool lay_sensors(Plug& plug, const Blob& blob, const TaskShape& shape) {
    const Genome& g = blob.genome;
    int side = plug.settings.sensor_side;
    Shelf shelf;
    shelf.grid_w = g.sensor_w / (side * g.sensor_stride);
    shelf.grid_h = g.sensor_h / (side * g.sensor_stride);

    int numbers_total = shape.numbers * shape.number_lines;
    plug.first_single = 2;
    plug.first_sheet = plug.first_single + shape.singles;
    plug.first_number = plug.first_sheet + shape.sheet_w * shape.sheet_h;
    plug.sensor_lines = plug.first_number + numbers_total;
    if (plug.sensor_lines > PLUG_MAX_SENSOR_LINES) {
        std::fprintf(stderr, "plug: %d sensor lines, at most %d\n", plug.sensor_lines,
                     PLUG_MAX_SENSOR_LINES);
        return false;
    }

    // good and bad always take the first two groups
    bool ok = place_sensor_block(plug, blob, shelf, 0, 2, 1);
    for (int i = 0; ok && i < shape.singles; i++) {
        ok = place_sensor_block(plug, blob, shelf, plug.first_single + i, 1, 1);
    }
    if (ok && shape.sheet_w * shape.sheet_h > 0) {
        ok = place_sensor_block(plug, blob, shelf, plug.first_sheet, shape.sheet_w, shape.sheet_h);
    }
    for (int i = 0; ok && i < shape.numbers; i++) {
        ok = place_sensor_block(plug, blob, shelf, plug.first_number + i * shape.number_lines,
                                shape.number_lines, 1);
    }
    if (!ok) {
        std::fprintf(stderr,
                     "plug: task's sensor lines do not fit a %dx%d sensor region at "
                     "group side %d\n",
                     g.sensor_w, g.sensor_h, side);
    }
    return ok;
}

static bool lay_motors(Plug& plug, const Blob& blob, const TaskShape& shape) {
    const Genome& g = blob.genome;
    int side = plug.settings.motor_side;
    Shelf shelf;
    shelf.grid_w = g.motor_w / (side * g.motor_stride);
    shelf.grid_h = g.motor_h / (side * g.motor_stride);
    int pitch = side * g.motor_stride;
    plug.motor_lines = shape.motors;
    if (plug.motor_lines > PLUG_MAX_MOTOR_LINES) {
        std::fprintf(stderr, "plug: %d motor lines, at most %d\n", plug.motor_lines,
                     PLUG_MAX_MOTOR_LINES);
        return false;
    }
    for (int line = 0; line < plug.motor_lines; line++) {
        int gx = 0, gy = 0;
        if (!shelf_place(shelf, 1, 1, gx, gy)) {
            std::fprintf(stderr, "plug: motor lines do not fit the motor region\n");
            return false;
        }
        gx = near_origin(shelf.grid_w, gx, 1, g.sensor_x + g.sensor_w / 2 > g.motor_x + g.motor_w / 2);
        gy = near_origin(shelf.grid_h, gy, 1, g.sensor_y + g.sensor_h / 2 > g.motor_y + g.motor_h / 2);
        plug.motor_cell_x[line] = g.motor_x + gx * pitch;
        plug.motor_cell_y[line] = g.motor_y + gy * pitch;
        int* group = &plug.motor_neuron[line * PLUG_MAX_GROUP];
        if (!fill_group(blob, blob.motor, plug.motor_cell_x[line], plug.motor_cell_y[line], side,
                        g.motor_stride, group)) {
            std::fprintf(stderr, "plug: motor line %d lands off the motor neurons\n", line);
            return false;
        }
    }
    return true;
}

bool plug_attach(Plug& plug, Blob& blob, const TaskShape& shape, const PlugSettings& settings) {
    plug = Plug{};
    plug.settings = settings;
    plug.shape = shape;
    if (settings.sensor_side < 1 || settings.sensor_side > PLUG_MAX_SIDE ||
        settings.motor_side < 1 || settings.motor_side > PLUG_MAX_SIDE) {
        std::fprintf(stderr, "plug: group side must be 1..%d\n", PLUG_MAX_SIDE);
        return false;
    }
    if (!lay_sensors(plug, blob, shape) || !lay_motors(plug, blob, shape)) {
        return false;
    }
    // attach the lines the task uses; the rest of the sensor region stays unattached
    int group = plug_sensor_group(plug);
    for (int line = 0; line < plug.sensor_lines; line++) {
        bool reward_line = line == PLUG_GOOD_LINE || line == PLUG_BAD_LINE;
        plug.attached[line] = reward_line ? shape.uses_reward : 1;
        if (!plug.attached[line]) {
            continue;
        }
        plug.sensed_neurons += group;
        for (int k = 0; k < group; k++) {
            blob_sense(blob, plug.sensor_neuron[line * PLUG_MAX_GROUP + k], false);
        }
    }
    return true;
}

void plug_detach(Plug& plug, Blob& blob) {
    int group = plug_sensor_group(plug);
    for (int line = 0; line < plug.sensor_lines; line++) {
        if (!plug.attached[line]) {
            continue;
        }
        for (int k = 0; k < group; k++) {
            int n = plug.sensor_neuron[line * PLUG_MAX_GROUP + k];
            blob_detach(blob, n);
        }
        plug.attached[line] = 0;
    }
    plug.sensed_neurons = 0;
}

void plug_set_lines(Plug& plug, const TaskSense& sense) {
    const TaskShape& shape = plug.shape;
    for (int i = 0; i < shape.singles; i++) {
        plug.line_on[plug.first_single + i] = sense.singles[i] != 0;
    }
    for (int i = 0; i < shape.sheet_w * shape.sheet_h; i++) {
        plug.line_on[plug.first_sheet + i] = sense.sheet[i] != 0;
    }
    // a number lights its lines from the left as it passes evenly spaced thresholds
    int n = shape.number_lines;
    for (int i = 0; i < shape.numbers; i++) {
        int64_t lo = shape.number_lo[i];
        int64_t span = (int64_t)shape.number_hi[i] - lo;
        for (int k = 0; k < n; k++) {
            int64_t threshold = lo + span * (2 * k + 1) / (2 * n);
            plug.line_on[plug.first_number + i * n + k] = sense.numbers[i] >= threshold;
        }
    }
}

void plug_quiet_lines(Plug& plug) {
    for (int line = plug.first_single; line < plug.sensor_lines; line++) {
        plug.line_on[line] = 0;
    }
}

void plug_write(Plug& plug, Blob& blob) {
    plug.line_on[PLUG_GOOD_LINE] = plug.good_left > 0;
    plug.line_on[PLUG_BAD_LINE] = plug.bad_left > 0;
    plug.good_left -= plug.good_left > 0;
    plug.bad_left -= plug.bad_left > 0;
    int group = plug_sensor_group(plug);
    for (int line = 0; line < plug.sensor_lines; line++) {
        if (!plug.attached[line]) {
            continue;
        }
        bool on = plug.line_on[line];
        for (int k = 0; k < group; k++) {
            blob_sense(blob, plug.sensor_neuron[line * PLUG_MAX_GROUP + k], on);
        }
    }
}

void plug_count(Plug& plug, const Blob& blob) {
    int group = plug_motor_group(plug);
    for (int line = 0; line < plug.motor_lines; line++) {
        const int* neurons = &plug.motor_neuron[line * PLUG_MAX_GROUP];
        for (int k = 0; k < group; k++) {
            plug.motor_count[line] += blob_fired(blob, neurons[k]);
        }
    }
}

void plug_clear_counts(Plug& plug) {
    for (int line = 0; line < plug.motor_lines; line++) {
        plug.motor_count[line] = 0;
    }
}

int plug_choice(Plug& plug) {
    int best = -1;
    int best_count = 0;
    bool tie = false;
    for (int line = 0; line < plug.motor_lines; line++) {
        int c = plug.motor_count[line];
        if (c > best_count) {
            best = line;
            best_count = c;
            tie = false;
        } else if (c == best_count && c > 0) {
            tie = true;
        }
    }
    plug_clear_counts(plug);
    return tie ? -1 : best;
}

void plug_reward(Plug& plug, int valence) {
    if (!plug.shape.uses_reward) {
        return;
    }
    if (valence > 0) {
        plug.good_left = plug.settings.reward_line_ticks;
    }
    if (valence < 0) {
        plug.bad_left = plug.settings.reward_line_ticks;
    }
}
