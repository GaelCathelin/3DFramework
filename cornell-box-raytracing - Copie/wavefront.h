#pragma once

#include <engine.h>

typedef enum {
    GEOMETRY_ONLY    = 1,
    INVERT_WINDING   = 2,
    RECALC_NORMALS   = 4,
    Y_UP             = 8,
    COMPUTE_TANGENTS = 16,
} WavefrontFlag;

typedef struct {
    char name[64];
    Float4 Kd, Ks, Ke, Tf;
    float Ns, Ni, d;
    int illum;
    Texture diffuse, specular, bump;
} Material;

typedef struct {
    Range range;
    Material *material;
} SubMesh;

typedef struct {
    uint nbMeshes, nbMaterials;
    SubMesh *meshes;
    Material *materials;
} GPUMeshData;

typedef struct {
    uint *indices, nbIndices, nbElems;
    bool hasNormals, hasTexCoords, hasTangents;
    float (*vertices)[3], (*texCoords)[2], (*normals)[3], (*tangents)[3];
} WavefrontMesh;

WavefrontMesh loadWavefrontMesh(const char *path, const char *fileName, const WavefrontFlag flags, GPUMeshData *gpuData) WARN_UNUSED_RESULT;
Mesh loadWavefront(const char *path, const char *fileName, const WavefrontFlag flags, GPUMeshData *gpuData) WARN_UNUSED_RESULT;
Mesh createGPUMesh(const WavefrontMesh mesh) WARN_UNUSED_RESULT;
void deleteWavefrontMesh(WavefrontMesh *mesh);
void deleteGPUMeshData(GPUMeshData *gpuData);