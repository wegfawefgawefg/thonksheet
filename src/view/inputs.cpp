// Keys and the mouse. Keys sit under the left hand; the mouse does the rest.
//
//   space  pause / run           E  one tick (pauses)        S  slow motion on / off
//   W      one task step (pauses; with --task)
//   A / D  slow motion slower / faster                        F  full speed, nothing drawn
//   T  tile lines   G  sector lines   R  raster rows: neurons / tiles   Z  reset zoom
//   V  pick the next button's cell (the readout)   Q  its evidence picture on / off
//   C  readability tint: the next feature (measured if it has not been)   X  measure again
//   left click   pick a neuron (sheet or raster)    esc  drop the pick, else quit
//   wheel        zoom the sheet or the raster rows  right drag  pan them
//   F11          fullscreen

#include "imgui_layer.hpp"
#include "state.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

static void select_neuron(State& s, int neuron) {
    s.sim.shared.selected = neuron;
}

void pick_readout_cell(State& s, int index) {
    const std::vector<ReadoutCell>& cells = s.frame.readout.cells;
    if (cells.empty()) {
        return;
    }
    const int count = static_cast<int>(cells.size());
    // V walks the buttons in turn (the cells are listed button by button, the same number
    // each): the next button's cell of the same rank, then the next rank
    if (index < 0) {
        const int buttons = std::max<int>(1, static_cast<int>(s.frame.readout.buttons.size()));
        const int per = std::max(1, count / buttons);
        int button = 0, rank = 0;
        if (s.readout_pick >= 0) {
            button = s.readout_pick / per + 1;
            rank = s.readout_pick % per;
            if (button >= buttons) {
                button = 0;
                rank = (rank + 1) % per;
            }
        }
        index = (button * per + rank) % count;
    }
    s.readout_pick = std::clamp(index, 0, count - 1);
    const ReadoutCell& c = cells[s.readout_pick];
    select_neuron(s, ((c.y / 8) * s.frame.tiles_w + c.x / 8) * 64 + (c.y % 8) * 8 + c.x % 8);
}

void measure_now(State& s, int feature) {
    s.sim.shared.measure = feature;
}

void cycle_tint(State& s) {
    s.view.tint = s.view.tint + 1 >= READ_FEATURES ? -1 : s.view.tint + 1;
    const int t = s.view.tint;
    if (t >= 0 && s.frame.readable[t].tile.empty() && s.frame.reading != t) {
        measure_now(s, t);
    }
}

static void zoom_sheet(State& s, float mx, float my, float wheel) {
    const Rect& r = s.layout.sheet;
    const int cw = s.frame.tiles_w * 8;
    const int ch = s.frame.tiles_h * 8;
    const SheetPlace before = sheet_place(r, cw, ch, s.view);
    const float cell_x = (mx - before.origin_x) / before.cell_px;
    const float cell_y = (my - before.origin_y) / before.cell_px;
    s.view.zoom = std::clamp(s.view.zoom * std::pow(1.25f, wheel), 1.0f, 64.0f);
    // keep the cell under the mouse where it is
    ViewSettings fresh = s.view;
    fresh.pan_x = fresh.pan_y = 0;
    const float px = sheet_place(r, cw, ch, fresh).cell_px;
    s.view.pan_x = cell_x - cw * 0.5f - (mx - (r.x + r.w * 0.5f)) / px;
    s.view.pan_y = cell_y - ch * 0.5f - (my - (r.y + r.h * 0.5f)) / px;
    if (s.view.zoom <= 1.0f) {
        s.view.pan_x = s.view.pan_y = 0;
    }
}

static void zoom_raster(State& s, float my, float wheel) {
    const Rect& r = s.layout.raster;
    const float total = static_cast<float>(raster_total_rows(s.frame.tile_count, s.view));
    float first = 0, per_px = 1;
    raster_rows(r, s.frame.tile_count, s.view, &first, &per_px);
    const float under = first + (my - r.y) * per_px;
    const float count = std::clamp(per_px * r.h * std::pow(0.8f, wheel), 16.0f, total);
    s.view.raster_count = count >= total ? 0.0f : count;
    s.view.raster_first = count >= total ? 0.0f : under - (my - r.y) * count / r.h;
}

static void click(State& s, float mx, float my) {
    if (rect_contains(s.layout.sheet, mx, my)) {
        const SheetPlace p =
            sheet_place(s.layout.sheet, s.frame.tiles_w * 8, s.frame.tiles_h * 8, s.view);
        select_neuron(s, sheet_pick(p, s.frame.tiles_w, s.frame.tiles_h, mx, my));
    } else if (rect_contains(s.layout.raster, mx, my) && s.view.raster_rows == 0) {
        float first = 0, per_px = 1;
        raster_rows(s.layout.raster, s.frame.tile_count, s.view, &first, &per_px);
        select_neuron(s, static_cast<int>(first + (my - s.layout.raster.y) * per_px));
    }
}

