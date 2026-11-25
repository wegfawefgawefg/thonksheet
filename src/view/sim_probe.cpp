// The readability probe on the live sheet (probe/feature_measure.hpp, blobprobe feature's
// measurement): the probe's tries go onto one tile's sensor cells, every non-sensor
// neuron's spikes in each try's reading window are counted, and each tile gets the number
// an ideal reader of it would get. Any engine: it needs only the shell's sense lines and
// spike map.
//
// Asked for while a task runs, it waits for the end of a task step, takes the task's lines
// off the sheet (the task stands still), measures with learning off so the measurement
// teaches nothing, then plugs the lines back and the task goes on. With --probe it is the
// world from the start, learning as the genome says, measuring again and again.

#include "sim.hpp"

#include <algorithm>
#include <cstdlib>

// the probe's numbers are refreshed every this many tries while it runs
constexpr int PROBE_REFRESH = 50;

void sim_probe_ask(Sim& sim, int feature) {
    SimProbe& p = sim.probe;
    if (sim.blob == nullptr || feature < 0 || feature >= READ_FEATURES) {
        return;
    }
    // --probe: switch to the new feature at once. otherwise it comes after the one under way
    if (p.on && p.live) {
        p.results[static_cast<int>(p.feature)].running = false;
        p.on = false;
    }
    p.pending = feature;
}

// the tile the cues go on: the flags', else the tile of the task's first on/off line, else
// tile (1, 1) as in blobprobe
static int cue_tile(const Sim& sim) {
    const Blob& b = *sim.blob;
    const ProbeSettings& ps = sim.settings.probe;
    if (ps.cue_x >= 0 && ps.cue_y >= 0) {
        return std::min(ps.cue_y, b.tiles_h - 1) * b.tiles_w + std::min(ps.cue_x, b.tiles_w - 1);
    }
    const Plug& plug = sim.task.ticker.plug;
    if (sim.task.on && plug.shape.singles > 0) {
        const int x = plug.sensor_cell_x[plug.first_single];
        const int y = plug.sensor_cell_y[plug.first_single];
        return (y / 8) * b.tiles_w + x / 8;
    }
    return std::min(1, b.tiles_h - 1) * b.tiles_w + std::min(1, b.tiles_w - 1);
}

static void next_try(SimProbe& p, const ProbeSettings& ps) {
    p.random ^= p.random << 13;
    p.random ^= p.random >> 7;
    p.random ^= p.random << 17;
    p.now = feature_draw_try(p.feature, p.random >> 20, ps.hold_gap);
    p.tick = 0;
    std::fill(p.count.begin(), p.count.end(), 0);
}

// the per-tile numbers from the sums so far, and their means by distance from the cue tile
static void refresh(SimProbe& p, const Blob& b, int want) {
    Readability& r = p.results[static_cast<int>(p.feature)];
    r.want = want;
    r.tries = static_cast<int>(p.sums.seen[0] + p.sums.seen[1]);
    r.tick = b.stats.tick;
    r.cue_tile = p.cue_tile;
    r.tile.assign(b.tile_count, 0.0f);
    float sum[READ_DISTANCES] = {};
    int at[READ_DISTANCES] = {};
    const int cx = p.cue_tile % b.tiles_w, cy = p.cue_tile / b.tiles_w;
    for (int t = 0; t < b.tile_count; ++t) {
        r.tile[t] = static_cast<float>(feature_tile_readable(p.sums, t));
        const int d = std::max(std::abs(t % b.tiles_w - cx), std::abs(t / b.tiles_w - cy));
        if (d < READ_DISTANCES) {
            sum[d] += r.tile[t];
            at[d] += 1;
        }
    }
    r.distances = 0;
    for (int d = 0; d < READ_DISTANCES && at[d] > 0; ++d) {
        r.by_distance[d] = sum[d] / at[d];
        r.distances = d + 1;
    }
}

