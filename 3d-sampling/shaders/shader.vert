#define PI 3.1415927

uniform _ {int frame;};

out float pointSize;

float uint2float(uint i) {return uintBitsToFloat(0x3F800000u | (i >> 9)) - 1.0;}
float goldenSequence(uint i) {return uint2float(i * 2654435769u);}
vec2 plasticSequence(uint i) {return vec2(uint2float(i * 3242174889u), uint2float(i * 2447445414u));}
vec3 cosineSample(vec2 r) {return vec3(cos(2.0 * PI * r.x) * sqrt(r.y), sin(2.0 * PI * r.x) * sqrt(r.y), sqrt(1.0 - r.y));}
vec2 radialSample(vec2 r) {return sqrt(r.y) * vec2(cos(2.0 * PI * r.x), sin(2.0 * PI * r.x));};

vec2 r2(uint i) {
    float x = 1.0;
    for (uint j = 0; j < 30; j++)
        x = pow(1.0 + x, 1.0 / 3.0);

    return fract(i / vec2(x, x * x));
}

vec2 radialTransform(vec2 r) {
    r = 2.0 * r - 1.0;
    if (r.y * r.y > r.x * r.x)
        r.x *= 0.125 / r.y;
    else
        r = vec2(0.25 - 0.125 * r.y / r.x, r.x);
    if (r.y < 0.0) r.x += 0.5;
    r.y *= r.y;
    return r;
}

void main() {
//    vec2 r = plasticSequence(gl_InstanceIndex);
//    vec2 r = vec2(goldenSequence(gl_InstanceIndex), (gl_InstanceIndex + 0.5) / 256.0);
//    r = radialTransform(r);
//    vec2 P = radialSample(r);
    vec2 r = r2(gl_InstanceIndex);
    vec2 P = 2.0 * r - 1.0;
    gl_Position = vec4(P, 0.0, 1.0);
	gl_PointSize = pointSize = 3.0 + 0.75;
}
