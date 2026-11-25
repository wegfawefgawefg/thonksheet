// the buffers of shaders/gpu/memory_life.comp. mirrors src/gpu/gpu_memory.hpp and the shared
// parts of src/gpu/gpu_layout.hpp (std430); the binding numbers are the minimal engine's
// (gpu_batch.cpp makes one set layout for both).

const int MAX_LINES = 32;
const int MAX_MOTORS = 16;
const int WORLD_ROWS = 1 + MAX_LINES + MAX_MOTORS;
const int UNITS = 16;
const int FIELD_WORDS = 16;
const int DEPTH = 4;
const int FRAMES = 128;
const int IN_MAX = 64;
const int LEVELS = 64;

struct Params {
    int lives;
    int task;
    int k, cue_offset, delay;
    int first_single;
    int motor_lines;
    int ticks_per_step, rest_ticks, reward_line_ticks;
    int window, steps, max_windows;
    uint attached;
    int uses_reward;
    int pad;
};

struct MemGenome {
    int learning_on, code, tables, table_bits;
    int counter_max, good_step, bad_step, others_up;
    int credit_ticks, trace_ticks, trace_across, across_ticks;
    int explore_max, explore_min, avg_shift, predict;
    int predict_bits, pad0, pad1, pad2;
};

struct MemFrame {
    int64_t first_tick, last_tick;
    int unit, pad;
};

struct MemLife {
    int64_t tick, now_start, frame_count, last_reward_tick;
    uint64_t predict_key;
    uint64_t rng[4];
    uint64_t code_now[FIELD_WORDS];
    uint64_t past_code[DEPTH * FIELD_WORDS];
    uint64_t past_in_hash[DEPTH];
    uint64_t in_now[IN_MAX];
    uint64_t predicted[IN_MAX];
    int past_level[DEPTH];
    int level_now, drive, units, explore_unit;
    int new_cell, frame_due, good_avg, predict_have;
    int tie_key[UNITS + 1];
    int tick_in_step, rest_left, good_left, bad_left;
    int block, motor_lines;
    uint attached;
    int surprise;
    int motor_count[MAX_MOTORS];
    int pad;
};

struct ExchangeIn {
    int valence, run, acted, pad;
};

struct ExchangeOut {
    int motor_count[MAX_MOTORS];
    int surprise, ticks, pad0, pad1;
};

layout(std430, binding = 0) readonly buffer ParamsBlock {
    Params P;
};
// per world: sensor, MAX_LINES lines (0 good, 1 bad, 2 every attached sensor cell), then the
// buttons, tiles + 1 words each
layout(std430, binding = 1) readonly buffer WorldWords {
    uint64_t world_words[];
};
layout(std430, binding = 2) readonly buffer Genomes {
    MemGenome genomes[];
};
layout(std430, binding = 3) coherent buffer Lives {
    MemLife lives[];
};
// counters[(life * COUNTERS) + ((table << table_bits | slot) * UNITS + unit)], -max..max
layout(std430, binding = 4) buffer Counters {
    int8_t counters[];
};
// table t reads bit pos[life * POS + t * table_bits + i] = field << 10 | bit
layout(std430, binding = 5) readonly buffer Pos {
    uint pos[];
};
// each input cell's code row, FIELD_WORDS words (life * IN_WORDS * 64 rows)
layout(std430, binding = 6) readonly buffer Rows {
    uint64_t rows[];
};
// the length codes, LEVELS of FIELD_WORDS words per life
layout(std430, binding = 7) readonly buffer Levels {
    uint64_t levels[];
};
// every table's slot for the last FRAMES frames (life * FRAMES * TABLES)
layout(std430, binding = 8) buffer FrameSlots {
    uint frame_slot[];
};
layout(std430, binding = 9) readonly buffer XIn {
    ExchangeIn xin[];
};
layout(std430, binding = 10) readonly buffer XWords {
    uint64_t xwords[];
};
layout(std430, binding = 11) writeonly buffer XOut {
    ExchangeOut xout[];
};
// per batch: the input tiles (IN_MAX), then the motor cells' neurons (MOTOR_CELLS)
layout(std430, binding = 12) readonly buffer Meta {
    uint meta[];
};
layout(std430, binding = 13) coherent buffer Frames {
    MemFrame frames[];
};
// the next-input store: a tag per entry, then the input words (life * ENTRIES [* IN_WORDS])
layout(std430, binding = 14) coherent buffer PredictTag {
    uint64_t predict_tag[];
};
layout(std430, binding = 15) coherent buffer PredictIn {
    uint64_t predict_in[];
};
// each motor cell's button, -1 none yet (life * MOTOR_CELLS)
layout(std430, binding = 16) coherent buffer UnitOf {
    int unit_of[];
};
