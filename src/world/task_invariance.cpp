#include "world/task.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// the invariance battery (task_invariance.hpp). picture kinds: a trial is one blank step,
// then one step with the picture and the ask line; only that step's press counts. time
// kinds: one blank step, two steps (shift: one) in which the tune plays tick by tick, then the ask
// step with every note off. the right button is the shape (or tune, or object). versions in the
// held-out set are never rewarded and are the only trials scored (tally 0 and 1); the rest
// are rewarded and counted apart (tally 2 and 3). versions are drawn evenly.
//
// --inv-motion P: before a trial, whenever moving steps are under P percent of all steps, a
// moving stretch of --inv-motion-steps steps runs instead: one shape, no ask, no reward,
// drifting one notch a step through the versions (sliding a line, growing a quarter line,
// turning a 16th, one line dimmer, one block more clutter), bouncing at the ends.

static const char* const kind_names[(int)InvKind::count] = {
    "position", "scale", "rotation", "bright", "clutter",
    "mixed",    "shift", "tempo",    "pitch",  "view",
};

constexpr int SHOW_STEPS = 2; // time kinds: steps the tune has to play in (shift: 1)
constexpr int SHIFT_NOTE = 3; // ticks per note, shift
constexpr int PITCH_NOTE = 6; // ticks per note, pitch
constexpr int PITCH_H = 8;    // pitch: each frequency line is a column this tall

static const int bright_density[3] = {4, 2, 1};
static const int clutter_count[4] = {0, 3, 8, 16};
static const int tempo_note[4] = {2, 4, 6, 8};
static const int mixed_cx2[4] = {8, 12, 20, 24}; // two places left, two right, middle row

static InvKind kind_of(const Task& task) {
    return (InvKind)task.settings.inv.kind;
}

static bool picture_kind(InvKind k) {
    return k <= InvKind::mixed || k == InvKind::view;
}

// shift plays its tune in one step, so even the earliest start is at most 8 ticks before
// the ask; the default sheet has mostly lost a cue 20 to 60 ticks after it goes off
static int show_steps(InvKind k) {
    return k == InvKind::shift ? 1 : SHOW_STEPS;
}

const char* inv_kind_name(int kind) {
    return kind >= 0 && kind < (int)InvKind::count ? kind_names[kind] : "?";
}

bool inv_kind_from_name(const char* name, int& kind) {
    for (int i = 0; i < (int)InvKind::count; i++) {
        if (std::strcmp(name, kind_names[i]) == 0) {
            kind = i;
            return true;
        }
    }
    return false;
}

bool invariance_task_name(const char* name, InvarianceSettings& s) {
    return std::strncmp(name, "inv-", 4) == 0 && inv_kind_from_name(name + 4, s.kind);
}

int inv_versions(const InvarianceSettings& s) {
    static const int versions[(int)InvKind::count] = {15, 3, 8, 3, 4, 8, 3, 4, 6, 8};
    return s.kind >= 0 && s.kind < (int)InvKind::count ? versions[s.kind] : 1;
}

// held out unless said otherwise: a bit per version, or a count drawn per life
static uint64_t default_hold(InvKind k) {
    switch (k) {
    case InvKind::scale:
        return 1u << 0; // 1x, the smallest; taught at 2x and 3x
    case InvKind::rotation:
        return 0xCC; // half and three quarter turns, plain and mirrored
    case InvKind::bright:
        return 1u << 2; // one line per pixel
    case InvKind::clutter:
        return 0xC; // 8 and 16 blocks
    case InvKind::mixed:
        return 0x3C; // 1x on the right, 2x on the left
    case InvKind::shift:
        return 1u << 1; // starting 4 ticks in, between the taught 0 and 8
    case InvKind::tempo:
        return 1u << 0; // 2 ticks a note, faster than any taught
    case InvKind::view:
        return 0x1C; // a quarter turn to a half: three views in a row
    default:
        return 0;
    }
}

static int default_draw(InvKind k) {
    return k == InvKind::position ? 4 : (k == InvKind::pitch ? 2 : 0);
}

