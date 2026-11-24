#pragma once

#include "run/drive.hpp"
#include "run/run_flags.hpp"

#include <cstdio>
#include <string>

// What blobrun does after parsing: one of the life protocols in run_flags.hpp. Each prints a
// table, writes its csv, and writes <csv stem>-sum.csv with one "driver,key,value" line per
// number, which is what scripts/suite.sh reads.

int protocol_run(const RunFlags& flags);

// --frozen-readout: the genome's learn_mode 1, when the core has it (exits if not)
void genome_readout_only(Genome& genome);

// <csv stem>-sum.csv, opened with its header line
FILE* summary_open(const std::string& csv_path);

// in life_measures.cpp: blobrun --reach and --speed
int protocol_reach(const RunFlags& flags);
int protocol_speed(const RunFlags& flags);
