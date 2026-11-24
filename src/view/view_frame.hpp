#pragma once
// ViewFrame: everything the viewer draws, as plain numbers. The sim thread fills it from a
// Blob (frame_fill, tape_record) and the viewer copies it out under one lock (frame_copy).
// Nothing past this point reads a Blob.

#include <cstdint>
#include <vector>

struct Blob;

constexpr int TAPE_TICKS = 1024;        // spike maps kept for the raster, at most
constexpr int TAPE_WORDS_MAX = 1 << 20; // the tape stays under 8 MB on big sheets
constexpr int LONG_POINTS = 1024;       // points in the long history
constexpr int LONG_STRIDE = 256;        // ticks averaged into one long point
constexpr int HIST_BINS = 32;
constexpr int LAG_MAX = 64; // lags in the self-similarity curve

// the lines over time
enum Line {
    LINE_FIRED,
    LINE_SURPRISE,
    LINE_GROWN,
    LINE_PRUNED,
    LINE_STRENGTHENED,
    LINE_WEAKENED,
    LINE_GOOD,
    LINE_BAD,
    LINE_COUNT,
};
extern const char* const line_names[LINE_COUNT];

struct LinePoint {
    float v[LINE_COUNT] = {};
};

// the last `len` ticks: one spike map and one LinePoint per tick, in a ring
struct Tape {
    int len = 0;                 // rows in the ring
    int row_words = 0;           // words per spike map (the tile count)
    uint64_t ticks = 0;          // rows ever written; the newest is row (ticks - 1) % len
    std::vector<uint64_t> words; // len * row_words
    std::vector<LinePoint> lines;
    // long history: one point per LONG_STRIDE ticks, a ring of LONG_POINTS
    uint64_t long_points = 0;
    std::vector<LinePoint> long_lines;
    LinePoint long_sum; // the point being summed (sim side)
    int long_n = 0;
};

// counts of integer values in HIST_BINS bins of `width`, from `first`; the last bin takes
// everything above
struct Histogram {
    int first = 0;
    int width = 1;
    uint32_t counts[HIST_BINS] = {};
    uint32_t total = 0;
    int low = 0, high = 0; // the smallest and largest value seen
};

struct SectorView {
    float activity = 0; // spikes per tick, smoothed
    float target = 0;   // what regulation aims for
    float surprise = 0; // wrong predictions per tick, smoothed
    int bias = 0;       // added to thresholds in the sector
    int tiles = 0;
};

// one source or target of the selected neuron
struct Link {
    int16_t x = 0, y = 0; // its cell
    uint8_t strength = 0; // 1, 4 or 5
    uint8_t far = 0;
    uint8_t inhibitory = 0;
    uint8_t delay = 1; // ticks the spike takes to arrive (sources only)
};

enum Kind { KIND_EXCITATORY, KIND_INHIBITORY, KIND_SENSOR, KIND_MOTOR };

struct Selected {
    int neuron = -1;
    int x = 0, y = 0, tile = 0, bit = 0, kind = 0, sector = 0;
    int threshold = 0, potential = 0, leak_shift = 0, cooldown = 0, cooldown_len = 0;
    int fan_in = 0, sector_bias = 0;
    int far_tile[2] = {-1, -1};
    int far_delay[2] = {0, 0};
    std::vector<Link> sources, targets;
};

// a task's line on the sheet: the cells of its group, what it is, whether it is on
enum TaskLineKind { TASK_LINE_GOOD, TASK_LINE_BAD, TASK_LINE_SENSE, TASK_LINE_MOTOR };

struct TaskLine {
    int16_t x0 = 0, y0 = 0, x1 = 0, y1 = 0; // cells, x1 and y1 one past the group
    uint8_t kind = TASK_LINE_SENSE;
    uint8_t on = 0;      // sensor: firing this tick. motor: the last answer
    int16_t count = 0;   // motor: spikes so far this step
    char label[24] = {}; // empty for all but the first line of a bitmap or number
};

// one answered step: what would have been right and what the blob did (-1: hold still)
struct TaskTry {
    int8_t cue = 0, right = -1, move = -1, valence = 0;
};

constexpr int TASK_TRIES = 256; // answered steps kept

struct TaskView {
    bool on = false;
    int life = 0; // counts restarts
    char name[32] = {};
    char settings[160] = {};
    bool learning_on = true;
    bool ratio_score = true; // score is a share (0..1); cartpole's is steps per fall
    bool has_cue = false;    // echo family and go/no-go: a try answers a cue
    int window = 0;          // steps per score window
    int ticks_per_step = 0;
    int tick_in_step = 0;
    int64_t step = 0;
    uint64_t answer_tick = 0; // blob tick of the last answer
    int64_t good = 0, bad = 0;
    std::vector<TaskLine> lines;
    TaskTry tries[TASK_TRIES]; // a ring; the newest is (tries_total - 1) % TASK_TRIES
    int64_t tries_total = 0;
    bool twin = true; // the learning-off twin runs
};

// window scores over the whole life: the blob, its learning-off twin, coin flips. Kept
// apart from TaskView because they only grow, so copies take just the new tail.
struct TaskScores {
    int life = 0; // the TaskView's life these belong to; a new one starts them over
    std::vector<float> blob, twin, coin;
};
void task_scores_copy(TaskScores& to, const TaskScores& from);

struct ViewFrame {
    // the shape, fixed at birth
    bool real = false; // from the core, not the fake
    int tiles_w = 0, tiles_h = 0, tile_count = 0, neuron_count = 0;
    int sector_count = 0, sector_tiles = 1;
    uint64_t tick = 0;

    // kinds and sectors, one word or number per tile
    std::vector<uint64_t> inhibitory, sensor, motor;
    std::vector<uint32_t> tile_sector;

    Tape tape;

    // regulation and broadcasts
    std::vector<SectorView> sectors;
    int good_ticks = 0, bad_ticks = 0, surprise_high = 0;

    // histograms over the whole blob, refreshed a few times a second
    uint64_t weight_levels[4] = {}; // possible sources at strength 0, 1, 4, 5
    uint64_t local_links = 0, far_links = 0;
    Histogram threshold, fan_in, length;

    Selected selected;

    TaskView task; // the task plugged in, if any
    TaskScores scores;

    // worked out by the viewer from the tape (frame_derive)
    Histogram rate;                     // spikes per neuron over the tape
    int silent = 0;                     // neurons that never fired on the tape
    int stuck = 0;                      // neurons firing on more than a quarter of the ticks
    float mean_rate = 0;                // spikes per neuron per tick
    float lag_similarity[LAG_MAX] = {}; // overlap of tick t with t - lag, lag 1..LAG_MAX
    // the rhythm of the whole sheet: how the fired count per tick goes with itself `lag`
    // ticks later (correlation, -1..1), and its period: the first clear peak, 0 if none
    float fired_lag[LAG_MAX] = {};
    int period = 0;
    int selected_spikes = 0; // the selected neuron's spikes on the tape
};

// tape.cpp
void tape_init(Tape& tape, int row_words);
void tape_record(Tape& tape, const Blob& blob); // after every tick
void tape_copy_new(Tape& to, const Tape& from); // only the rows `to` does not have
void frame_copy(ViewFrame& to, const ViewFrame& from);
bool tape_fired(const Tape& tape, int age, int neuron); // age 0 is the newest tick

// frame_fill.cpp: everything but the tape. heavy: also the whole-blob histograms
void frame_fill(ViewFrame& frame, const Blob& blob, int selected, bool heavy, bool real);

// frame_derive.cpp
void frame_derive(ViewFrame& frame);
