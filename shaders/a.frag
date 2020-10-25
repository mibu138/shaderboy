#version 460
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

#define NCOUNT 6

const vec2 nodes[NCOUNT] = {
    {0.4, 0.2},
    {0.3, 0.3},
    {0.7, 0.1},
    {0.1, 0.5},
    {0.3, 0.9},
    {0.2, 0.7},
};

float fn0(const vec2 st)
{
    const float t = parms.time;
    const float r = length(st);
    const float angle = getAngle(st, r);
    const float sintime = sin(t);
    const float sinrad  = sin(r);
    const float o = r + sintime * .25;
    const float v = sin(angle * t + t * 3) * 0.5 + 0.5;
    return o;
}

void main()
{
    float o = 1.0;
    vec4 color = {0.0, 0.0, 0.0, 1.0};
    for (int i = 0; i < NCOUNT; i++)
    {
        const vec2 pos = uv - nodes[i];
        const float r = fn0(pos);
        o = min(o, r);
        color = over(vec4(r,r,r,1.0), color);
    }
    outColor = vec4(o, o, o, 1.0);
}
