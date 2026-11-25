#pragma once

#include "blob/blob.hpp"

#include <cstddef>
#include <cstdint>

// Idea 123: a memory, not a sheet. Nothing is wired and nothing is rewired. What is sensed
// becomes a long bit code, the code picks a few slots in a store by content, and the slots
// hold small saturating counters, one per button. Good and bad move the counters of the
// slots that were used; the button with the largest sum is pressed.
//
//  code    the sensor cells the world wrote, folded into 1,024 bits: each sensor cell owns a
//          fixed random row of code bits, and the code is the XOR (parity) or the OR
//          (threshold one) of the rows of the cells that are on.
//  events  the input is cut into events where it changes. the store sees the event now, how
//          long it has lasted, and the last `depth` events with their lengths. a length is a
//          level code: 4 buckets per doubling, neighbouring buckets share most bits.
//  store   `tables` tables. table t reads `table_bits` fixed bits of the address: all from
//          the code of now, or some from now and the rest from one other field (how long now
//          has lasted, or one past event's code or length). those bits are the slot. so the
//          active slots are one per table, found with no search, and a table whose bits all
//          fall on what matters is specific to it. (Kanerva's memory with hashed locations,
//          or CMAC tile coding over random bit subsets.)
//  acting  every time the address changes (a frame) the counters of the active slots are
//          summed per button; the largest sum fires its button's motor cells for the frame.
//          a button nobody has pressed yet counts as a sum of 0, so a context where every
//          known button was bad tries a new one. ties and exploring are drawn once per
//          reward, so a whole task step acts the same way.
//  credit  on a reward, the frames since the last reward (at most trace_ticks back) are
//          credited: those of the step just answered with the button the world says was
//          pressed, older ones with the button they drove. good adds good_step to that
//          button's counter in each active slot, bad takes bad_step off.
//  predict a second, small store: (input now, last event, ticks in) to the input on the
//          next tick, a hashed table that remembers the last thing that followed. it fills
//          the shell's predicted map, which is what the sequence task scores.
//
// Buttons are found from the world's motor_acted marks: the cells marked together after a
// press are one button (unit). The engine never reads which task it is on.
//
// It stands behind the same Blob shell as the other engines; the shell is the first member
// of MemoryBlob, so a Blob* from memory_birth converts back.

constexpr int MEM_UNITS = 16;       // buttons the store keeps counters for (the world's most)
constexpr int MEM_FIELD_WORDS = 16; // 1,024 bits per address field
constexpr int MEM_FIELD_BITS = MEM_FIELD_WORDS * 64;
constexpr int MEM_DEPTH_MAX = 4;                     // past events kept, at most
constexpr int MEM_FIELDS = 2 + 2 * MEM_DEPTH_MAX;    // now code, now length, then per event
constexpr int MEM_LEVELS = 64;                       // length buckets (to 2^16 ticks)
constexpr int MEM_FRAMES = 128;                      // frames kept for credit, a power of two
constexpr int MEM_IN_WORDS = 64;                     // sensor tiles read, at most
constexpr int MEM_MOTOR_CELLS = 1024;                // motor cells, at most
constexpr int MEM_TABLE_BITS_MAX = 14;

// plain integers; "--y.name value" overrides one. chances are out of 256.
struct MemoryGenome {
    int learning_on = 1; // 0: counters and the predictor never change
    int code = 0;        // 0 parity (XOR of rows), 1 threshold one (OR of rows)
    int taps = 16;       // code bits a sensor cell's row sets, per 256 sensor cells
    int tables = 256;
    int table_bits = 10; // bits a table reads: its slots are 2^this
    int past_bits = 5;   // of those, how many from the table's other field (none for now-only)
    int depth = 3;       // past events in the address (1..4)
    int now_only = 1;    // share of now-only tables, in 1 + 2 * depth + now_only parts
    int counter_max = 31; // counters run -max..max
    int good_step = 1;
    int bad_step = 2;
    int others_up = 0; // 1: on bad, every other known button's counter goes up 1
    int credit_ticks = 20; // frames this recent get the button the world says was pressed
    int trace_ticks = 100; // frames since the last reward, this far back, are credited
    int explore_max = 32;  // chance to press a random button when nothing has been good
    int explore_min = 2;   // and when everything has
    int avg_shift = 5;     // the good rate follows about 2^this rewards
    int level_span = 2;    // length buckets until two level codes are unrelated
    int fade_shift = 0;    // 0 off; else a touched slot halves its counters with 2^-this
    int predict = 1;       // the next-input store
    int predict_bits = 12; // its entries, 2^this
};

