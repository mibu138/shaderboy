#version 460
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

void main()
{
    const vec2 st = uv * 2.0 - 1.0;
    float A = 1 - step(0.5, length(st));
    float B = 1 - step(0.4, length(st));
    vec3 C = vec3(A, B, 0.0);
    outColor = vec4(C, 1.0);
}
