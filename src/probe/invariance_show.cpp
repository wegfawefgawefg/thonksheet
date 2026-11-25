// blobprobe invariance --show 1: what a battery task puts on the sensor lines, as text. Each
// shape in each version (a held-out version is marked *), and for the picture kinds the
// first moving stretch frame by frame when --inv-motion is on. Time kinds print each
// line's on ticks over a whole trial (blank step, two steps of tune, the ask step). No blob.

#include "run/run_flags.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static void print_pictures(const std::vector<std::vector<uint8_t>>& pics,
                           const std::vector<std::string>& names, int w, int h) {
    const size_t per_row = 6;
    for (size_t from = 0; from < pics.size(); from += per_row) {
        size_t to = from + per_row < pics.size() ? from + per_row : pics.size();
        for (size_t i = from; i < to; i++) {
            std::printf("%-*s", w + 2, names[i].c_str());
        }
        std::printf("\n");
        for (int y = 0; y < h; y++) {
            for (size_t i = from; i < to; i++) {
                for (int x = 0; x < w; x++) {
                    std::putchar(pics[i].empty() ? '?'
                                                 : (pics[i][(size_t)(y * w + x)] ? '#' : '.'));
                }
                std::printf("  ");
            }
            std::printf("\n");
        }
        std::printf("\n");
    }
}

void inv_show(const RunFlags& flags) {
    if (flags.task != TaskKind::invariance) {
        std::fprintf(stderr, "blobprobe invariance --show: only the battery's inv-* tasks\n");
        return;
    }
    Task task;
    task_reset(task, flags.task, flags.task_settings, flags.task_seed);
    const InvarianceSettings& s = task.settings.inv;
    TaskShape shape = task_shape(task);
    int versions = inv_versions(s), shapes = s.shapes;
    int w = shape.sheet_w, h = shape.sheet_h;
    int step_ticks = flags.plug.ticks_per_step;
    bool by_tick = inv_by_tick(task);
    std::printf("%s: %d shapes, %d versions, held out:", inv_kind_name(s.kind), shapes, versions);
    for (int v = 0; v < versions; v++) {
        if ((task.inv.held_set >> v) & 1) {
            std::printf(" %d", v);
        }
    }
    std::printf("\n\n");

    std::vector<std::vector<uint8_t>> pics((size_t)(shapes * versions));
    std::vector<std::vector<uint8_t>> frames;
    // a time-kind trial: per line, one char per tick over the trial
    int lines = shape.singles + w * h;
    std::vector<uint8_t> strip;
    std::vector<std::vector<uint8_t>> strips((size_t)(shapes * versions));
    int left = shapes * versions;
    bool motion_done = s.motion == 0;
    for (int step = 0; step < 400000 && (left > 0 || !motion_done); step++) {
        bool moving = task.inv.motion_left > 0;
        bool answering = !moving && task.phase == task.trial_len - 1;
        int key = task.cue * versions + task.inv.version;
        if (!moving && task.phase == 0) {
            strip.assign((size_t)lines * task.trial_len * step_ticks, 0);
        }
        TaskSense sense;
        for (int tick = 0; tick < (by_tick ? step_ticks : 1); tick++) {
            inv_set_tick(task, tick, step_ticks);
            task_sense(task, sense);
            for (int l = 0; by_tick && l < lines; l++) {
                bool on = l < shape.singles ? sense.singles[l] : sense.sheet[l - shape.singles];
                int at = task.phase * step_ticks + tick;
                strip[(size_t)l * task.trial_len * step_ticks + at] = on;
            }
        }
        if (moving && !motion_done) {
            frames.emplace_back(sense.sheet, sense.sheet + w * h);
            motion_done = task.inv.motion_left == 1;
        }
        if (answering && key >= 0 && pics[(size_t)key].empty()) {
            pics[(size_t)key].assign(sense.sheet, sense.sheet + w * h);
            strips[(size_t)key] = strip;
            left--;
        }
        TaskStepIn in;
        in.move = task_perfect_move(task);
        task_step(task, in);
    }
    if (!by_tick) {
        for (int sh = 0; sh < shapes; sh++) {
            std::vector<std::vector<uint8_t>> row;
            std::vector<std::string> names;
            for (int v = 0; v < versions; v++) {
                row.push_back(pics[(size_t)(sh * versions + v)]);
                bool held = (task.inv.held_set >> v) & 1;
                names.push_back("s" + std::to_string(sh) + " v" + std::to_string(v) +
                                (held ? "*" : ""));
            }
            print_pictures(row, names, w, h);
        }
        if (!frames.empty()) {
            std::printf("one moving stretch, frame by frame:\n");
            std::vector<std::string> names;
            for (size_t i = 0; i < frames.size(); i++) {
                names.push_back("step " + std::to_string(i));
            }
            print_pictures(frames, names, w, h);
        }
        return;
    }
    for (int key = 0; key < shapes * versions; key++) {
        const std::vector<uint8_t>& st = strips[(size_t)key];
        if (st.empty()) {
            continue;
        }
        int len = (int)st.size() / lines;
        int v = key % versions;
        std::printf("tune %d, version %d%s (ticks; | starts a step)\n", key / versions, v,
                    ((task.inv.held_set >> v) & 1) ? " held out" : "");
        for (int l = 0; l < lines; l++) {
            bool any = false;
            for (int t = 0; t < len; t++) {
                any = any || st[(size_t)l * len + t];
            }
            // pitch rows repeat the same column; show each frequency line once
            if (!any || (l >= shape.singles && (l - shape.singles) / w > 0)) {
                continue;
            }
            if (l < shape.singles) {
                static const char* const names[3] = {"A", "B", "C"};
                std::printf("  %-6s ", l == shape.singles - 1 ? "ask" : names[l]);
            } else {
                std::printf("  f%-5d ", (l - shape.singles) % w);
            }
            for (int t = 0; t < len; t++) {
                if (t % step_ticks == 0) {
                    std::putchar('|');
                }
                std::putchar(st[(size_t)l * len + t] ? '#' : '.');
            }
            std::printf("\n");
        }
    }
}
