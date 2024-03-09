#pragma once

#include <vector.h>

#ifdef __cplusplus
extern "C" {
#endif

uint linuxHash(const uint s);
uint microsoftHash(const uint s);
uint wellons2Hash(uint s);
uint wellons3Hash(uint s);

void seed(const uint s);
uint randi();
float randf();
float randNormal();

void getRandomIndices(uint *indices, const uint nbIndices);

float goldenSequence(const uint i);
Float4 plasticSequence(const uint i);
Float4 sequence3(const uint i);
Float4 sequence4(const uint i);

Float4 radialTransform(Float4 r);

#ifdef __cplusplus
}
#endif
