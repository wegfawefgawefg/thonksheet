#pragma once

#include "blob/blob.hpp"

// readout_opponent (docs/results-2026-11-25-output-gap.md): a one-bit excitatory connection
// can only speak for a button. With readout_opponent 1 the motor cells on every second
// column of the motor lattice count against their button in the vote and learn the other
// way, so a source wired into them is evidence that it is not this button. The buttons are
// dealt out along diagonals of the lattice (plug_spread.cpp), so a column split gives each
// button about half of its cells each way. No state: the column is the neuron's place.
inline bool motor_against(const Blob& blob, int neuron) {
    const Genome& g = blob.genome;
    if (!g.readout_opponent || g.motor_stride <= 0) {
        return false;
    }
    int lx = (blob_neuron_x(blob, neuron) - g.motor_x) / g.motor_stride;
    return (lx & 1) != 0;
}
