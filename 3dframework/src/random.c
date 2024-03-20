#include <random.h>
#include <math.h>

static _Thread_local uint r = 1;

void seed(const uint s) {
    r = s;
}

uint linuxHash(const uint s) {
    return 1103515245 * s + 12345;
}

uint microsoftHash(const uint s) {
    return 214013 * s + 2531011;
}

uint wellons2Hash(uint s) {
    s ^= s >> 16;
    s *= 0x21F0AAADu;
    s ^= s >> 15;
    s *= 0x735A2D97u;
    s ^= s >> 15;
    return s;
}

uint wellons3Hash(uint s) {
    s ^= s >> 17;
    s *= 0xED5AD4BBu;
    s ^= s >> 11;
    s *= 0xAC4C1B51u;
    s ^= s >> 15;
    s *= 0x31848BABu;
    s ^= s >> 14;
    return s;
}

uint randi() {
    return r = wellons3Hash(r);
}

float randf() {
    randi();
    return uint2float(r);
}

float randNormal() {
    static _Thread_local bool hasSpare = 0;
    static _Thread_local float spare;

    if (hasSpare) {
        hasSpare = 0;
        return spare;
    } else {
        float x, y, r;
        do {
            x = 2.0f * randf() - 1.0f;
            y = 2.0f * randf() - 1.0f;
            r = x * x + y * y;
        } while (r >= 1.0f || r == 0.0f);

        float k = sqrtf(-2.0f * logf(r) / r);
        hasSpare = 1;
        spare = k * y;
        return k * x;
    }
}

void getRandomIndices(uint *indices, const uint nbIndices) {
    uint temp[nbIndices];

    for (uint i = 0; i < nbIndices; i++)
        temp[i] = randi();

    for (uint i = 0; i < nbIndices; i++) {
        uint nbInf = 0;
        for (uint j = 0; j < nbIndices; j++)
            if ((j <= i && temp[j] < temp[i]) || (j > i && temp[j] <= temp[i]))
                nbInf++;

        indices[i] = nbInf;
    }
}

float goldenSequence(const uint i) {
    return uint2float(i * 2654435769u);
}

Float4 plasticSequence(const uint i) {
    return float4(uint2float(i * 3242174889u), uint2float(i * 2447445414u));
}

Float4 sequence3(const uint i) {
    return float4(uint2float(i * 3518319155u), uint2float(i * 2882110345u), uint2float(i * 2360945575u));
}

Float4 sequence4(const uint i) {
    return float4(uint2float(i * 3679390609u), uint2float(i * 3152041523u), uint2float(i * 2700274806u), uint2float(i * 2313257605u));
}

Float4 radialTransform(Float4 r) {
    r = 2.0f * r - 1.0f;
    if (r[1] * r[1] > r[0] * r[0])
        r[0] *= 0.125f / r[1];
    else
        r = float4(0.25f - 0.125f * r[1] / r[0], r[0]);
    if (r[1] < 0.0f) r[0] += 0.5f;
    r[1] *= r[1];
    return r;
}
