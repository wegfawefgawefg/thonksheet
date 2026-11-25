#include "gpu/gpu_lives.hpp"

#include "world/plug.hpp"
#include "world/task.hpp"

#include <cstring>

// born lives and the world, laid out for the shader.

bool gpu_genome_supported(const MinimalGenome& mg, std::string& why) {
    // the plain engine everywhere; evidence counters, far slots and delays with host worlds
    // (gpu_needs_host). drop_mode 1 and noise_pair stay on the CPU.
    if (mg.drop_mode != 0) {
        why = "drop_mode 1 (not ported)";
    } else if (mg.noise_pair != 0) {
        why = "noise_pair (not ported)";
    } else if (mg.readout != 0) {
        why = "the core's readout (m.readout, idea 65; not ported)";
    } else if (mg.inner_reward != 1) {
        why = "inner_reward 0 (the core's switch; not ported)";
    } else if (mg.rng_mode != 1) {
        why = "rng_mode must be 1 (keyed words)";
    } else {
        return true;
    }
    return false;
}

static GpuTaskKind kind_of(TaskKind kind, bool& ok) {
    ok = true;
    switch (kind) {
    case TaskKind::echo:
        return GPU_ECHO;
    case TaskKind::shuffled_echo:
        return GPU_SHUFFLED_ECHO;
    case TaskKind::delayed_echo:
        return GPU_DELAYED_ECHO;
    case TaskKind::both_not_either:
        return GPU_BOTH_NOT_EITHER;
    default:
        ok = false;
        return GPU_ECHO;
    }
}

bool gpu_world_in_shader(TaskKind kind) {
    bool known = false;
    kind_of(kind, known);
    return known;
}

static void set_bit(std::vector<uint64_t>& words, size_t first, int neuron) {
    words[first + (size_t)(neuron / TILE_NEURONS)] |= 1ull << (neuron % TILE_NEURONS);
}

bool gpu_world_build(GpuWorld& w, Blob& blob, const RunFlags& flags, std::string& why) {
    bool known = false;
    GpuParams p{};
    p.task = kind_of(flags.task, known);
    if (!known) {
        why = std::string("task ") + task_name(flags.task) + " is not on the GPU yet";
        return false;
    }
    // the task's own numbers after its reset clamps them
    Task task;
    task_reset(task, flags.task, flags.task_settings, flags.task_seed);
    Plug plug;
    if (!plug_attach(plug, blob, task_shape(task), flags.plug)) {
        why = "the plug does not fit";
        return false;
    }
    if (plug.sensor_lines > GPU_MAX_LINES || plug.motor_lines > GPU_MAX_MOTORS) {
        why = "more than 32 sensor lines or 16 buttons";
        plug_detach(plug, blob);
        return false;
    }
    p.k = task.settings.k;
    p.cue_offset = flags.task == TaskKind::both_not_either ? 0 : task.settings.cue_offset;
    p.delay = task.settings.delay;
    p.first_single = plug.first_single;
    p.motor_lines = plug.motor_lines;
    p.ticks_per_step = flags.plug.ticks_per_step;
    p.rest_ticks = flags.plug.rest_ticks;
    p.reward_line_ticks = flags.plug.reward_line_ticks;
    p.window = flags.window;
    p.steps = (int32_t)flags.steps;
    p.max_windows = (int32_t)(flags.steps / flags.window);
    p.uses_reward = plug.shape.uses_reward ? 1 : 0;
    for (int line = 0; line < plug.sensor_lines; line++) {
        p.attached |= plug.attached[line] ? 1u << line : 0u;
    }

    // sensor, then each line's sensor cells, then each button's motor cells
    const int tiles1 = blob.tile_count + 1;
    w.words.assign((size_t)(1 + GPU_MAX_LINES + GPU_MAX_MOTORS) * tiles1, 0);
    std::memcpy(w.words.data(), blob.sensor, sizeof(uint64_t) * (size_t)tiles1);
    int group = plug_sensor_group(plug);
    for (int line = 0; line < plug.sensor_lines; line++) {
        for (int k = 0; k < group; k++) {
            set_bit(w.words, (size_t)(1 + line) * tiles1,
                    plug.sensor_neuron[line * PLUG_MAX_GROUP + k]);
        }
    }
    for (int line = 0; line < plug.motor_lines; line++) {
        const int* cells = nullptr;
        int count = plug_motor_cells(plug, line, &cells);
        for (int k = 0; k < count; k++) {
            set_bit(w.words, (size_t)(1 + GPU_MAX_LINES + line) * tiles1, cells[k]);
        }
    }
    plug_detach(plug, blob);

    const MinimalBlob& m = minimal_of(blob);
    w.params = p;
    w.shape.tiles = blob.tile_count;
    w.shape.tiles_w = blob.tiles_w;
    w.shape.stride = m.stride;
    w.shape.max_windows = p.max_windows;
    return true;
}

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

