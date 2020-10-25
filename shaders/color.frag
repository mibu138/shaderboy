#version 460
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

void main()
{
    const vec2 st = uv * 2.0 - 1.0;
    float r = length(st);
    r += sin(parms.time) * .25;
    outColor = vec4(r, r /2, 0.0, 1.0);
}