// right drag: pan the sheet, or scroll the raster rows when zoomed
static void drag(State& s, float x, float y) {
    const float dx = x - s.drag.x;
    const float dy = y - s.drag.y;
    s.drag.x = x;
    s.drag.y = y;
    if (s.drag.sheet) {
        const SheetPlace p =
            sheet_place(s.layout.sheet, s.frame.tiles_w * 8, s.frame.tiles_h * 8, s.view);
        s.view.pan_x -= dx / p.cell_px;
        s.view.pan_y -= dy / p.cell_px;
    } else if (s.view.raster_count > 0) {
        float first = 0, per_px = 1;
        raster_rows(s.layout.raster, s.frame.tile_count, s.view, &first, &per_px);
        s.view.raster_first = first - dy * per_px;
    }
}

static void key(State& s, SDL_Keycode k, SDL_Window* window) {
    SimShared& sh = s.sim.shared;
    if (k == SDLK_SPACE) {
        set_mode(s, s.mode == RUN_PAUSED ? RUN_FULL : RUN_PAUSED);
    } else if (k == SDLK_E) {
        set_mode(s, RUN_PAUSED);
        sh.steps += 1;
    } else if (k == SDLK_W && s.sim.task.on) {
        set_mode(s, RUN_PAUSED);
        sh.task_steps += 1;
    } else if (k == SDLK_S) {
        set_mode(s, s.mode == RUN_SLOW ? RUN_FULL : RUN_SLOW);
    } else if (k == SDLK_F) {
        set_mode(s, s.mode == RUN_TURBO ? RUN_FULL : RUN_TURBO);
    } else if (k == SDLK_A) {
        sh.slow_tps = std::max(1, sh.slow_tps / 2);
    } else if (k == SDLK_D) {
        sh.slow_tps = std::min(4096, sh.slow_tps * 2);
    } else if (k == SDLK_T) {
        s.view.tile_lines = !s.view.tile_lines;
    } else if (k == SDLK_G) {
        s.view.sector_lines = !s.view.sector_lines;
    } else if (k == SDLK_R) {
        s.view.raster_rows = 1 - s.view.raster_rows;
        s.view.raster_first = s.view.raster_count = 0;
    } else if (k == SDLK_V) {
        pick_readout_cell(s, -1);
    } else if (k == SDLK_Q) {
        s.view.evidence = !s.view.evidence;
    } else if (k == SDLK_C) {
        cycle_tint(s);
    } else if (k == SDLK_X && s.frame.real) {
        measure_now(s, s.view.tint >= 0 ? s.view.tint : 0);
        s.view.tint = s.view.tint >= 0 ? s.view.tint : 0;
    } else if (k == SDLK_Z) {
        s.view.zoom = 1;
        s.view.pan_x = s.view.pan_y = 0;
        s.view.raster_first = s.view.raster_count = 0;
    } else if (k == SDLK_ESCAPE) {
        if (sh.selected.load() >= 0) {
            select_neuron(s, -1);
        } else {
            s.running = false;
        }
    } else if (k == SDLK_F11 && window != nullptr) {
        const bool full = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
        SDL_SetWindowFullscreen(window, !full);
    }
}

void handle_event(State& s, const SDL_Event& e) {
    // the views are never under an ImGui window, so the mouse over a view is ours (ImGui's
    // own idea of where the mouse is runs a frame behind)
    const auto ours = [&](float x, float y) {
        return rect_contains(s.layout.sheet, x, y) || rect_contains(s.layout.raster, x, y) ||
               !imgui_want_capture_mouse();
    };
    if (e.type == SDL_EVENT_QUIT) {
        s.running = false;
    } else if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        s.resized = true;
    } else if (e.type == SDL_EVENT_KEY_DOWN && !imgui_want_capture_keyboard()) {
        const bool repeats = e.key.key == SDLK_E || e.key.key == SDLK_W || e.key.key == SDLK_A ||
                             e.key.key == SDLK_D;
        if (!e.key.repeat || repeats) {
            key(s, e.key.key, SDL_GetWindowFromID(e.key.windowID));
        }
    } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && ours(e.button.x, e.button.y)) {
        if (e.button.button == SDL_BUTTON_LEFT) {
            click(s, e.button.x, e.button.y);
        } else if (e.button.button == SDL_BUTTON_RIGHT) {
            s.drag.sheet = rect_contains(s.layout.sheet, e.button.x, e.button.y);
            s.drag.raster = rect_contains(s.layout.raster, e.button.x, e.button.y);
            s.drag.x = e.button.x;
            s.drag.y = e.button.y;
        }
    } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_RIGHT) {
        s.drag = Drag{};
    } else if (e.type == SDL_EVENT_MOUSE_MOTION && (s.drag.sheet || s.drag.raster)) {
        drag(s, e.motion.x, e.motion.y);
    } else if (e.type == SDL_EVENT_MOUSE_WHEEL && ours(e.wheel.mouse_x, e.wheel.mouse_y)) {
        if (rect_contains(s.layout.sheet, e.wheel.mouse_x, e.wheel.mouse_y)) {
            zoom_sheet(s, e.wheel.mouse_x, e.wheel.mouse_y, e.wheel.y);
        } else if (rect_contains(s.layout.raster, e.wheel.mouse_x, e.wheel.mouse_y)) {
            zoom_raster(s, e.wheel.mouse_y, e.wheel.y);
        }
    }
}
