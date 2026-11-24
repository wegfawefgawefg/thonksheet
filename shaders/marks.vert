#version 450
#extension GL_GOOGLE_include_directive : require
// Marks over the sheet for the selected neuron: rings round its sources, dots on its
// targets, lines for far links. One instance per mark, six vertices each.
// Mark.a: a rect (x0, y0, x1, y1) or a line (from, to), in cells.
// Mark.p.x: 0 filled rect, 1 ring, 2 line. Mark.p.y: width in pixels.

#include "common.glsl"

struct Mark {
    vec4 a;
    vec4 color;
    vec4 p;
};
layout(std430, set = 0, binding = 2) readonly buffer Marks {
    Mark marks[];
};

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_local; // pixels from the mark's top left (rect and ring)
layout(location = 2) out vec4 v_shape; // size in pixels, ring width, type

void main() {
    const vec2 corners[6] =
        vec2[](vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 0), vec2(1, 1), vec2(0, 1));
    vec2 c = corners[gl_VertexIndex];
    Mark m = marks[gl_InstanceIndex];
    // never wider than a third of a cell, so small cells still show which one is meant
    float width = min(m.p.y, max(1.0, pc.cell_px * 0.34));
    vec2 px;
    if (m.p.x < 1.5) {
        // rect or ring: the cell span, grown by the ring width so it sits round the cells
        vec2 lo = pc.origin + m.a.xy * pc.cell_px - vec2(m.p.x > 0.5 ? width : 0.0);
        vec2 hi = pc.origin + m.a.zw * pc.cell_px + vec2(m.p.x > 0.5 ? width : 0.0);
        px = mix(lo, hi, c);
        v_local = px - lo;
        v_shape = vec4(hi - lo, width, m.p.x);
    } else {
        // line: a quad along the segment
        vec2 p0 = pc.origin + m.a.xy * pc.cell_px;
        vec2 p1 = pc.origin + m.a.zw * pc.cell_px;
        vec2 along = p1 - p0;
        vec2 dir = length(along) > 0.001 ? normalize(along) : vec2(1.0, 0.0);
        vec2 side = vec2(-dir.y, dir.x) * width * 0.5;
        px = mix(p0, p1, c.x) + side * (c.y * 2.0 - 1.0);
        v_local = vec2(0.0);
        v_shape = vec4(0.0, 0.0, width, 2.0);
    }
    v_color = m.color;
    gl_Position = vec4(px / pc.extent * 2.0 - 1.0, 0.0, 1.0);
}
