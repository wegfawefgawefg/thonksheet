#version 450
#extension GL_GOOGLE_include_directive : require
// One quad over pc.rect; the fragment shader works from gl_FragCoord.

#include "common.glsl"

void main() {
    const vec2 corners[6] =
        vec2[](vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 0), vec2(1, 1), vec2(0, 1));
    vec2 px = pc.rect.xy + corners[gl_VertexIndex] * pc.rect.zw;
    gl_Position = vec4(px / pc.extent * 2.0 - 1.0, 0.0, 1.0);
}
