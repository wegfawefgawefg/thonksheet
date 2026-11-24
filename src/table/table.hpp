#pragma once

#include "blob/blob.hpp"

#include <cstdint>

// Idea 26, table neurons. A neuron reads K input bits, each from one other neuron's spike at
// its own delay, and the K bits address its own table of 2^K tiny saturating counters. It
// fires if the addressed counter is at or above its sector's bar, or by chance (noise).
// This is the K-input lookup cell an FPGA is built from. No popcounts, no floats.
//
// It stands behind the same Blob struct the plug and the tasks use (as world/standin does):
// a TableBlob starts with a Blob shell holding the spike ring, the sensor and motor masks and
// sense_in / sense_attached, so plug_write, blob_fired and blob_sense work unchanged.

constexpr int TABLE_MAX_INPUTS = 8;         // addresses fit a byte
constexpr int TABLE_MAX_BITS = 4;           // bits per counter
constexpr int TABLE_ADDR_HISTORY = HISTORY; // addresses kept per neuron, like the spike ring

// the numbers a table sheet is born from. the sheet size and the sensor and motor regions
// come from the ordinary Genome, so the plug lays lines out exactly as for the blob.
// a "shift" is a chance of 2^-shift. defaults are the setting chosen on seeds 1-4 of echo
// k=2 and k=4 (docs/results-2026-11-24-table-neurons.md).
struct TableGenome {
    int inputs = 8;       // K: bits read per neuron, 1..8
    int counter_bits = 4; // B: bits per table counter, 1, 2 or 4
    int reach = 8;        // local inputs come from cells at most this far away (each axis)
    int far_percent = 5;  // per input, chance in percent that it is far instead of local
    int far_reach_tiles = 3;
    int far_delay_per_tile = 1;
    int local_delay_min = 1, local_delay_max = 4;
    int init_fire_shift = 3; // at birth each counter sits just above the middle with this
                             // chance, otherwise just below

    int noise_shift = 7; // output flipped (either way) with this chance per neuron per tick

    // keeping activity alive.
    // rate_mode 1, per sector: a neuron fires when its counter is at or above its sector's
    // bar (starting at the middle, 2^(B-1)). every 2^bar_period_shift ticks the bar moves one
    // step toward the sector's target rate. tables are never touched.
    // rate_mode 0, per neuron: the bar stays at the middle; when the table says fire, the
    // addressed counter steps down with chance 2^-rate_fire_shift, when it says quiet, up
    // with 2^-rate_quiet_shift (balance near 2^(rate_fire_shift - rate_quiet_shift)).
    // rate_mode 2 is both.
    int rate_mode = 1;
    int sector_tiles = 2;
    int target_per_1024 = 32; // wanted spikes per 1,024 neurons per tick
    int bar_period_shift = 4;
    int rate_fire_shift = 7;
    int rate_quiet_shift = 12;
    int rate_rest_only = 0; // 1: regulation moves only the resting entry (all inputs off)

    // learning on good and bad
    int learning_on = 1;      // 0: no reward learning and no growth (regulation still runs)
    int learn_mode = 0;       // 0 plain: graded neurons' used counters move toward what they did
                              // on good, away on bad. 1 noise is the experiment: only neurons
                              // whose output noise flipped are graded, against average reward.
                              // 2 the same as 1 with the sign turned over
    int grade = 2;            // mode 0, who is graded on a tick: 0 neurons that fired, 1 also
                              // quiet ones with an input on, 2 every neuron, 3 every neuron on
                              // good but only the ones that fired on bad
    int baseline = 0;         // mode 0: 1 grades against the running average of reward
    int reward_shift = 4;     // chance per graded neuron per tick
    int rise_extra_shift = 0; // a move toward fire needs this extra chance as well
    int bad_rise_shift = 2;   // and on bad (a quiet neuron told to fire) this much more
    int recent_ticks = 16;    // ticks back from the reward that get graded, 1..16
    int average_shift = 4;    // modes 1, 2 and baseline: reward average over about 2^this

    // growth by co-activity: per tick a neuron is picked with this chance (0 = off); an input
    // that changed none of its recent answers is swapped for a recently active neighbour
    int grow_shift = 0;

