#version 460
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

void main()
{
    const vec2 st = uv * 2.0 - 1.0;
    const float t = parms.time;
    const float r = length(st);
    const float angle = getAngle(st, r);
    const float sintime = sin(t);
    const float sinrad  = sin(r);
    const float o = r + sintime * .25;
    const float v = sin(angle * t + t * 3) * 0.5 + 0.5;
    outColor = vec4(o, v, 0.0, 1.0);
}
