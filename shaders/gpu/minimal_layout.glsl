// the buffers of shaders/gpu/minimal_life.comp. mirrors src/gpu/gpu_layout.hpp (std430).

const int MAX_LINES = 32;
const int MAX_MOTORS = 16;
const int RING = 32;

const int TASK_ECHO = 0;
const int TASK_SHUFFLED_ECHO = 1;
const int TASK_DELAYED_ECHO = 2;
const int TASK_BOTH_NOT_EITHER = 3;
const int TASK_HOST_WORLD = 4;
const int WORLD_ROWS = 1 + MAX_LINES + MAX_MOTORS; // a world table block, in tiles + 1 words

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

struct Genome {
    int learning_on, budget_on, inhibit_gain, threshold_cap;
    int up_shift, down_shift, noise_shift, credit_ticks;
    int learn_shift, avg_shift, grade_mode, cowire_on;
    int cowire_shift, cowire_needs_input, noise_sign, pad;
    int budget, threshold_min, threshold_max, inhibitory_percent; // birth only
    int slots, delay_max, far_percent, far_reach, far_delay_per_tile, delay_percent;
    int ev_on, ev_bits, ev_init, ev_read, ev_spikes, ev_fired_min;
    int ev_quiet, ev_quiet_shift, ev_shift, ev_inh, ev_cowire, pad2;
};

struct Life {
    uint64_t key;
    uint64_t tick;
    uint64_t world;
    int now, reward_avg;
    int cue, phase, trial;
    int tick_in_step, rest_left, good_left, bad_left;
    uint line_on;
    int step, right, chances, windows, done;
    int block;
    int button_of[MAX_MOTORS];
    int motor_count[MAX_MOTORS];
    int motor_lines;
    uint attached; // host worlds: bit 0 good, 1 bad, 31 any task line
    int pad2, pad3;
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
// per batch: sensor[tiles + 1], then line_mask[MAX_LINES][tiles + 1], then
// motor_mask[MAX_MOTORS][tiles + 1]
layout(std430, binding = 1) readonly buffer WorldWords {
    uint64_t world_words[];
};
layout(std430, binding = 2) readonly buffer Genomes {
    Genome genomes[];
};
layout(std430, binding = 3) buffer Lives {
    Life lives[];
};
layout(std430, binding = 4) buffer Masks {
    uint64_t masks[];
};
layout(std430, binding = 5) buffer Thresholds {
    uint8_t thresholds[];
};
// spikes then noise, RING maps of STRIDE words each, per life. written by one thread and
// read by others inside a workgroup, so coherent
layout(std430, binding = 6) coherent buffer Rings {
    uint64_t rings[];
};
layout(std430, binding = 7) buffer Inhibitory { // written by births only
    uint64_t inhibitory[];
};
// per life per window: right answers, chances to answer
layout(std430, binding = 8) buffer Scores {
    uvec2 scores[];
};
// host worlds (one step a dispatch): the reward and the next step's sensor words in, the
// motor counts out. host-visible memory, written and read between dispatches.
layout(std430, binding = 9) readonly buffer XIn {
    ExchangeIn xin[];
};
layout(std430, binding = 10) readonly buffer XWords { // tiles + 1 words per life
    uint64_t xwords[];
};
layout(std430, binding = 11) writeonly buffer XOut {
    ExchangeOut xout[];
};
// evidence counters (EV 1). meta, per batch: the rows' neurons per tile, the row before each
// tile, the motor cells per tile (tiles + 1 words each). planes, per life: row-major, then
// slot, then bit (EV_BITS planes of 64 counters). lines: one byte per row per life (read 1).
layout(std430, binding = 12) readonly buffer EvMeta {
    uint64_t ev_meta[];
};
layout(std430, binding = 13) buffer EvPlanes {
    uint64_t ev_planes[];
};
layout(std430, binding = 14) buffer EvLines {
    uint8_t ev_lines[];
};
// far slots and delays, per life: delays as the masks (one byte per neuron per slot), far
// source tiles ((tile * far slots + j) * 64 + bit)
layout(std430, binding = 15) buffer Delays {
    uint8_t delays[];
};
layout(std430, binding = 16) buffer FarTiles {
    uint far_tiles[];
};
