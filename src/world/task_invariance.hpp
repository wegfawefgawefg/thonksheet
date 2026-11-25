#pragma once

#include <cstdint>

// The invariance battery (docs/curriculum.md, "Invariance"; idea 101). One task kind with a
// setting for which change it is. Every kind shows one of a few shapes (or tunes, or 3D
// objects) in one of several versions of that change; the right button is the shape. Some
// versions are rewarded; trials on the others get no good or bad and are the only ones
// scored, so the score is carrying what was learned to versions it was never taught on.
// Shapes, versions and the moving stretches are all integers, so runs repeat exactly.

struct Task;
struct TaskSense;
struct TaskShape;
struct TaskStepIn;

enum class InvKind {
    position, // a shape at one of a grid of places
    scale,    // the same shape at 1x, 2x, 3x, about the middle
    rotation, // turned by quarter turns, plain and mirrored
    bright,   // each shape pixel lights 4, 2 or 1 of its 2x2 lines
    clutter,  // the shape with 0, 3, 8 or 16 extra 2x2 blocks lit around and among it
    mixed,    // place and scale at once: two sizes at four places
    shift,    // a short tune of three cue lines starting 0, 4 or 8 ticks into its step
    tempo,    // the same tune played with notes 2, 4, 6 or 8 ticks long
    pitch,    // a tune over a row of 16 frequency lines, shifted 0..10 lines along the row
    view,     // a small 3D object seen from one of eight angles around the vertical
    count,
};

constexpr int INV_MAX_VERSIONS = 64;
constexpr int INV_MAX_CLUTTER = 16;
constexpr int INV_CANVAS_W = 16, INV_CANVAS_H = 12; // the bitmap of the picture kinds

struct InvarianceSettings {
    int kind = 0;         // InvKind
    int shapes = 2;       // shapes, tunes or objects to tell apart (2..4; view 2..3)
    uint64_t hold = 0;    // one bit per version never rewarded; 0 means the kind's default
    int hold_draw = 0;    // > 0: draw this many held-out versions per life instead
    int motion = 0;       // percent of steps spent in unrewarded moving stretches (picture kinds)
    int motion_steps = 8; // steps in one moving stretch
};

// what one step shows, for the picture kinds: a template drawn about a centre
struct InvPose {
    int shape = 0;
    int cx2 = INV_CANVAS_W, cy2 = INV_CANVAS_H; // centre, in half lines from the corner
    int size4 = 8;   // lines per template pixel, in quarters (4 = 1 line)
    int angle = 0;   // in 16ths of a turn
    int mirror = 0;  // flip left-right before turning
    int density = 4; // lines lit of each 2x2 cell of a template pixel (4, 3, 2, 1)
    int clutter = 0; // extra 2x2 blocks lit
    uint8_t clutter_at[INV_MAX_CLUTTER] = {}; // which blocks of the 6x6 grid around the shape
};

struct InvarianceState {
    int version = 0;
    bool held = false;
    uint64_t held_set = 0;
    InvPose pose;
    // time kinds: where the tick under way is (set by the ticker)
    int tick = 0, step_ticks = 8;
    // moving stretches: steps left, steps done in motion and in all, and the drift
    int motion_left = 0;
    int64_t motion_done = 0, steps_done = 0;
    int drift_dx = 0, drift_dy = 0, drift_d = 0;
};

const char* inv_kind_name(int kind);
// "inv-scale" style names, or the kind's own name after --inv
bool inv_kind_from_name(const char* name, int& kind);
// blobrun's --task NAME: true and the settings set if NAME is "inv-<kind>"
bool invariance_task_name(const char* name, InvarianceSettings& s);
// blobrun's own flags for this family: true if the flag was one of them (ok false on a bad
// value, with a message)
bool invariance_flag(const char* flag, const char* value, InvarianceSettings& s, bool& ok);
int inv_versions(const InvarianceSettings& s);
// true for the kinds whose lines change inside a step (the ticker then senses every tick)
bool inv_by_tick(const Task& task);
// the ticker tells the task which tick of the step is under way
void inv_set_tick(Task& task, int tick, int step_ticks);
// the version of the trial under way and whether it is held out (also for `place`); false
// for a task with no versions
bool task_version(const Task& task, int& version, bool& held);

void invariance_reset(Task& task);
TaskShape invariance_shape(const Task& task);
void invariance_sense(const Task& task, TaskSense& sense);
int invariance_perfect_move(const Task& task);
int invariance_step(Task& task, const TaskStepIn& in);

// task_invariance_draw.cpp: the picture of one pose on a canvas_w-wide row-major sheet
void inv_draw(const InvPose& pose, int is_view, uint8_t* sheet, int canvas_w, int canvas_h);
bool inv_template_on(int shape, int tx, int ty);
// the tune of the time kinds: which of the three notes (bits 0..2) are on at tick t of the
// tune, for tune `tune` with notes `note` ticks long; 0 outside it
uint32_t inv_tune_notes(int tune, int note, int t);
int inv_tune_ticks(int note);
