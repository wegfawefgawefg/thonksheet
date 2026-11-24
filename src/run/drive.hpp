#pragma once

#include "blob/blob.hpp"
#include "run/run_flags.hpp"
#include "world/plug.hpp"

#include <cstdint>

// Who drives the motor lines in one life, and the four calls that differ by driver. The
// calls are defined in main.cpp, next to the life loop's other choices.
// table and table_off are idea 26 (src/table/), picked with --table
enum class Driver { blob, learn_off, coin, perfect, table, table_off, count };
extern const char* const driver_names[(int)Driver::count];

// the learning-off twin: same genome and seed, nothing changes its wiring
void genome_learning_off(Genome& g);
bool real_blob(Driver driver);
bool table_driver(Driver driver);
// an engine that is measured (blob, table or minimal, and their learning-off twins), as
// against the coin and perfect stand-ins
inline bool engine_driver(Driver driver) {
    return real_blob(driver) || table_driver(driver);
}
// which drivers this call runs, from the flags and the engines built in
void drivers_wanted(const RunFlags& flags, bool* want);
Blob* birth(Driver driver, const Genome& genome, uint64_t seed);
void death(Driver driver, Blob* blob);
void tick(Driver driver, Blob& blob, const Plug& plug, int perfect_move);
void reward(Driver driver, Blob& blob, int valence);
