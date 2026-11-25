#version 450
#extension GL_GOOGLE_include_directive : require
// The sheet: one cell per neuron, dim in its kind's colour, lit when it fires and fading
// over pc.fade ticks. With the overlay (flags 4) a cell's colour can be replaced (the
// buttons' cells) or laid over (evidence, readability). Tile and sector lines on top.

#include "common.glsl"

layout(location = 0) out vec4 out_color;

void main() {
    vec2 px = gl_FragCoord.xy - pc.origin;
    vec2 cell = px / pc.cell_px;
    if (cell.x < 0.0 || cell.y < 0.0 || cell.x >= float(pc.cells_w) ||
        cell.y >= float(pc.cells_h)) {
        out_color = vec4(BACKGROUND, 1.0);
        return;
    }
    uint cx = uint(cell.x);
    uint cy = uint(cell.y);
    uint tile = (cy / 8u) * pc.tiles_w + cx / 8u;
    uint b = (cy % 8u) * 8u + cx % 8u;
    vec3 kind = kind_color(tile, b);
    uint over = 0u;
    float quiet = 0.13;
    if ((pc.flags & 4u) != 0u) {
        over = overlay[cy * pc.cells_w + cx];
        if ((over >> 31) != 0u) {
            kind = overlay_rgb(over);
            quiet = 0.38; // a button's cell stays visible while quiet
        }
    }

    // the newest spike within the fade
    uint ages = min(uint(pc.fade), pc.rows_valid);
    float glow = 0.0;
    for (uint age = 0u; age < ages; ++age) {
        if (bit_set(spikes_at(age, tile), b)) {
            glow = 1.0 - float(age) / float(ages);
            break;
        }
    }
    vec3 c = mix(BACKGROUND + kind * quiet, kind, glow);
    if (glow >= 1.0) {
        c = mix(kind, vec3(1.0), 0.3); // fired on the newest tick
    }
    if (over != 0u && (over >> 31) == 0u) {
        c = mix(c, overlay_rgb(over), float((over >> 24) & 127u) / 127.0);
    }

    // a dark gap between cells when they are big enough to see one
    vec2 in_cell = fract(cell) * pc.cell_px;
    if (pc.cell_px >= 5.0 && (in_cell.x < 1.0 || in_cell.y < 1.0)) {
        c = mix(BACKGROUND, c, 0.35);
    }

    // tile and sector lines, on the near edge of each and the far edge of the sheet
    vec2 to_end = (vec2(pc.cells_w, pc.cells_h) - cell) * pc.cell_px;
    bool sheet_edge = to_end.x <= 1.0 || to_end.y <= 1.0;
    vec2 in_tile = mod(px, pc.cell_px * 8.0);
    if ((pc.flags & 1u) != 0u && (in_tile.x < 1.0 || in_tile.y < 1.0 || sheet_edge)) {
        c = vec3(0.30, 0.33, 0.40);
    }
    vec2 in_sector = mod(px, pc.cell_px * 8.0 * float(pc.sector_tiles));
    if ((pc.flags & 2u) != 0u && (in_sector.x < 1.5 || in_sector.y < 1.5 || sheet_edge)) {
        c = vec3(0.62, 0.66, 0.74);
    }
    out_color = vec4(c, 1.0);
}
