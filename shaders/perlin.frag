#version 460
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

#define PCOUNT 256
#define ROWLEN 16

int permutation[] = { 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 
                          103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 
                          26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 
                          87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 
                          77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 
                          46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 
                          187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 
                          198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 
                          255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 
                          170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 
                          172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 
                          104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 
                          241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 
                          157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 
                          93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180 };

vec2 getGrad(ivec2 gridpos)
{
    int perm = permutation[gridpos.x + gridpos.y * ROWLEN];
    float angle = (float(perm) / 255.0) * PI * 2;
    return vec2(cos(angle), sin(angle));
}

float interp(const float a, const float b, const float w)
{
    float s = smoothstep(0.0, 1.0, w);
    return (b - a) * s  + a;
}

void main()
{
    const vec2  st = uv * 15;
    const float t = parms.time;

    const ivec2 gridPos0 = ivec2(st) + ivec2(0, 0);
    const ivec2 gridPos1 = ivec2(st) + ivec2(1, 0);
    const ivec2 gridPos2 = ivec2(st) + ivec2(0, 1);
    const ivec2 gridPos3 = ivec2(st) + ivec2(1, 1);

    const vec2 grad0 = getGrad(gridPos0);
    const vec2 grad1 = getGrad(gridPos1);
    const vec2 grad2 = getGrad(gridPos2);
    const vec2 grad3 = getGrad(gridPos3);

    vec2 gridSt0 = (st - gridPos0);
    vec2 gridSt1 = (st - gridPos1);
    vec2 gridSt2 = (st - gridPos2);
    vec2 gridSt3 = (st - gridPos3);

    const float dot0 = dot(gridSt0, grad0);
    const float dot1 = dot(gridSt1, grad1);
    const float dot2 = dot(gridSt2, grad2);
    const float dot3 = dot(gridSt3, grad3);

    vec2 sp = gridSt0;

    const float o0 =  interp(dot0, dot1, sp.x);
    const float o1 =  interp(dot2, dot3, sp.x);
    float val = interp(o0, o1, sp.y);
    val = val * 0.5 + 0.5;
    val = pow(val, 4);

    vec4 color = vec4(val, val, val, 1.0);
    outColor = color;
}