bool invariance_flag(const char* flag, const char* value, InvarianceSettings& s, bool& ok) {
    ok = true;
    if (std::strcmp(flag, "--inv") == 0) {
        ok = inv_kind_from_name(value, s.kind);
        if (!ok) {
            std::fprintf(stderr, "blobrun: --inv is position, scale, rotation, bright, clutter, "
                                 "mixed, shift, tempo, pitch or view\n");
        }
        return true;
    }
    if (std::strcmp(flag, "--inv-hold") == 0) {
        // "2,3": versions 2 and 3; "r4": four drawn per life
        s.hold = 0;
        s.hold_draw = 0;
        if (value[0] == 'r') {
            s.hold_draw = std::atoi(value + 1);
            ok = s.hold_draw > 0;
        } else {
            for (const char* p = value; *p;) {
                int v = std::atoi(p);
                ok = ok && v >= 0 && v < INV_MAX_VERSIONS && *p >= '0' && *p <= '9';
                s.hold |= ok ? 1ull << v : 0;
                while (*p && *p != ',') {
                    p++;
                }
                p += *p == ',';
            }
        }
        if (!ok) {
            std::fprintf(stderr, "blobrun: --inv-hold is a list like 2,3 or rN\n");
        }
        return true;
    }
    int* field = nullptr;
    if (std::strcmp(flag, "--inv-shapes") == 0) {
        field = &s.shapes;
    } else if (std::strcmp(flag, "--inv-motion") == 0) {
        field = &s.motion;
    } else if (std::strcmp(flag, "--inv-motion-steps") == 0) {
        field = &s.motion_steps;
    }
    if (!field) {
        return false;
    }
    char* end = nullptr;
    *field = (int)std::strtol(value, &end, 10);
    ok = end != value && *end == '\0';
    if (!ok) {
        std::fprintf(stderr, "blobrun: %s needs a whole number\n", flag);
    }
    return true;
}

bool inv_by_tick(const Task& task) {
    InvKind k = kind_of(task);
    return task.kind == TaskKind::invariance &&
           (k == InvKind::shift || k == InvKind::tempo || k == InvKind::pitch);
}

void inv_set_tick(Task& task, int tick, int step_ticks) {
    task.inv.tick = tick;
    task.inv.step_ticks = step_ticks;
}

bool task_version(const Task& task, int& version, bool& held) {
    if (task.kind == TaskKind::place) {
        version = task.place;
        held = task.scored;
        return true;
    }
    if (task.kind != TaskKind::invariance || task.inv.motion_left > 0) {
        return false;
    }
    version = task.inv.version;
    held = task.inv.held;
    return true;
}

// clutter blocks from `from` on: distinct blocks of the 6x6 grid not lit by the shape
static void draw_clutter(Task& task, InvPose& p, int from) {
    for (int i = from; i < p.clutter; i++) {
        for (;;) {
            int b = world_random_below(task.random, 36);
            int bx = b % 6, by = b / 6;
            bool taken = inv_template_on(p.shape, bx - 1, by - 1);
            for (int j = 0; j < i; j++) {
                taken = taken || p.clutter_at[j] == b;
            }
            if (!taken) {
                p.clutter_at[i] = (uint8_t)b;
                break;
            }
        }
    }
}

static void pose_of(Task& task, int v, int shape, InvPose& p) {
    p = InvPose{};
    p.shape = shape;
    switch (kind_of(task)) {
    case InvKind::position:
        // 8x8 footprint, top-left corner on a 2-line grid: x 0..8, y 0..4
        p.cx2 = 2 * (2 * (v % 5)) + 8;
        p.cy2 = 2 * (2 * (v / 5)) + 8;
        break;
    case InvKind::scale:
        p.size4 = 4 * (v + 1);
        break;
    case InvKind::rotation:
        p.angle = (v % 4) * 4;
        p.mirror = v / 4;
        break;
    case InvKind::bright:
        p.density = bright_density[v];
        break;
    case InvKind::clutter:
        p.clutter = clutter_count[v];
        draw_clutter(task, p, 0);
        break;
    case InvKind::mixed:
        p.size4 = v < 4 ? 4 : 8;
        p.cx2 = mixed_cx2[v % 4];
        break;
    case InvKind::view:
        p.angle = 2 * v;
        break;
    default:
        break;
    }
}

