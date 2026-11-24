#include "world/plug_print.hpp"

// one character per cell of the sensor and motor regions:
// G good, B bad, s on/off line, # bitmap, n number line, 0-9a-z motor line, . unattached.
// a line's cells sit every stride-th cell, as on the sheet

static char motor_char(int line) {
    if (line < 10) {
        return (char)('0' + line);
    }
    return line < 36 ? (char)('a' + line - 10) : '+';
}

static char sensor_char(const Plug& plug, int line) {
    if (!plug.attached[line]) {
        return '.';
    }
    if (line == PLUG_GOOD_LINE) {
        return 'G';
    }
    if (line == PLUG_BAD_LINE) {
        return 'B';
    }
    if (line < plug.first_sheet) {
        return 's';
    }
    return line < plug.first_number ? '#' : 'n';
}

static void print_region(FILE* f, int w, int h, const char* cells) {
    for (int y = 0; y < h; y++) {
        std::fprintf(f, "  ");
        std::fwrite(&cells[y * w], 1, (size_t)w, f);
        std::fprintf(f, "\n");
    }
}

void plug_print(const Plug& plug, const Blob& blob, FILE* f) {
    const Genome& g = blob.genome;
    char cells[64 * 64];

    int side = plug.settings.sensor_side;
    if (g.sensor_w * g.sensor_h <= (int)sizeof(cells)) {
        for (int i = 0; i < g.sensor_w * g.sensor_h; i++) {
            cells[i] = '.';
        }
        for (int line = 0; line < plug.sensor_lines; line++) {
            int x0 = plug.sensor_cell_x[line] - g.sensor_x;
            int y0 = plug.sensor_cell_y[line] - g.sensor_y;
            for (int k = 0; k < side * side; k++) {
                int at = (y0 + k / side * g.sensor_stride) * g.sensor_w + x0 +
                         k % side * g.sensor_stride;
                cells[at] = sensor_char(plug, line);
            }
        }
        std::fprintf(f,
                     "sensor region at (%d,%d), %dx%d cells, %d lines of %d neurons, %d "
                     "attached neurons\n",
                     g.sensor_x, g.sensor_y, g.sensor_w, g.sensor_h, plug.sensor_lines, side * side,
                     plug.sensed_neurons);
        print_region(f, g.sensor_w, g.sensor_h, cells);
    }

    side = plug.settings.motor_side;
    if (g.motor_w * g.motor_h <= (int)sizeof(cells)) {
        for (int i = 0; i < g.motor_w * g.motor_h; i++) {
            cells[i] = '.';
        }
        for (int line = 0; line < plug.motor_lines; line++) {
            int x0 = plug.motor_cell_x[line] - g.motor_x;
            int y0 = plug.motor_cell_y[line] - g.motor_y;
            for (int k = 0; k < side * side; k++) {
                int at =
                    (y0 + k / side * g.motor_stride) * g.motor_w + x0 + k % side * g.motor_stride;
                cells[at] = motor_char(line);
            }
        }
        std::fprintf(f, "motor region at (%d,%d), %dx%d cells, %d lines of %d neurons\n", g.motor_x,
                     g.motor_y, g.motor_w, g.motor_h, plug.motor_lines, side * side);
        print_region(f, g.motor_w, g.motor_h, cells);
    }
}
