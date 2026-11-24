#version 450

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_local;
layout(location = 2) in vec4 v_shape;
layout(location = 0) out vec4 out_color;

void main() {
    // a ring keeps only its border
    if (v_shape.w > 0.5 && v_shape.w < 1.5) {
        vec2 to_edge = min(v_local, v_shape.xy - v_local);
        if (min(to_edge.x, to_edge.y) > v_shape.z) {
            discard;
        }
    }
    out_color = v_color;
}
