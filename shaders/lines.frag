#version 460
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

float avg(const float a, const float b)
{
    return (a + b) / 2;
}

float solidify(float edge, float v)
{
    return 1.0 - step(edge, v);
}

layout(location = 0) in vec2 st;

layout(location = 0) out vec4 outColor;

#define r 1
#define g 0.2
#define b 0.1

void main()
{
    vec2 uv = st;
    uv.y = uv.y * -1 + 1;

    const float t = parms.time;
    float v, w, x, y;
    v = sdfSplat(t, 0.8, 0.5, xformSdf(0.2 * t, 0.5, vec2(0.5, 0.5), uv));
    w = sdfSplat(t, 0.8, 0.5, xformSdf(0.5 * t, 0.6, vec2(0.5, 0.5), uv));
    x = sdfSplat(t * 2, 0.8, 0.5, xformSdf(0.6 * t, 0.3, vec2(0.25, 0.75), uv));
    y = sdfSplat(t * 1.5, 0.8, 0.5, xformSdf(0.5 * -t, 0.3, vec2(0.75, 0.45), uv));
    vec4 Cv = vec4(0.1, 0.4, 0.6, solidify(0.01, v));
    vec4 Cw = vec4(0.4, 0.2, 0.8, solidify(0.01, w));
    vec4 Cx = vec4(0.1, 0.9, 0.6, solidify(0.1, x));
    vec4 Cy = vec4(0.2, 0.4, 0.9, solidify(0.1, y));
    vec4 Co = over(Cw, Cv);
    outColor = Co;
}
