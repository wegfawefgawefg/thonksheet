#pragma once

#include "blob/blob.hpp"

// idea 175 (regulate_rate.cpp, docs/results-2026-11-26-rate-control.md): rate controls that do
// not fight the rest. each runs only while its genome number is on; none draws a random number.

// the dead band: each cell counts its spikes over a window of 2^rate_band_shift ticks (tiles
// staggered); at the end its threshold steps only if the count is outside the band
void regulate_band(Blob& blob);

// inhibition in proportion: the sector's bias is a slow part (the usual one-step move, on the
// ticks `step` allows) plus sector_gain threshold steps per target's worth of activity over the
// target. `act` is the sector's smoothed activity, x256
void regulate_sector_gain(Blob& blob, int sector, int act, int step);

// idea 184 (docs/results-2026-11-26-motor-apart.md): motor cells regulated apart from the
// rest, by motor_apart.
// 1 and up: on the first tick every sector's target is made again without its motor cells
// (the caller then leaves them out of the count too)
void motor_apart_start(Blob& blob);

// 3: the sector's second bias, for its motor cells: their smoothed count and one step toward
// their own target, as the sector's bias moves (`spikes`: the motor cells' this tick)
void regulate_motor_bias(Blob& blob, int sector, int spikes, int step);

// 2 and 3, from the fire step: tile t's motor cells are compared against their own bias (none,
// or the sector's second one) in place of the sector's. `would` is the compare under the
// sector's `bias`, on this tick's potentials. `margin`: null, or with the neuron dynamics on
// each cell's potential less its bar under `bias`, moved here by the difference
uint64_t motor_apart_compare(const Blob& blob, int t, uint64_t would, int bias, int32_t* margin);

// motor_winners, from the fire step in place of the tile's one cut: the tile's winners are
// picked among the spikes of cells that are not motor cells; the motor cells keep every
// spike (-1) or the motor_winners most driven of theirs
uint64_t motor_apart_winners(const Blob& blob, int t, uint64_t spikes, const int32_t* margin);

// the motor cells' own bias in a sector (3), for probes; 0 while there is none
int motor_apart_bias(const Blob& blob, int sector);

void rate_free(Blob& blob);
