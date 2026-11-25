#pragma once

#include "blob/blob.hpp"

// shaped wiring at birth (ideas 19-21, 77-80, 83), birth_shape.cpp. it only decides which
// bits of the masks are set: the slots, words and tick stay what they are.

// any of the wire_* numbers set
bool wire_shape_on(const Genome& g);
// fills local and far slots, delays and fan_in, in place of birth.cpp's wire()
void wire_shaped(Blob& blob);

// remove random connections until the neuron has at most cap (birth.cpp)
void enforce_fan_in(Blob& blob, int n, int cap);

// layer groups inside a tile (wire_layers). sensor cells are in, motor cells out; the others
// by bit position: x and y both even in, both odd out, the rest middle (16, 32, 16 cells).
enum { GROUP_IN = 0, GROUP_MID = 1, GROUP_OUT = 2 };
inline int wire_group(const Blob& blob, int n) {
    int t = n / TILE_NEURONS, b = n % TILE_NEURONS;
    if ((blob.sensor[t] >> b) & 1) {
        return GROUP_IN;
    }
    if ((blob.motor[t] >> b) & 1) {
        return GROUP_OUT;
    }
    int x = b % 8, y = b / 8;
    if ((x & 1) == 0 && (y & 1) == 0) {
        return GROUP_IN;
    }
    if ((x & 1) && (y & 1)) {
        return GROUP_OUT;
    }
    return GROUP_MID;
}

// the favoured flows: in feeds middle, middle feeds middle and out (inside a tile), out
// feeds the next tile's in (local slots) and far (far slots)
inline bool wire_layer_favoured(int from, int to, bool same_tile) {
    if (same_tile) {
        return (from == GROUP_IN && to == GROUP_MID) || (from == GROUP_MID && to == GROUP_MID) ||
               (from == GROUP_MID && to == GROUP_OUT);
    }
    return from == GROUP_OUT && to == GROUP_IN;
}