    int report = 0; // 1: print counter and firing-rate spreads every report_ticks, and at death
    int report_ticks = 1 << 16;
};

struct TableStats {
    uint64_t regulated = 0; // counters moved by the rate regulation
    uint64_t learned = 0;   // counters moved by good or bad
    uint64_t grown = 0;     // inputs swapped
};

struct TableBlob {
    Blob shell; // must stay first: the plug and the tasks only ever see this
    TableGenome tg;
    int inputs = 0;           // K
    int bits = 0;             // B
    int words_per_neuron = 0; // 2^K * B / 64, at least 1
    int words_per_plane = 0;  // 2^K / 64, at least 1 (one bit per entry, for the report)

    // input[n * K + j] is (source neuron << 4) | delay. delay 1..15 reads that many ticks
    // back. an input off the sheet points at the always-zero word (source tile_count * 64).
    uint32_t* input = nullptr;
    // table[n * words_per_neuron + entry * B / 64] holds counter `entry` of neuron n at bit
    // (entry * B) % 64, packed, so one load and a shift read it. B is 1, 2 or 4.
    uint64_t* table = nullptr;
    // address[slot * neuron_count + n]: the entry n used on the tick whose spike map is at
    // ring index r, slot = r & (TABLE_ADDR_HISTORY - 1)
    uint8_t* address = nullptr;
    // flips[r][tile]: which outputs noise flipped on the tick of spike map r
    uint64_t* flips[HISTORY] = {};

    // sectors: tile_sector[tile], and per sector its bar, its free (non-sensor) neurons and
    // its spikes in the current bar period
    int sectors_w = 0, sector_count = 0;
    uint32_t* tile_sector = nullptr;
    uint8_t* bar = nullptr;
    uint32_t* sector_free = nullptr;
    uint32_t* sector_fired = nullptr;

    int32_t reward_average = 0; // mode 1, x256
    uint64_t rng[4] = {};
    TableStats stats;

    // report only (null unless tg.report)
    uint32_t* fire_count = nullptr; // per neuron, this report span
    uint64_t* visited = nullptr;    // per table entry, used this report span
    uint64_t report_start = 0;
};

// "--t.name value" on a command line overrides one table genome number
void table_genome_args(int argc, char** argv, TableGenome& tg);

// birth from the ordinary genome (sheet, regions) and the table genome
Blob* table_birth(const Genome& genome, const TableGenome& tg, uint64_t seed);
void table_free(Blob* blob);
void table_tick(Blob& blob);
void table_reward(Blob& blob, int valence);

// the table sheet behind a shell made by table_birth
inline TableBlob& table_of(Blob& blob) {
    return *reinterpret_cast<TableBlob*>(&blob);
}

// one counter's value, and a one-step move toward fire (+1) or quiet (-1), saturating
inline int table_counter(const TableBlob& tb, int neuron, int entry) {
    int at = entry * tb.bits;
    uint64_t w = tb.table[(int64_t)neuron * tb.words_per_neuron + (at >> 6)];
    return (int)((w >> (at & 63)) & ((1u << tb.bits) - 1));
}
inline bool table_step(TableBlob& tb, int neuron, int entry, int direction) {
    int at = entry * tb.bits;
    uint64_t& w = tb.table[(int64_t)neuron * tb.words_per_neuron + (at >> 6)];
    int value = (int)((w >> (at & 63)) & ((1u << tb.bits) - 1));
    int next = value + direction;
    if (next < 0 || next >= (1 << tb.bits)) {
        return false;
    }
    w += direction > 0 ? 1ull << (at & 63) : (uint64_t)0 - (1ull << (at & 63));
    return true;
}
inline bool table_output(const TableBlob& tb, int neuron, int entry) {
    return table_counter(tb, neuron, entry) >= tb.bar[tb.tile_sector[neuron / TILE_NEURONS]];
}

// spreads of counters and firing rates, printed to stdout (table_report.cpp)
void table_report(TableBlob& tb, const char* when);
void table_report_tick(TableBlob& tb);

// bytes of state per neuron, for the speed table
double table_bytes_per_neuron(const TableBlob& tb);
