#version 450
#extension GL_GOOGLE_include_directive : require
// Recurrence: pixel (i, j) is how alike tick i and tick j were (shared spikes over all
// spikes), oldest at the top left. Grey says how alike at all; amber, more alike than the
// usual pair (pc.fade). The diagonal is every tick against itself. Lines parallel to it
// mean activity repeats with that period; a flat bright square means it is stuck in one
// pattern; plain dark means each tick is new. Dark red: both ticks silent.

#include "common.glsl"

layout(location = 0) out vec4 out_color;

void main() {
    vec2 local = gl_FragCoord.xy - pc.rect.xy;
    uint n = max(pc.mode, 1u);
    uint i = min(uint(local.x / pc.rect.z * float(n)), n - 1u);
    uint j = min(uint(local.y / pc.rect.w * float(n)), n - 1u);
    uint row_i = tape_row(n - 1u - i) * pc.tile_count;
    uint row_j = tape_row(n - 1u - j) * pc.tile_count;
    uint both = 0u;
    uint either = 0u;
    for (uint w = 0u; w < pc.tile_count; w += pc.word_step) {
        uvec2 x = tape[row_i + w];
        uvec2 y = tape[row_j + w];
        both += ones(x & y);
        either += ones(x | y);
    }
    vec3 c;
    if (either == 0u) {
        c = vec3(0.30, 0.05, 0.06);
    } else {
        // dim by how alike they are at all, bright where more alike than usual
        float alike = float(both) / float(either);
        float usual = clamp(pc.fade, 0.0, 0.95);
        float above = clamp((alike - usual) / (1.0 - usual), 0.0, 1.0);
        c = BACKGROUND + vec3(0.30, 0.28, 0.25) * pow(alike, 0.6);
        c = mix(c, vec3(1.0, 0.86, 0.55), pow(above, 0.7));
    }
    out_color = vec4(c, 1.0);
}