static int bounce(int& value, int& d, int lo, int hi) {
    value += d;
    if (value < lo || value > hi) {
        d = -d;
        value += 2 * d;
    }
    return value;
}

static void start_motion(Task& task) {
    InvarianceState& st = task.inv;
    WorldRandom& r = task.random;
    int shape = world_random_below(r, task.settings.inv.shapes);
    pose_of(task, world_random_below(r, inv_versions(task.settings.inv)), shape, st.pose);
    st.motion_left = task.settings.inv.motion_steps;
    int dir = world_random_below(r, 4);
    st.drift_dx = dir == 0 ? 1 : (dir == 1 ? -1 : 0);
    st.drift_dy = dir == 2 ? 1 : (dir == 3 ? -1 : 0);
    st.drift_d = world_random_below(r, 2) ? 1 : -1;
    if (kind_of(task) == InvKind::mixed) {
        st.drift_dx = st.drift_d;
        st.drift_d = world_random_below(r, 2) ? 1 : -1;
    }
}

// one notch further along the drift
static void drift(Task& task) {
    InvarianceState& st = task.inv;
    InvPose& p = st.pose;
    switch (kind_of(task)) {
    case InvKind::position:
        bounce(p.cx2, st.drift_dx, 8, 24);
        bounce(p.cy2, st.drift_dy, 8, 16);
        break;
    case InvKind::scale:
        bounce(p.size4, st.drift_d, 4, 12);
        break;
    case InvKind::rotation:
    case InvKind::view:
        p.angle = (p.angle + st.drift_d) & 15;
        break;
    case InvKind::bright:
        bounce(p.density, st.drift_d, 1, 4);
        break;
    case InvKind::clutter: {
        int before = p.clutter;
        bounce(p.clutter, st.drift_d, 0, INV_MAX_CLUTTER);
        draw_clutter(task, p, before);
        break;
    }
    case InvKind::mixed:
        bounce(p.cx2, st.drift_dx, 8, 24);
        bounce(p.size4, st.drift_d, 4, 8);
        break;
    default:
        break;
    }
}

static void new_trial(Task& task) {
    const InvarianceSettings& s = task.settings.inv;
    InvarianceState& st = task.inv;
    WorldRandom& r = task.random;
    InvKind k = kind_of(task);
    task.phase = 0;
    if (picture_kind(k) && s.motion > 0 &&
        st.motion_done * 100 < (int64_t)s.motion * st.steps_done) {
        start_motion(task);
        task.trial_len = 1;
        task.cue = -1;
        return;
    }
    task.cue = world_random_below(r, s.shapes);
    st.version = world_random_below(r, inv_versions(s));
    st.held = (st.held_set >> st.version) & 1;
    task.scored = st.held;
    pose_of(task, st.version, task.cue, st.pose);
    task.trial_len = picture_kind(k) ? 2 : 2 + show_steps(k);
}

void invariance_reset(Task& task) {
    InvarianceSettings& s = task.settings.inv;
    InvKind k = kind_of(task);
    int max_shapes = k == InvKind::view ? 3 : 4;
    s.shapes = s.shapes < 2 ? 2 : (s.shapes > max_shapes ? max_shapes : s.shapes);
    s.motion = s.motion < 0 ? 0 : (s.motion > 95 ? 95 : s.motion);
    s.motion_steps = s.motion_steps < 1 ? 1 : s.motion_steps;
    int n = inv_versions(s);
    uint64_t all = n >= 64 ? ~0ull : (1ull << n) - 1;
    InvarianceState& st = task.inv;
    st = InvarianceState{};
    int draw = s.hold_draw > 0 ? s.hold_draw : (s.hold == 0 ? default_draw(k) : 0);
    if (draw > 0) {
        draw = draw > n - 1 ? n - 1 : draw;
        while (draw > 0) {
            int v = world_random_below(task.random, n);
            if (!((st.held_set >> v) & 1)) {
                st.held_set |= 1ull << v;
                draw--;
            }
        }
    } else {
        st.held_set = (s.hold ? s.hold : default_hold(k)) & all;
    }
    if (st.held_set == 0 || st.held_set == all) {
        // at least one version taught and one held out
        st.held_set = st.held_set == 0 ? 1ull << (n - 1) : all & ~1ull;
    }
    new_trial(task);
}

