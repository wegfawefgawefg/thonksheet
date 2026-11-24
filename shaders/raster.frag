#version 450
#extension GL_GOOGLE_include_directive : require
// The raster: time across (newest at the right), one row per neuron or per tile down.
// When a pixel covers several rows it shows the share of them that fired, against the
// target rate, coloured by the kinds that fired.

#include "common.glsl"

layout(location = 0) out vec4 out_color;

const uint MOST_ROWS = 64u; // rows looked at per pixel; past that they are sampled

void main() {
    vec2 local = gl_FragCoord.xy - pc.rect.xy;
    uint shown = max(pc.rows_valid, 1u);
    uint column = min(uint(local.x / pc.rect.z * float(shown)), shown - 1u);
    uint age = shown - 1u - column;
    uint row_base = tape_row(age) * pc.tile_count;

    // the rows under this pixel
    float first = pc.origin.x;
    float per_px = pc.origin.y;
    float r0 = first + floor(local.y) * per_px;
    uint a = uint(r0);
    uint b = max(a + 1u, uint(ceil(r0 + per_px)));
    uint total = pc.mode == 0u ? pc.tile_count * 64u : pc.tile_count;
    b = min(b, total);
    if (a >= total) {
        out_color = vec4(BACKGROUND, 1.0);
        return;
    }
    uint step = max(1u, (b - a + MOST_ROWS - 1u) / MOST_ROWS);

    vec3 sum = vec3(0.0);
    float fired = 0.0;
    float looked = 0.0;
    for (uint r = a; r < b; r += step) {
        if (pc.mode == 0u) {
            uint tile = r / 64u;
            uint bit = r % 64u;
            looked += 1.0;
            if (bit_set(tape[row_base + tile], bit)) {
                fired += 1.0;
                sum += kind_color(tile, bit);
            }
        } else {
            uvec2 w = tape[row_base + r];
            uvec2 inh = kinds[r * 3u];
            uvec2 sen = kinds[r * 3u + 1u];
            uvec2 mot = kinds[r * 3u + 2u];
            float s = float(ones(w & sen));
            float m = float(ones(w & mot & ~sen));
            float i = float(ones(w & inh & ~sen & ~mot));
            float e = float(ones(w)) - s - m - i;
            looked += 64.0;
            fired += e + i + s + m;
            sum += e * EXCITATORY + i * INHIBITORY + s * SENSOR + m * MOTOR;
        }
    }
    float share = fired / max(looked, 1.0);
    // brightness against the target rate (pc.fade): on target a pixel is a third lit, three
    // times over nearly full, dead rows black
    float bright = 1.0 - exp(-share / (2.5 * max(pc.fade, 0.002)));
    vec3 c = fired > 0.0 ? mix(BACKGROUND, sum / fired, bright) : BACKGROUND;

    // a faint line between tiles when neuron rows are tall enough to tell apart
    float row_f = first + local.y * per_px;
    if (pc.mode == 0u && per_px <= 0.5 && fract(row_f / 64.0) * 64.0 < per_px) {
        c = max(c, vec3(0.16, 0.18, 0.22));
    }
    // the selected neuron's row: a mark at the left edge and a faint band across
    if (pc.selected >= 0) {
        uint sel = pc.mode == 0u ? uint(pc.selected) : uint(pc.selected) / 64u;
        if (sel >= a && sel < b) {
            c = local.x < 6.0 ? vec3(1.0) : max(c, vec3(0.13, 0.14, 0.17));
        }
    }
    out_color = vec4(c, 1.0);
}
