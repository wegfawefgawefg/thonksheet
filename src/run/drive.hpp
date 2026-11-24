#pragma once

#include "blob/blob.hpp"
#include "world/plug.hpp"

#include <cstdint>

// Who drives the motor lines in one life, and the four calls that differ by driver. The
// calls are defined in main.cpp, next to the life loop's other choices.
enum class Driver { blob, learn_off, coin, perfect, count };
extern const char* const driver_names[(int)Driver::count];

// the learning-off twin: same genome and seed, nothing changes its wiring
void genome_learning_off(Genome& g);
bool real_blob(Driver driver);
Blob* birth(Driver driver, const Genome& genome, uint64_t seed);
void death(Driver driver, Blob* blob);
void tick(Driver driver, Blob& blob, const Plug& plug, int perfect_move);
void reward(Driver driver, Blob& blob, int valence);
