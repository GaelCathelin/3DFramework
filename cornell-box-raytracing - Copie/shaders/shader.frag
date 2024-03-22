#extension GL_EXT_ray_query: require
#extension GL_EXT_ray_tracing_position_fetch: require
#define ever (;;)

#define PI 3.1415927

struct Material {vec3 Kd, Ke;};

layout(binding = 1) uniform _ {
    int frameId, spp, mode;
    ivec2 aa, resolution;
    vec3 Kd, Ke, LightKe;
};
layout(binding = 2) uniform accelerationStructureEXT Accel;
layout(binding = 3) readonly buffer Materials {Material materials[];};
layout(binding = 4) uniform sampler2D Bluenoise;

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

vec3 tracePath(vec3 O, mat3 TBN, vec2 r) {
    vec3 Throughput = vec3(1.0);
    for ever { // YOLO
        vec3 D = TBN * cosineSample(r);
        rayQueryEXT query;
        rayQueryInitializeEXT(query, Accel, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, O, 1e-4, D, 1e1);
        rayQueryProceedEXT(query);
        if (rayQueryGetIntersectionTypeEXT(query, true) == 0 || !rayQueryGetIntersectionFrontFaceEXT(query, true))
            return vec3(0.0);

        Material mat = materials[rayQueryGetIntersectionGeometryIndexEXT(query, true)];
        if (mat.Ke != vec3(0.0))
            return Throughput * mat.Ke;

        float russianRoulette = max(mat.Kd.r, max(mat.Kd.g, mat.Kd.b));
        if (fract(Noise.z + rand()) >= russianRoulette)
            return vec3(0.0);

        Throughput *= mat.Kd / russianRoulette;
        O += D * rayQueryGetIntersectionTEXT(query, true);
        r = fract(Noise.xy + vec2(rand(), rand()));
        vec3 tri[3];
        rayQueryGetIntersectionTriangleVertexPositionsEXT(query, true, tri);
        TBN = genTB(normalize(cross(tri[1] - tri[0], tri[2] - tri[0])));
    }
}

vec3 traceDirect(vec3 O, mat3 TBN, vec2 r) {
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

vec3 tracePathNEE(vec3 O, mat3 TBN, vec2 r) {
    vec3 Accum = vec3(0.0), Throughput = vec3(1.0);
    for ever { // YOLO
        Accum += Throughput * traceDirect(O, TBN, r);

        vec3 D = TBN * cosineSample(r);
        rayQueryEXT query;
        rayQueryInitializeEXT(query, Accel, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, O, 1e-4, D, 1e1);
        rayQueryProceedEXT(query);
        if (rayQueryGetIntersectionTypeEXT(query, true) == 0 || !rayQueryGetIntersectionFrontFaceEXT(query, true))
            return Accum;

        Material mat = materials[rayQueryGetIntersectionGeometryIndexEXT(query, true)];
        float russianRoulette = max(mat.Kd.r, max(mat.Kd.g, mat.Kd.b));
        if (fract(Noise.z + rand()) >= russianRoulette)
            return Accum;

        Throughput *= mat.Kd / russianRoulette;
        O += D * rayQueryGetIntersectionTEXT(query, true);
        r = fract(Noise.xy + vec2(rand(), rand()));
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
        vec2 r = fract(Noise.xy + plasticSequence(frameId * spp + i));
        switch (mode) {
            case 0: Color += traceDirect (iPos, TBN, r); break;
            case 1: Color += tracePath   (iPos, TBN, r); break;
            case 2: Color += tracePathNEE(iPos, TBN, r); break;
        }
    }

    FragColor = vec4(Color * Kd / spp, 1.0);
}
