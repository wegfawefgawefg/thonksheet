#include "gpu/gpu_host.hpp"

#include "blob/genome.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <sstream>

// host worlds: the worlds of a batch, their tables, and the host's half of an exchange.

static std::vector<std::string> split(const std::string& text, char by) {
    std::vector<std::string> parts;
    std::stringstream in(text);
    std::string part;
    while (std::getline(in, part, by)) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

// blobrun's parser over the batch's flags and then this world's own
static bool parse_world(const std::vector<std::string>& args, RunFlags& out) {
    std::vector<std::string> copy = args;
    std::vector<char*> argv;
    for (std::string& a : copy) {
        argv.push_back(a.data());
    }
    out = RunFlags{};
    apply_genome_args((int)argv.size(), argv.data(), out.genome);
    return run_flags_parse((int)argv.size(), argv.data(), out);
}

bool host_worlds_parse(const std::string& tasks, const std::vector<std::string>& run_args,
                       const RunFlags& base, std::vector<HostWorld>& worlds) {
    worlds.clear();
    if (tasks.empty()) {
        HostWorld w;
        w.name = task_name(base.task);
        w.flags = base;
        worlds.push_back(w);
        if (babble_on(base.babble)) {
            std::fprintf(stderr, "blobgpu: babbling and balance (ideas 60, 61) are not in the "
                                 "host worlds\n");
            return false;
        }
        return true;
    }
    for (const std::string& entry : split(tasks, ',')) {
        std::vector<std::string> parts = split(entry, ':');
        std::vector<std::string> args = run_args;
        args.push_back("--task");
        args.push_back(parts[0]);
        for (size_t i = 1; i < parts.size(); i++) {
            size_t eq = parts[i].find('=');
            if (eq == std::string::npos) {
                std::fprintf(stderr, "blobgpu: --tasks: '%s' is not key=value\n", parts[i].c_str());
                return false;
            }
            args.push_back("--" + parts[i].substr(0, eq));
            for (const std::string& v : split(parts[i].substr(eq + 1), '/')) {
                args.push_back(v);
            }
        }
        HostWorld w;
        w.name = entry;
        if (!parse_world(args, w.flags)) {
            return false;
        }
        if (babble_on(w.flags.babble)) {
            std::fprintf(stderr,
                         "blobgpu: %s: babbling and balance (ideas 60, 61) are not in "
                         "the host worlds\n",
                         entry.c_str());
            return false;
        }
        w.flags.genome = base.genome; // one sheet for the batch
        const PlugSettings &a = w.flags.plug, &b = base.plug;
        if (a.ticks_per_step != b.ticks_per_step || a.rest_ticks != b.rest_ticks ||
            a.reward_line_ticks != b.reward_line_ticks) {
            std::fprintf(stderr,
                         "blobgpu: --tasks: %s changes ticks, rest or reward ticks; "
                         "a batch shares them\n",
                         entry.c_str());
            return false;
        }
        worlds.push_back(w);
    }
    return !worlds.empty();
}

static void set_bit(uint64_t* words, int neuron) {
    words[neuron / TILE_NEURONS] |= 1ull << (neuron % TILE_NEURONS);
}

bool host_worlds_build(std::vector<HostWorld>& worlds, Blob& born, std::vector<uint64_t>& words,
                       std::string& why) {
    const size_t tiles1 = (size_t)born.tile_count + 1;
    const size_t block = (size_t)GPU_WORLD_ROWS * tiles1;
    words.assign(block * worlds.size(), 0);
    for (size_t i = 0; i < worlds.size(); i++) {
        HostWorld& w = worlds[i];
        Task task;
        task_reset(task, w.flags.task, w.flags.task_settings, 1);
        if (!plug_attach(w.plug, born, task_shape(task), w.flags.plug)) {
            why = "world " + w.name + ": the plug does not fit";
            return false;
        }
        if (w.plug.motor_lines > GPU_MAX_MOTORS) {
            why = "world " + w.name + ": more than 16 buttons";
            return false;
        }
        uint64_t* out = words.data() + block * i;
        std::memcpy(out, born.sensor, sizeof(uint64_t) * tiles1);
        int group = plug_sensor_group(w.plug);
        for (int line = 0; line < w.plug.sensor_lines; line++) {
            bool reward_line = line == PLUG_GOOD_LINE || line == PLUG_BAD_LINE;
            if (w.plug.attached[line]) {
                w.attached |= reward_line ? 1u << line : 1u << 31;
                // line 2's row: every attached sensor cell (the memory engine's sense_attached)
                for (int k = 0; k < group; k++) {
                    set_bit(out + 3 * tiles1, w.plug.sensor_neuron[line * PLUG_MAX_GROUP + k]);
                }
            }
            for (int k = 0; reward_line && k < group; k++) {
                set_bit(out + (1 + line) * tiles1, w.plug.sensor_neuron[line * PLUG_MAX_GROUP + k]);
            }
        }
        w.motor_lines = w.plug.motor_lines;
        for (int line = 0; line < w.plug.motor_lines; line++) {
            const int* cells = nullptr;
            int count = plug_motor_cells(w.plug, line, &cells);
            for (int k = 0; k < count; k++) {
                set_bit(out + (1 + GPU_MAX_LINES + line) * tiles1, cells[k]);
            }
        }
        Plug off = w.plug; // the world keeps its attached copy
        plug_detach(off, born);
    }
    return true;
}

void host_life_start(HostLife& h, int world, uint64_t seed, const HostWorld& w) {
    h = HostLife{};
    h.world = world;
    task_reset(h.task, w.flags.task, w.flags.task_settings, seed);
    TaskShape a = task_shape(h.task);
    const TaskShape& b = w.plug.shape;
    if (a.singles != b.singles || a.sheet_w != b.sheet_w || a.sheet_h != b.sheet_h ||
        a.numbers != b.numbers || a.number_lines != b.number_lines || a.motors != b.motors) {
        std::fprintf(stderr, "blobgpu: world %s has another shape for seed %llu\n", w.name.c_str(),
                     (unsigned long long)seed);
        std::exit(1);
    }
}

void host_scratch_open(HostScratch& s, const std::vector<HostWorld>& worlds, int tiles) {
    s.plugs.clear();
    for (const HostWorld& w : worlds) {
        s.plugs.push_back(w.plug);
    }
    s.sense_in.assign((size_t)tiles + 1, 0);
    s.attached.assign((size_t)tiles + 1, 0);
    s.predicted.assign((size_t)tiles + 1, 0);
    s.shell = Blob{};
    s.shell.tile_count = tiles;
    s.shell.sense_in = s.sense_in.data();
    s.shell.sense_attached = s.attached.data();
    s.shell.predicted = s.predicted.data(); // the minimal engine never predicts: all off
}

static bool fill_in_allowed = true;

void host_fill_in_allowed(bool allowed) {
    fill_in_allowed = allowed;
}

static double since(std::chrono::steady_clock::time_point& t) {
    auto now = std::chrono::steady_clock::now();
    double s = std::chrono::duration<double>(now - t).count();
    t = now;
    return s;
}

void host_exchange(const std::vector<HostWorld>& worlds, std::vector<HostLife>& host,
                   GpuGroup& group, int from, int to, bool answer, int tiles, HostScratch& s,
                   HostTimes& t) {
    const size_t n = (size_t)(to - from), tiles1 = (size_t)tiles + 1;
    auto clock = std::chrono::steady_clock::now();
    // the counts, out of the mapped buffer at once
    s.xout.resize(n);
    std::memcpy(s.xout.data(), (const GpuExchangeOut*)group.xout.mapped + from,
                sizeof(GpuExchangeOut) * n);
    t.exchange += since(clock);

    s.xin.resize(n);
    s.words.resize(n * tiles1);
    for (size_t i = 0; i < n; i++) {
        HostLife& h = host[(size_t)group.first + (size_t)from + i];
        GpuExchangeIn& x = s.xin[i];
        x = GpuExchangeIn{0, 0, -1, 0};
        if (h.done) {
            continue;
        }
        const HostWorld& w = worlds[(size_t)h.world];
        Plug& p = s.plugs[(size_t)h.world];
        if (answer) {
            // ticker_after_tick on the step's last tick, then the life loop's window
            for (int line = 0; line < p.motor_lines; line++) {
                p.motor_count[line] = s.xout[i].motor_count[line];
            }
            TaskStepIn in;
            in.move = plug_choice(p);
            in.surprise = s.xout[i].surprise;
            in.sensed_neurons = p.sensed_neurons;
            // fill-in: the ticker counts the hidden lines' predictions after every tick of
            // the step; the minimal engine's predictions are all off
            for (int k = 0; h.hidden && k < w.flags.plug.ticks_per_step; k++) {
                plug_count_hidden(p, s.shell, h.hidden, h.hidden_on, in);
            }
            h.move = in.move;
            h.valence = task_step(h.task, in);
            h.step++;
            if (h.step % w.flags.window == 0) {
                h.scores.push_back(task_window_score(h.task));
            }
            h.done = h.step >= w.flags.steps;
            x.valence = h.valence;
            x.acted = h.valence != 0 ? h.move : -1;
        }
        x.run = h.done ? 0 : 1;
        if (h.done) {
            continue;
        }
        // the next step's first tick: what the task shows, as the plug writes it (good and
        // bad are the GPU's, so they are off here)
        task_sense(h.task, s.sense);
        h.hidden = s.sense.hidden;
        h.hidden_on = s.sense.hidden_on;
        if (h.hidden && !fill_in_allowed) {
            std::fprintf(stderr,
                         "blobgpu: world %s hides lines (fill-in), which the memory "
                         "engine on the GPU cannot answer yet\n",
                         w.name.c_str());
            std::exit(1);
        }
        plug_set_lines(p, s.sense);
        p.good_left = 0;
        p.bad_left = 0;
        std::memset(s.sense_in.data(), 0, sizeof(uint64_t) * tiles1);
        plug_write(p, s.shell);
        std::memcpy(&s.words[i * tiles1], s.sense_in.data(), sizeof(uint64_t) * tiles1);
    }
    t.world += since(clock);

    std::memcpy((GpuExchangeIn*)group.xin.mapped + from, s.xin.data(), sizeof(GpuExchangeIn) * n);
    std::memcpy((uint64_t*)group.xwords.mapped + (size_t)from * tiles1, s.words.data(),
                sizeof(uint64_t) * n * tiles1);
    t.exchange += since(clock);
}