static void start(Sim& sim, int feature) {
    SimProbe& p = sim.probe;
    Blob& b = *sim.blob;
    p.on = true;
    p.live = sim.settings.probe.feature >= 0;
    p.feature = static_cast<Feature>(feature);
    p.cue_tile = cue_tile(sim);
    p.ticks = 0;
    p.random = 0x2545F4914F6CDD1Dull ^ (sim.settings.seed + b.stats.tick);
    feature_sums_begin(p.sums, b.neuron_count);
    p.count.assign(b.neuron_count, 0);
    if (sim.task.on && sim.task.plugged) {
        plug_detach(sim.task.ticker.plug, b);
        sim.task.plugged = false;
    }
    if (!p.live) {
        p.learning = engine_set_learning(sim.engine, b, false);
    }
    p.results[feature].running = true;
    next_try(p, sim.settings.probe);
}

static void finish(Sim& sim) {
    SimProbe& p = sim.probe;
    Blob& b = *sim.blob;
    refresh(p, b, sim.settings.probe.tries);
    p.results[static_cast<int>(p.feature)].running = false;
    p.on = false;
    feature_set_cues(b, p.cue_tile, false, false);
    if (p.live) {
        if (p.pending < 0) {
            p.pending = static_cast<int>(p.feature); // again, on the sheet as it is now
        }
        return;
    }
    engine_set_learning(sim.engine, b, p.learning);
    if (sim.task.on && !sim.task.plugged) {
        TaskTicker& tt = sim.task.ticker;
        plug_attach(tt.plug, b, task_shape(tt.task), sim.settings.run.plug);
        sim.task.plugged = true;
    }
}

bool sim_probe_on(Sim& sim) {
    SimProbe& p = sim.probe;
    if (p.on || p.pending < 0) {
        return p.on;
    }
    // a task waits for the end of its step
    const TaskTicker& tt = sim.task.ticker;
    const bool between = !sim.task.on || (tt.tick_in_step == 0 && tt.rest_left == 0);
    if (between) {
        const int feature = p.pending;
        p.pending = -1;
        start(sim, feature);
    }
    return p.on;
}

void sim_probe_before_tick(Sim& sim) {
    SimProbe& p = sim.probe;
    bool a_on = false, b_on = false;
    if (p.tick < p.now.length) {
        feature_lines_at(p.feature, p.now, p.tick, a_on, b_on);
    }
    feature_set_cues(*sim.blob, p.cue_tile, a_on, b_on);
}

void sim_probe_after_tick(Sim& sim) {
    SimProbe& p = sim.probe;
    const ProbeSettings& ps = sim.settings.probe;
    const bool counting = p.ticks >= ps.settle;
    ++p.ticks;
    if (counting && p.tick >= p.now.read_from && p.tick < p.now.read_to) {
        feature_count_tick(*sim.blob, p.count.data());
    }
    if (++p.tick < p.now.length + ps.rest) {
        return;
    }
    // the try is over
    if (counting) {
        feature_sums_add(p.sums, p.count.data(), p.now.label);
        const int64_t done = p.sums.seen[0] + p.sums.seen[1];
        if (done >= ps.tries) {
            finish(sim);
            return;
        }
        if (done % PROBE_REFRESH == 0) {
            const Readability& last = p.results[static_cast<int>(p.feature)];
            // live: keep showing the last full run until the new one has some tries
            if (!p.live || last.tile.empty() || done >= ps.tries / 4) {
                refresh(p, *sim.blob, ps.tries);
                p.results[static_cast<int>(p.feature)].running = true;
            }
        }
    }
    next_try(p, ps);
}

void sim_probe_fill(const Sim& sim, ViewFrame& out) {
    for (int i = 0; i < READ_FEATURES; ++i) {
        out.readable[i] = sim.probe.results[i];
    }
    out.reading = sim.probe.on ? static_cast<int>(sim.probe.feature) : sim.probe.pending;
}
