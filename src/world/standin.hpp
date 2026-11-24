#pragma once

#include "blob/blob.hpp"
#include "world/plug.hpp"

#include <cstdint>

// A stand-in for the blob: a Blob with only the arrays the plug touches (spike ring, sense
// in, attached, predicted, the sensor and motor masks), and a tick that fakes the motor
// spikes and the sensor predictions. It needs no core, so tasks and the plug can be run and
// checked through the same code paths a real blob uses. blobrun also uses it for the
// coin-flip baseline, which needs no real blob.

enum class StandinPlay {
    coin,    // each motor neuron fires by coin flip (1 in 4) each tick; predictions random
    perfect, // the right motor group fires every tick; predictions are the input itself
};

Blob* standin_birth(const Genome& genome, uint64_t seed);
void standin_free(Blob* blob);
// one fake tick. perfect_move is the task's right move this step (-1 = none).
void standin_tick(Blob& blob, const Plug& plug, StandinPlay play, int perfect_move);