bool gpu_needs_host(const MinimalGenome& mg) {
    return mg.ev_on != 0 || mg.far_slots != 0 || mg.delay_max > 1;
}

// the numbers the shaders read, clamped as minimal_birth's sanitize() clamps them. change
// both together.
static void genome_numbers(GpuGenome& g, const MinimalGenome& mg, const Genome& sheet) {
    g = GpuGenome{};
    g.slots = MIN_SLOTS + clamp_int(mg.far_slots, 0, MIN_FAR_MAX);
    g.delay_max = clamp_int(mg.delay_max, 1, MIN_DELAY_MAX);
    g.far_percent = clamp_int(mg.far_percent, 0, 100);
    g.far_reach = clamp_int(mg.far_reach, 1, 64);
    g.far_delay_per_tile = clamp_int(mg.far_delay_per_tile, 0, 16);
    g.delay_percent = clamp_int(mg.delay_percent, 0, 100);
    g.ev_on = clamp_int(mg.ev_on, 0, 2);
    g.ev_bits = clamp_int(mg.ev_bits, 2, 8);
    g.ev_init = clamp_int(mg.ev_init, 0, 255);
    g.ev_read = clamp_int(mg.ev_read, 0, 2);
    g.ev_spikes = mg.ev_spikes;
    g.ev_fired_min = clamp_int(mg.ev_fired_min, 1, 8);
    g.ev_quiet = mg.ev_quiet;
    g.ev_quiet_shift = clamp_int(mg.ev_quiet_shift, 0, 30);
    g.ev_shift = clamp_int(mg.ev_shift, 0, 30);
    g.ev_inh = mg.ev_inh;
    g.ev_cowire = mg.ev_cowire;
    g.learning_on = sheet.learning_on && mg.learning_on;
    g.budget_on = mg.budget_on;
    g.inhibit_gain = clamp_int(mg.inhibit_gain, 0, 64);
    g.threshold_min = clamp_int(mg.threshold_min, 1, 255);
    g.threshold_max = clamp_int(mg.threshold_max, g.threshold_min, 255);
    g.threshold_cap = clamp_int(mg.threshold_cap, g.threshold_max, 255);
    g.up_shift = mg.up_shift;
    g.down_shift = mg.down_shift;
    g.noise_shift = mg.noise_shift;
    g.credit_ticks = clamp_int(mg.credit_ticks, 1, MIN_HISTORY - 1 - g.delay_max);
    g.learn_shift = mg.learn_shift;
    g.avg_shift = clamp_int(mg.avg_shift, 0, 16);
    g.grade_mode = mg.grade_mode;
    g.cowire_on = mg.cowire_on;
    g.cowire_shift = mg.cowire_shift;
    g.cowire_needs_input = mg.cowire_needs_input;
    g.noise_sign = mg.noise_sign;
    g.budget = clamp_int(mg.budget, 1, MIN_SLOTS * TILE_NEURONS - 1);
    g.inhibitory_percent = sheet.inhibitory_percent;
}

// a newborn life's clock, reward average and task (the task's part only for the tasks in
// the shader; a host world's task is on the host)
static void life_start(GpuLife& L, uint64_t seed, const RunFlags& flags) {
    L = GpuLife{};
    L.key = seed ^ 0x4d696e696d616c00ull; // minimal_birth's rng_key
    bool known = false;
    kind_of(flags.task, known);
    if (!known) {
        return;
    }
    Task task;
    task_reset(task, flags.task, flags.task_settings, seed);
    L.world = task.random.state;
    L.cue = task.cue;
    L.phase = task.phase;
    L.trial = (int32_t)task.trial_lines[1];
    for (int i = 0; i < GPU_MAX_MOTORS; i++) {
        L.button_of[i] = task.button_of[i];
    }
}

void gpu_life_unborn(GpuLifeData& out, const GpuLifeSpec& spec, const RunFlags& flags) {
    genome_numbers(out.genome, spec.mg, spec.genome);
    life_start(out.life, spec.seed, flags);
    out.mask.clear();
}

void gpu_life_pack(GpuLifeData& out, const MinimalBlob& m, const RunFlags& flags,
                   uint64_t task_seed) {
    genome_numbers(out.genome, m.mg, m.shell.genome);
    life_start(out.life, task_seed, flags);
    out.life.key = m.rng_key;
    const size_t tiles = (size_t)m.shell.tile_count, n = tiles * TILE_NEURONS;
    out.mask.assign(m.mask, m.mask + n * MIN_SLOTS);
    out.threshold.assign(m.threshold, m.threshold + n);
    out.inhibitory.assign(m.shell.inhibitory, m.shell.inhibitory + tiles + 1);
}
