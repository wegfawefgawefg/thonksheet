#include "probe/input_scramble.hpp"

constexpr int SCRAMBLE_CASES = 256;

static uint64_t split_mix(uint64_t& state) {
    uint64_t z = (state += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

void scramble_start(Scramble& s, int on, uint64_t seed) {
    s = Scramble{};
    s.on = on;
    s.rng = seed * 0x2545f4914f6cdd1dull + 91;
    if (on) {
        s.ring.assign((size_t)SCRAMBLE_CASES * PLUG_MAX_SENSOR_LINES, 0);
    }
}

void scramble_tick(Scramble& s, const TaskTicker& tt, Blob& blob) {
    if (!s.on || tt.resting) {
        return;
    }
    const Plug& plug = tt.plug;
    if (tt.tick_in_step == 0) {
        uint8_t* keep = &s.ring[(size_t)s.next * PLUG_MAX_SENSOR_LINES];
        for (int line = plug.first_single; line < plug.sensor_lines; line++) {
            keep[line] = plug.line_on[line];
        }
        s.next = (s.next + 1) % SCRAMBLE_CASES;
        s.filled += s.filled < SCRAMBLE_CASES;
        s.show = (int)(split_mix(s.rng) % (uint64_t)s.filled);
    }
    const uint8_t* shown = &s.ring[(size_t)s.show * PLUG_MAX_SENSOR_LINES];
    int group = plug_sensor_group(plug);
    for (int line = plug.first_single; line < plug.sensor_lines; line++) {
        if (!plug.attached[line]) {
            continue;
        }
        for (int k = 0; k < group; k++) {
            blob_sense(blob, plug.sensor_neuron[line * PLUG_MAX_GROUP + k], shown[line] != 0);
        }
    }
}
