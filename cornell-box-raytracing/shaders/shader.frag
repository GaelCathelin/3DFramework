#extension GL_EXT_ray_query: require
#extension GL_EXT_ray_tracing_position_fetch: require
#extension GL_EXT_scalar_block_layout: require

#define PI 3.1415927

struct Material {vec3 Kd, Ke;};

layout(binding = 1) uniform _ {
    int frameId, spp;
    ivec2 aa, resolution;
    vec3 Kd, Ke, LightKe;
};
layout(binding = 2) uniform accelerationStructureEXT Accel;
layout(binding = 3) readonly buffer Materials {Material materials[];};
layout(binding = 4) uniform sampler2D Bluenoise;
layout(binding = 5, scalar) buffer Sampling {uint sampling[];};

centroid in vec3 iPos, iNormal;

out vec4 FragColor;

mat3 genTB(vec3 N) {
    float s = N.z < 0.0 ? -1.0 : 1.0;
    float a = -1.0 / (s + N.z);
    float b = N.x * N.y * a;
    return mat3(
        vec3(1.0 + s * N.x * N.x * a, s * b, -s * N.x),
        vec3(b, s + N.y * N.y * a, -N.y),
        N);
}

vec2 radialTransform(vec2 r) {
    r = 2.0 * r - 1.0;
    if (r.y * r.y - r.x * r.x > 0.0)
        r.x *= 0.125 / r.y;
    else
        r = vec2(0.25 - 0.125 * r.y / r.x, r.x);
    if (r.y < 0.0) r.x += 0.5;
    r.y *= r.y;
    return r;
}

uint hash(uint i) {i ^= i >> 16; i *= 0x21F0AAADu; i ^= i >> 15; i *= 0x735A2D97u; i ^= i >> 15; return i;}
uint lcg(uint i) {return 1103515245u * i + 12345u;}

//uint rstate = hash((frameId * resolution.y + uint(aa.y * gl_FragCoord.y)) * resolution.x + uint(aa.x * gl_FragCoord.x));
uint rstate = hash(frameId);
vec3 Noise;

float uint2float(uint i) {return uintBitsToFloat(0x3F800000u | (i >> 9)) - 1.0;}
float rand() {return uint2float((rstate = lcg(rstate)) << 1);}
float goldenSequence(uint i) {return uint2float(i * 2654435769u);}
vec2 plasticSequence(uint i) {return vec2(uint2float(i * 3242174889u), uint2float(i * 2447445414u));}
vec3 cosineSample(vec2 r) {return vec3(cos(2.0 * PI * r.x) * sqrt(r.y), sin(2.0 * PI * r.x) * sqrt(r.y), sqrt(1.0 - r.y));}

vec2 customSequence(uint i, uint j) {
    const uint offset = (20 * 19) / 2 + 2 * j;
    return vec2(uint2float(i * sampling[offset]), uint2float(i * sampling[offset + 1]));
}

vec3 sampleLight(vec3 O, mat3 TBN, vec2 r) {
    // Hardcoded light information (should extract it from mesh)
    const vec3 LightGeom = vec3(0.5, 0.5, 1.99);
    const vec3 LightNormal = vec3(0.0, 0.0, -1.0);

    vec3 P = vec3(LightGeom.xy * (r - 0.5), LightGeom.z);
    vec3 L = P - O;
    float cosNL = dot(TBN[2], L);
    float cosLN2 = dot(-L, LightNormal);
    if (cosNL <= 0.0 || cosLN2 <= 0.0)
        return vec3(0.0);

    rayQueryEXT query;
    rayQueryInitializeEXT(query, Accel, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, O, 1e-4, L, 0.99);
    rayQueryProceedEXT(query);
    return rayQueryGetIntersectionTypeEXT(query, true) != 0 ? vec3(0.0) : cosNL * cosLN2 / (PI * dot(L, L) * dot(L, L)) * LightGeom.x * LightGeom.y * LightKe;
}

vec3 tracePath(vec3 O, mat3 TBN, vec2 r) {
    vec3 Accum = vec3(0.0), Throughput = vec3(1.0);
//    Accum = -sampleLight(O, TBN, r);

    for (int i = 0; i < 9; i++) { // YOLO
        Accum += Throughput * sampleLight(O, TBN, r);

        vec3 D = TBN * cosineSample(r);
        rayQueryEXT query;
        rayQueryInitializeEXT(query, Accel, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, O, 1e-4, D, 1e1);
        rayQueryProceedEXT(query);
        if (rayQueryGetIntersectionTypeEXT(query, true) == 0 || !rayQueryGetIntersectionFrontFaceEXT(query, true))
            return Accum;

        Material mat = materials[rayQueryGetIntersectionGeometryIndexEXT(query, true)];
        float rr = 1.0;//max(mat.Kd.r, max(mat.Kd.g, mat.Kd.b));
        if (fract(Noise.z + rand()) >= rr)
            return Accum;

        Throughput *= mat.Kd / rr;
        O += D * rayQueryGetIntersectionTEXT(query, true);
//        r = fract(Noise.xy + vec2(rand(), rand()));
        r = fract(Noise.xy + customSequence(frameId * spp + i, i + 1));
        vec3 tri[3];
        rayQueryGetIntersectionTriangleVertexPositionsEXT(query, true, tri);
        TBN = genTB(normalize(cross(tri[1] - tri[0], tri[2] - tri[0])));
    }
}

void main() {
    if (Ke != vec3(0.0)) {
        FragColor = vec4(Ke, 1.0);
        return;
    }

    Noise = (texelFetch(Bluenoise, ivec2(aa * gl_FragCoord.xy) & 0xFF, 0).xyz * 255.0 + 0.5) / 256.0;
    mat3 TBN = genTB(normalize(iNormal));

    vec3 Color = vec3(0.0);
    for (uint i = 0; i < spp; i++) {
//        vec2 r = fract(Noise.xy + plasticSequence(frameId * spp + i));
        vec2 r = fract(Noise.xy + customSequence(frameId * spp + i, 0));
//        Color += sampleLight(iPos, TBN, r);
        Color += tracePath(iPos, TBN, r);
    }

    FragColor = vec4(Color * Kd / spp, 1.0);
}
