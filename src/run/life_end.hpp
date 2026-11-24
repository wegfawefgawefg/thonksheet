#pragma once

#include "blob/blob.hpp"

#include <cstdint>
#include <cstdio>
#include <vector>

// Room and aliveness at the end of a life (docs/bench.md). Every number that needs an array
// a given engine does not have stays at -1 ("not known") instead of guessing.
//
// room:  connections per neuron; the share of connections at strength 1, 4 and 5; the share
//        of neurons at the connection cap (fan_in_max); the share with the threshold at its
//        cap (threshold_cap). read off the weight planes after the life's last step.
// alive: spikes per 1,024 neurons per tick against the genome's target, the share of silent
//        neurons (no spike) and of stuck ones (a spike on more than a quarter of the ticks),
//        over the life's last alive_ticks ticks with its task still on; sensor cells are left
//        out of silent and stuck since the world drives them. then the rhythm: everything
//        unplugged for rhythm_ticks more ticks, and the lag at which the count of neurons
//        fired per tick best repeats (probe/rhythm_find.hpp).

struct LifeEnd {
    bool measured = false;
    double connections_per_neuron = -1;
    double share_strength[3] = {-1, -1, -1}; // strengths 1, 4, 5
    double share_at_cap = -1;
    double share_threshold_cap = -1;
    double rate_per_1024 = -1, target_per_1024 = -1;
    double share_silent = -1, share_stuck = -1;
    int rhythm_period = -1;
    double rhythm_strength = -1;
};

// spikes per neuron over the life's last ticks
struct AliveCount {
    std::vector<uint32_t> spikes;
    int64_t ticks = 0;
    int64_t fired = 0;
};

void alive_count_tick(AliveCount& count, const Blob& blob);
// neurons that fired on the latest tick
int fired_now(const Blob& blob);

void life_end_room(LifeEnd& end, const Blob& blob);
void life_end_alive(LifeEnd& end, const AliveCount& count, const Blob& blob);
void life_end_rhythm(LifeEnd& end, const std::vector<double>& fired_per_tick);

void life_end_print(const LifeEnd& end, const char* driver, FILE* f);
// one "driver,key,value" line per number
void life_end_write(const LifeEnd& end, const char* driver, FILE* f);
