#define PI 3.141592653589793

layout(set = 0, binding = 0) uniform Parms {
    float time;
} parms;

vec4 over(const vec4 a, const vec4 b)
{
    const vec3 color = a.rgb + b.rgb * (1. - a.a);
    const float alpha = a.a + b.a * (1. - a.a);
    return vec4(color, alpha);
}

float getAngle(const vec2 st, const float r)
{
    if (r < 0.0001) return 0.0;
    const float x = st.x;
    const float y = st.y;
    const float v = x / r;
    if (y >= 0) return acos(v);
    else return -1 * acos(v);
}

vec2 rotVec2(const float angle, const vec2 v)
{
    mat2 rot = mat2(
        cos(angle), -sin(angle),
        sin(angle),  cos(angle)
        );
    return rot * v;
}
