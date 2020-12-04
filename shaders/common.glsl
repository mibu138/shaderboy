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

float sdfLineSegment(const vec2 A, const vec2 B, const vec2 P)
{
    const float h = min(1, max(0, dot(P - A, B - A) / pow(length(B - A), 2)));
    return length(P - A - h * (B - A));
}

float mul(const float a, const float b) 
{ 
    return a * b; 
}

// a: angle between segments
// b: difference in length between segments
// c: normalized distance to the cross on segment 1
//    normalized distance to the cross on segment 2 is always 0.5
float sdfSplat(const float a, const float b, const float c, const vec2 P)
{
    const vec2 A = vec2(b * cos(a), b * sin(a));
    const vec2 B = vec2(0, c);
    const vec2 A_ = -1 * A;
    const vec2 B_ = vec2(0, c - 1);
    const float sdfA = sdfLineSegment(A, A_, P);
    const float sdfB = sdfLineSegment(B, B_, P);
    return sdfA * sdfB;
}

vec2 rotate(const float a, const vec2 P)
{
    const mat2 m = mat2(cos(a), -1 * sin(a),
                        sin(a), cos(a));
    return m * P;
}

vec2 xformSdf(const float r, const float s, const vec2 T, vec2 P)
{
    P -= T;
    P *= 1.0/s;
    P = rotate(r, P);
    return P;
}