struct MemoryStats {
    uint64_t frames = 0;  // addresses computed
    uint64_t events = 0;  // input changes
    uint64_t credited = 0; // slot updates
    uint64_t explored = 0; // rewards after which a random button was drawn
    uint64_t predict_hits = 0, predict_misses = 0;
};

struct MemoryFrame {
    int64_t first_tick = 0, last_tick = 0;
    int unit = -1; // the button it drove (-1 a new one, -2 none)
};

struct MemoryBlob {
    Blob shell; // stays first
    MemoryGenome mg;
    int64_t tick = 0;

    // input: the sensor tiles (one word each, sensor cells only), and per sensor cell its
    // code row. cell c is bit (c % 64) of input word c / 64 in this numbering.
    int in_words = 0;
    int in_tile[MEM_IN_WORDS] = {};
    uint64_t* rows = nullptr; // in_words * 64 rows of MEM_FIELD_WORDS words
    uint64_t in_now[MEM_IN_WORDS] = {};

    // events: now's code and how long it has lasted, and the past ones (index 0 is newest)
    uint64_t code_now[MEM_FIELD_WORDS] = {};
    int64_t now_start = 0;
    uint64_t past_code[MEM_DEPTH_MAX][MEM_FIELD_WORDS] = {};
    int past_level[MEM_DEPTH_MAX] = {};
    uint64_t past_in_hash[MEM_DEPTH_MAX] = {};
    uint64_t* levels = nullptr; // MEM_LEVELS level codes of MEM_FIELD_WORDS words
    int level_now = -1;

    // the store. table t reads bits pos[t * table_bits + i] = field << 10 | bit.
    // counters[(t << table_bits | slot) * MEM_UNITS + unit]
    int slots = 0;
    uint16_t* pos = nullptr;
    int8_t* counters = nullptr;

    // frames: the slot of every table for the last MEM_FRAMES frames
    uint16_t* frame_slot = nullptr; // MEM_FRAMES * tables
    MemoryFrame frames[MEM_FRAMES] = {};
    int64_t frame_count = 0; // frames made over the life; the newest is (count - 1)
    int32_t sum[MEM_UNITS] = {};
    int drive = -2; // the newest frame's button

    // buttons: motor cells in shell order, the unit of each (-1 none yet), units known
    int motor_cells = 0;
    int motor_neuron[MEM_MOTOR_CELLS] = {};
    int8_t unit_of[MEM_MOTOR_CELLS] = {};
    int units = 0;

    // decided at each reward and kept until the next: tie order, exploring, the new cell
    uint8_t tie_key[MEM_UNITS + 1] = {};
    int explore_unit = -2; // -2 not exploring, -1 a new button, else that button
    int new_cell = -1;     // the unassigned motor cell a new button fires
    int64_t last_reward_tick = -1;
    bool frame_due = false; // a reward came: the next tick makes a new frame
    int32_t good_avg = 0; // x256

    // the next-input store: tag and input words per entry
    uint64_t* predict_tag = nullptr;
    uint64_t* predict_in = nullptr;
    uint64_t predict_key = 0; // key of the previous tick
    bool predict_have = false;

    uint64_t rng[4] = {};
    MemoryStats stats;
};

Blob* memory_birth(const Genome& genome, const MemoryGenome& mg, uint64_t seed);
void memory_free(Blob* blob);
// one tick: read the input, maybe a new frame, fire the chosen button's cells, predict
void memory_tick(Blob& blob);
// good (+1) or bad (-1): credit the frames since the last reward
void memory_reward(Blob& blob, int valence);
// bytes the engine itself allocates (store, rows, predictor; not the shell)
size_t memory_bytes(const MemoryBlob& m);
// "--y.name value" overrides one number
void apply_memory_args(int argc, char** argv, MemoryGenome& mg);

inline MemoryBlob& memory_of(Blob& blob) {
    return *reinterpret_cast<MemoryBlob*>(&blob);
}

// internal: the code of the input now, a frame's slots and sums (memory_code.cpp)
void memory_code_of_input(MemoryBlob& m, uint64_t* out);
int memory_level(int64_t ticks);
void memory_new_frame(MemoryBlob& m);
