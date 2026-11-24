#include "blob/blob.hpp"
#include "view_frame.hpp"

#include <algorithm>
#include <cstring>

const char* const line_names[LINE_COUNT] = {
    "fired", "surprise", "grown", "pruned", "strengthened", "weakened", "good", "bad",
};

void tape_init(Tape& tape, int row_words) {
    tape = Tape{};
    tape.row_words = row_words;
    tape.len = std::clamp(TAPE_WORDS_MAX / std::max(1, row_words), 64, TAPE_TICKS);
    tape.words.assign(static_cast<size_t>(tape.len) * row_words, 0);
    tape.lines.assign(tape.len, LinePoint{});
    tape.long_lines.assign(LONG_POINTS, LinePoint{});
}

void tape_record(Tape& tape, const Blob& blob) {
    // the newest spike map into the next row
    const int row = static_cast<int>(tape.ticks % static_cast<uint64_t>(tape.len));
    std::memcpy(&tape.words[static_cast<size_t>(row) * tape.row_words], blob.spikes[blob.now],
                sizeof(uint64_t) * tape.row_words);

    // this tick's counts, and the broadcasts that are on
    LinePoint p;
    p.v[LINE_FIRED] = static_cast<float>(blob.stats.fired);
    p.v[LINE_SURPRISE] = static_cast<float>(blob.stats.surprise);
    p.v[LINE_GROWN] = static_cast<float>(blob.stats.grown);
    p.v[LINE_PRUNED] = static_cast<float>(blob.stats.pruned);
    p.v[LINE_STRENGTHENED] = static_cast<float>(blob.stats.strengthened);
    p.v[LINE_WEAKENED] = static_cast<float>(blob.stats.weakened);
    p.v[LINE_GOOD] = blob.good_ticks > 0 ? 1.0f : 0.0f;
    p.v[LINE_BAD] = blob.bad_ticks > 0 ? 1.0f : 0.0f;
    tape.lines[row] = p;
    ++tape.ticks;

    // the long history: a mean over every LONG_STRIDE ticks
    for (int i = 0; i < LINE_COUNT; ++i) {
        tape.long_sum.v[i] += p.v[i];
    }
    if (++tape.long_n == LONG_STRIDE) {
        LinePoint& out = tape.long_lines[tape.long_points % LONG_POINTS];
        for (int i = 0; i < LINE_COUNT; ++i) {
            out.v[i] = tape.long_sum.v[i] / LONG_STRIDE;
        }
        ++tape.long_points;
        tape.long_sum = LinePoint{};
        tape.long_n = 0;
    }
}

void tape_copy_new(Tape& to, const Tape& from) {
    // a different shape starts over
    if (to.len != from.len || to.row_words != from.row_words || to.ticks > from.ticks) {
        to = from;
        return;
    }
    // rows the copy lacks, at most a whole ring
    const uint64_t start =
        std::max(to.ticks, from.ticks > static_cast<uint64_t>(from.len) ? from.ticks - from.len
                                                                        : uint64_t{0});
    for (uint64_t t = start; t < from.ticks; ++t) {
        const size_t row = t % static_cast<uint64_t>(from.len);
        std::memcpy(&to.words[row * from.row_words], &from.words[row * from.row_words],
                    sizeof(uint64_t) * from.row_words);
        to.lines[row] = from.lines[row];
    }
    to.ticks = from.ticks;
    if (to.long_points != from.long_points) {
        to.long_lines = from.long_lines;
        to.long_points = from.long_points;
    }
}

void frame_copy(ViewFrame& to, const ViewFrame& from) {
    to.real = from.real;
    to.tiles_w = from.tiles_w;
    to.tiles_h = from.tiles_h;
    to.tile_count = from.tile_count;
    to.neuron_count = from.neuron_count;
    to.sector_count = from.sector_count;
    to.sector_tiles = from.sector_tiles;
    to.tick = from.tick;
    to.inhibitory = from.inhibitory;
    to.sensor = from.sensor;
    to.motor = from.motor;
    to.tile_sector = from.tile_sector;
    tape_copy_new(to.tape, from.tape);
    to.sectors = from.sectors;
    to.good_ticks = from.good_ticks;
    to.bad_ticks = from.bad_ticks;
    to.surprise_high = from.surprise_high;
    std::memcpy(to.weight_levels, from.weight_levels, sizeof to.weight_levels);
    to.local_links = from.local_links;
    to.far_links = from.far_links;
    to.threshold = from.threshold;
    to.fan_in = from.fan_in;
    to.length = from.length;
    to.selected = from.selected;
    to.task = from.task;
    task_scores_copy(to.scores, from.scores);
}

bool tape_fired(const Tape& tape, int age, int neuron) {
    if (tape.len == 0 || static_cast<uint64_t>(age) >= std::min<uint64_t>(tape.ticks, tape.len)) {
        return false;
    }
    const uint64_t row = (tape.ticks - 1 - age) % static_cast<uint64_t>(tape.len);
    const uint64_t word = tape.words[row * tape.row_words + neuron / TILE_NEURONS];
    return (word >> (neuron % TILE_NEURONS)) & 1;
}
