#pragma once

#include "blob/genome.hpp"

#include <cstdint>

// the modes of blobbench. each takes the arguments after the mode name.
int run_speed(int argc, char** argv);
int run_stage0(int argc, char** argv);
int run_learn_check(int argc, char** argv);

// a genome for a square sheet of `tiles` x `tiles`, sensor and motor regions kept inside it
Genome genome_for_sheet(int tiles);

// "--name value" lookup over argv; returns fallback when absent
int64_t arg_int(int argc, char** argv, const char* name, int64_t fallback);

// seconds since some fixed point
double now_seconds();