TaskShape invariance_shape(const Task& task) {
    TaskShape shape;
    shape.motors = task.settings.inv.shapes;
    InvKind k = kind_of(task);
    if (picture_kind(k)) {
        shape.singles = 1; // ask
        shape.sheet_w = INV_CANVAS_W;
        shape.sheet_h = INV_CANVAS_H;
    } else if (k == InvKind::pitch) {
        shape.singles = 1;
        shape.sheet_w = 16;
        shape.sheet_h = PITCH_H;
    } else {
        shape.singles = 4; // three notes and the ask
    }
    return shape;
}

// time kinds: the notes on at the tick under way, as three bits
static uint32_t notes_now(const Task& task) {
    const InvarianceState& st = task.inv;
    int show = show_steps(kind_of(task));
    if (task.phase < 1 || task.phase > show) {
        return 0;
    }
    int window = show * st.step_ticks;
    int t = (task.phase - 1) * st.step_ticks + st.tick;
    int note = PITCH_NOTE, start = 0;
    switch (kind_of(task)) {
    case InvKind::shift: {
        note = SHIFT_NOTE;
        int room = window - inv_tune_ticks(note);
        room = room < 0 ? 0 : room;
        start = st.version * room / (inv_versions(task.settings.inv) - 1);
        break;
    }
    case InvKind::tempo:
        note = tempo_note[st.version];
        start = window - inv_tune_ticks(note); // ends as the ask comes on
        break;
    default:
        start = window - inv_tune_ticks(note);
        break;
    }
    return inv_tune_notes(task.cue, note, t - start);
}

void invariance_sense(const Task& task, TaskSense& sense) {
    const InvarianceState& st = task.inv;
    InvKind k = kind_of(task);
    bool is_view = k == InvKind::view;
    if (st.motion_left > 0) {
        inv_draw(st.pose, is_view, sense.sheet, INV_CANVAS_W, INV_CANVAS_H);
        return;
    }
    bool answering = task.phase == task.trial_len - 1;
    if (picture_kind(k)) {
        sense.singles[0] = answering;
        if (answering) {
            inv_draw(st.pose, is_view, sense.sheet, INV_CANVAS_W, INV_CANVAS_H);
        }
        return;
    }
    uint32_t notes = notes_now(task);
    if (k == InvKind::pitch) {
        // note i is the line 2i above the tune's lowest, which is two lines per version up
        sense.singles[0] = answering;
        for (int i = 0; i < 3; i++) {
            int col = 2 * st.version + 2 * i;
            for (int y = 0; notes >> i & 1 && y < PITCH_H && col < 16; y++) {
                sense.sheet[y * 16 + col] = 1;
            }
        }
        return;
    }
    for (int i = 0; i < 3; i++) {
        sense.singles[i] = (notes >> i) & 1;
    }
    sense.singles[3] = answering;
}

int invariance_perfect_move(const Task& task) {
    if (task.inv.motion_left > 0) {
        return -1;
    }
    return task.phase == task.trial_len - 1 ? task.cue : -1;
}

int invariance_step(Task& task, const TaskStepIn& in) {
    InvarianceState& st = task.inv;
    st.steps_done++;
    if (st.motion_left > 0) {
        st.motion_done++;
        st.motion_left--;
        drift(task);
        if (st.motion_left == 0) {
            new_trial(task);
        }
        return 0;
    }
    if (task.phase < task.trial_len - 1) {
        task.phase++; // presses before the ask are ignored
        return 0;
    }
    bool right = in.move == task.cue;
    int valence = in.move < 0 ? 0 : (right ? 1 : -1);
    if (st.held) {
        task.tally[1]++;
        task.tally[0] += right;
        valence = 0; // held-out versions are never rewarded
    } else {
        task.tally[3]++;
        task.tally[2] += right;
    }
    new_trial(task);
    return valence;
}
