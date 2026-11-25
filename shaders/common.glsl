// Shared by every blobview shader: the push constants (mirrored by ViewPush in
// src/view/renderer/vulkan_internal.hpp), the buffers, the palette, and reading one bit of a spike word.
// A 64-bit word is a uvec2: x holds bits 0..31, y bits 32..63.

layout(push_constant) uniform Push {
    vec4 rect;       // where this view is drawn, in framebuffer pixels
    vec2 extent;     // the framebuffer's size
    vec2 origin;     // sheet: pixel of cell (0, 0). raster: first row, rows per pixel
    float cell_px;   // sheet: pixels per cell
    float fade;      // sheet: ticks to fade. raster: target rate. recurrence: usual overlap
    uint cells_w;
    uint cells_h;
    uint tiles_w;
    uint tile_count;
    uint tape_len;   // rows in the tape ring
    uint newest_row; // the ring row of the newest tick
    uint rows_valid; // rows written so far, at most tape_len
    uint flags;      // sheet: 1 tile lines, 2 sector lines, 4 the overlay
    uint sector_tiles;
    uint mode;       // raster: 0 a row per neuron, 1 a row per tile. recurrence: ticks across
    int selected;    // selected neuron, or -1
    uint word_step;  // recurrence: every word_step-th word is compared
} pc;

layout(std430, set = 0, binding = 0) readonly buffer TapeWords {
    uvec2 tape[];
};
// per tile: inhibitory, sensor, motor
layout(std430, set = 0, binding = 1) readonly buffer KindWords {
    uvec2 kinds[];
};
// per cell, row by row: a colour laid over the sheet (src/view/renderer/overlay.hpp), read
// only when flags has 4. bytes: red, green, blue, then bit 7 "replaces the kind colour" and
// bits 0..6 "this much laid over"
layout(std430, set = 0, binding = 3) readonly buffer OverlayWords {
    uint overlay[];
};

vec3 overlay_rgb(uint word) {
    return vec3(float(word & 255u), float((word >> 8) & 255u), float((word >> 16) & 255u)) /
           255.0;
}

const vec3 BACKGROUND = vec3(0.043, 0.051, 0.071); // #0b0d12
const vec3 EXCITATORY = vec3(0.949, 0.722, 0.294); // #f2b84b
const vec3 INHIBITORY = vec3(0.290, 0.639, 1.000); // #4aa3ff
const vec3 SENSOR = vec3(0.373, 0.827, 0.553);     // #5fd38d
const vec3 MOTOR = vec3(1.000, 0.420, 0.604);      // #ff6b9a
const vec3 FAR = vec3(0.690, 0.549, 1.000);        // #b08cff

bool bit_set(uvec2 word, uint b) {
    uint half_word = b < 32u ? word.x : word.y;
    return ((half_word >> (b & 31u)) & 1u) != 0u;
}

uint ones(uvec2 word) {
    return uint(bitCount(word.x) + bitCount(word.y));
}

// the ring row `age` ticks before the newest
uint tape_row(uint age) {
    return (pc.newest_row + pc.tape_len - (age % pc.tape_len)) % pc.tape_len;
}

uvec2 spikes_at(uint age, uint tile) {
    return tape[tape_row(age) * pc.tile_count + tile];
}

// a neuron's colour by kind: sensor, then motor, then inhibitory, else excitatory
vec3 kind_color(uint tile, uint b) {
    if (bit_set(kinds[tile * 3u + 1u], b)) {
        return SENSOR;
    }
    if (bit_set(kinds[tile * 3u + 2u], b)) {
        return MOTOR;
    }
    if (bit_set(kinds[tile * 3u], b)) {
        return INHIBITORY;
    }
    return EXCITATORY;
}
