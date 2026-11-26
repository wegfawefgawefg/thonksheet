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

void rate_free(Blob& blob);
