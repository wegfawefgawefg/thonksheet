#include "gpu/gpu_lives.hpp"

#include "world/plug.hpp"
#include "world/task.hpp"

#include <cstring>

// born lives and the world, laid out for the shader.

bool gpu_genome_supported(const MinimalGenome& mg, std::string& why) {
    // the port is the plain engine: 9 local slots read one tick back, random drops, the plain
    // reward rule. the grown-back pieces stay on the CPU for now.
    if (mg.far_slots != 0) {
        why = "far_slots (not ported)";
    } else if (mg.delay_max > 1) {
        why = "delay_max above 1 (not ported)";
    } else if (mg.ev_on != 0) {
        why = "ev_on, evidence counters (not ported)";
    } else if (mg.drop_mode != 0) {
        why = "drop_mode 1 (not ported)";
    } else if (mg.noise_pair != 0) {
        why = "noise_pair (not ported)";
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

void gpu_life_pack(GpuLifeData& out, const MinimalBlob& m, const RunFlags& flags,
                   uint64_t task_seed) {
    const MinimalGenome& mg = m.mg;
    GpuGenome& g = out.genome;
    g = GpuGenome{};
    g.learning_on = mg.learning_on;
    g.budget_on = mg.budget_on;
    g.inhibit_gain = mg.inhibit_gain;
    g.threshold_cap = mg.threshold_cap;
    g.up_shift = mg.up_shift;
    g.down_shift = mg.down_shift;
    g.noise_shift = mg.noise_shift;
    g.credit_ticks = mg.credit_ticks;
    g.learn_shift = mg.learn_shift;
    g.avg_shift = mg.avg_shift;
    g.grade_mode = mg.grade_mode;
    g.cowire_on = mg.cowire_on;
    g.cowire_shift = mg.cowire_shift;
    g.cowire_needs_input = mg.cowire_needs_input;
    g.noise_sign = mg.noise_sign;

    Task task;
    task_reset(task, flags.task, flags.task_settings, task_seed);
    GpuLife& L = out.life;
    L = GpuLife{};
    L.key = m.rng_key;
    L.tick = m.shell.stats.tick;
    L.world = task.random.state;
    L.now = m.now;
    L.reward_avg = m.reward_avg;
    L.cue = task.cue;
    L.phase = task.phase;
    L.trial = (int32_t)task.trial_lines[1];
    for (int i = 0; i < GPU_MAX_MOTORS; i++) {
        L.button_of[i] = task.button_of[i];
    }

    const size_t tiles = (size_t)m.shell.tile_count, n = tiles * TILE_NEURONS;
    out.mask.assign(m.mask, m.mask + n * MIN_SLOTS);
    out.threshold.assign(m.threshold, m.threshold + n);
    size_t ring = (size_t)m.stride * MIN_HISTORY;
    out.ring.resize(2 * ring);
    std::memcpy(out.ring.data(), m.spikes, ring * 8);
    std::memcpy(out.ring.data() + ring, m.noise, ring * 8);
    out.inhibitory.assign(m.shell.inhibitory, m.shell.inhibitory + tiles + 1);
}
