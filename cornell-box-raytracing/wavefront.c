#include "wavefront.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <SDL2/SDL_image.h>

typedef enum {
    VERTEX_INDEXED,
    TEXCOORD_INDEXED,
    NORMAL_INDEXED,
    NO_INDICES,
} IndexByAttrib;

static void loadMaterials(const char *path, const char *fileName, Material **materials, uint *nbMaterials) {
    char fullName[256];
    sprintf(fullName, "%s%s", path, fileName);
    FILE *file = fopen(fullName, "r");
    if (file == NULL) {
        printf("Can't open material '%s'\n", fullName);
        return;
    }

    char line[256];
    *nbMaterials = 0;
    while (fgets(line, 256, file)) {
        char str[192];
        Float4 color = {};
        int illum;
        if (sscanf(line, "newmtl %s", str) == 1) {
            *materials = realloc(*materials, (*nbMaterials + 1) * sizeof(Material));
            memset(&(*materials)[*nbMaterials], 0, sizeof(Material));
            strcpy((*materials)[*nbMaterials].name, str);
            (*materials)[*nbMaterials].illum = 2;
            (*materials)[*nbMaterials].Kd = float1(1.0f);
            (*materials)[*nbMaterials].Tf = float1(1.0f);
            (*materials)[*nbMaterials].Ns = 100.0f;
            (*materials)[*nbMaterials].Ni = 1.5f;
            ++*nbMaterials;
        } else if (sscanf(line, " map_Kd %s", str) == 1) {
            sprintf(fullName, "%s%s", path, str);
            (*materials)[*nbMaterials - 1].diffuse = loadTexture(fullName, SRGB_FLAG | MIPMAPS_FLAG);
            getSampler((*materials)[*nbMaterials - 1].diffuse)->wrapU = getSampler((*materials)[*nbMaterials - 1].diffuse)->wrapV = WrapMode_Repeat;
            getSampler((*materials)[*nbMaterials - 1].diffuse)->anisotropy = 16.0f;
        } else if (sscanf(line, " map_%*cump %s", str) == 1) {
            sprintf(fullName, "%s%s", path, str);
            (*materials)[*nbMaterials - 1].bump = loadTexture(fullName, MIPMAPS_FLAG);
            getSampler((*materials)[*nbMaterials - 1].bump)->wrapU = getSampler((*materials)[*nbMaterials - 1].bump)->wrapV = WrapMode_Repeat;
            getSampler((*materials)[*nbMaterials - 1].bump)->anisotropy = 16.0f;
        }
        else if (sscanf(line, " illum %d", &illum) == 1) (*materials)[*nbMaterials - 1].illum = illum;
        else if (sscanf(line, " Kd %f %f %f", &color[0], &color[1], &color[2]) == 3) (*materials)[*nbMaterials - 1].Kd = color;
        else if (sscanf(line, " Ks %f %f %f", &color[0], &color[1], &color[2]) == 3) (*materials)[*nbMaterials - 1].Ks = color;
        else if (sscanf(line, " Ke %f %f %f", &color[0], &color[1], &color[2]) == 3) (*materials)[*nbMaterials - 1].Ke = color;
        else if (sscanf(line, " Tf %f %f %f", &color[0], &color[1], &color[2]) == 3) (*materials)[*nbMaterials - 1].Tf = color;
        else if (sscanf(line, " Ns %f", &color[0]) == 1) (*materials)[*nbMaterials - 1].Ns = color[0];
        else if (sscanf(line, " Ni %f", &color[0]) == 1) (*materials)[*nbMaterials - 1].Ni = color[0];
        else if (sscanf(line, " d %f", &color[0]) == 1) (*materials)[*nbMaterials - 1].d = color[0];
    }

    fclose(file);
}

static void readFloats(const char *str, const int nbFloats, ...) {
    va_list params;
    va_start(params, nbFloats);

    for (int i = 0; i < nbFloats; i++) {
        char *end;
        *va_arg(params, float*) = strtof(str, &end);
        assert(end != str);
        str = end;
    }

    va_end(params);
}

WavefrontMesh loadWavefrontMesh(const char *path, const char *fileName, const WavefrontFlag flags, GPUMeshData *gpuData) {
    WavefrontMesh mesh = {};
    float (*indexedVertices)[3], (*indexedNormals)[3], (*indexedTexCoords)[2];
    uint vSize, vnSize, vtSize, iSize, eSize, nbElems[4] = {};

    char fullName[128];
    sprintf(fullName, "%s%s", path, fileName);
    FILE *file = fopen(fullName, "r");
    if (file == NULL) {
        printf("Can't open model '%s'\n", fullName);
        return mesh;
    }

    vSize = vnSize = vtSize = iSize = eSize = 12;

    mesh.indices     = malloc(iSize  * sizeof(uint    ));
    indexedVertices  = malloc(vSize  * sizeof(float[3]));
    indexedTexCoords = malloc(vtSize * sizeof(float[2]));
    indexedNormals   = malloc(vnSize * sizeof(float[3]));
    mesh.vertices    = malloc(eSize  * sizeof(float[3]));
    mesh.texCoords   = malloc(eSize  * sizeof(float[2]));
    mesh.normals     = malloc(eSize  * sizeof(float[3]));
    mesh.tangents    = malloc(eSize  * sizeof(float[3]));

    char line[128];
    while (fgets(line, 128, file)) {
        uint index[4][4] = {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}, readVertices = 0;
        char mtl[96];

        if (nbElems[VERTEX_INDEXED  ] == vSize ) indexedVertices  = realloc(indexedVertices , (vSize  <<= 1) * sizeof(float[3]));
        if (nbElems[TEXCOORD_INDEXED] == vtSize) indexedTexCoords = realloc(indexedTexCoords, (vtSize <<= 1) * sizeof(float[2]));
        if (nbElems[NORMAL_INDEXED  ] == vnSize) indexedNormals   = realloc(indexedNormals  , (vnSize <<= 1) * sizeof(float[3]));
        if (mesh.nbIndices + 6 >= iSize) mesh.indices = realloc(mesh.indices, (iSize <<= 1) * sizeof(uint));

        if (line[0] == 'v') {
            if      (line[1] == ' ') {readFloats(line + 2, 3, &indexedVertices [nbElems[VERTEX_INDEXED  ]][0], &indexedVertices [nbElems[VERTEX_INDEXED  ]][1], &indexedVertices[nbElems[VERTEX_INDEXED]][2]); nbElems[VERTEX_INDEXED  ]++;}
            else if (line[1] == 'n') {readFloats(line + 3, 3, &indexedNormals  [nbElems[NORMAL_INDEXED  ]][0], &indexedNormals  [nbElems[NORMAL_INDEXED  ]][1], &indexedNormals [nbElems[NORMAL_INDEXED]][2]); nbElems[NORMAL_INDEXED  ]++;}
            else if (line[1] == 't') {readFloats(line + 3, 2, &indexedTexCoords[nbElems[TEXCOORD_INDEXED]][0], &indexedTexCoords[nbElems[TEXCOORD_INDEXED]][1]                                              ); nbElems[TEXCOORD_INDEXED]++;}
        } else if (line[0] == 'f') {
            if      (sscanf(line + 2, "%u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u", &index[0][0], &index[1][0], &index[2][0], &index[0][1], &index[1][1], &index[2][1], &index[0][2], &index[1][2], &index[2][2], &index[0][3], &index[1][3], &index[2][3]) == 12) readVertices = 4;
            else if (sscanf(line + 2, "%u//%u %u//%u %u//%u %u//%u"        , &index[0][0]              , &index[2][0], &index[0][1]              , &index[2][1], &index[0][2]              , &index[2][2], &index[0][3]              , &index[2][3]) ==  8) readVertices = 4;
            else if (sscanf(line + 2, "%u/%u %u/%u %u/%u %u/%u"            , &index[0][0], &index[1][0]              , &index[0][1], &index[1][1]              , &index[0][2], &index[1][2]              , &index[0][3], &index[1][3]              ) ==  8) readVertices = 4;
            else if (sscanf(line + 2, "%u %u %u %u"                        , &index[0][0]                            , &index[0][1]                            , &index[0][2]                            , &index[0][3]                            ) ==  4) readVertices = 4;
            else if (sscanf(line + 2, "%u/%u/%u %u/%u/%u %u/%u/%u"         , &index[0][0], &index[1][0], &index[2][0], &index[0][1], &index[1][1], &index[2][1], &index[0][2], &index[1][2], &index[2][2]                                          ) ==  9) readVertices = 3;
            else if (sscanf(line + 2, "%u//%u %u//%u %u//%u"               , &index[0][0]              , &index[2][0], &index[0][1]              , &index[2][1], &index[0][2]              , &index[2][2]                                          ) ==  6) readVertices = 3;
            else if (sscanf(line + 2, "%u/%u %u/%u %u/%u"                  , &index[0][0], &index[1][0]              , &index[0][1], &index[1][1]              , &index[0][2], &index[1][2]                                                        ) ==  6) readVertices = 3;
            else if (sscanf(line + 2, "%u %u %u"                           , &index[0][0]                            , &index[0][1]                            , &index[0][2]                                                                      ) ==  3) readVertices = 3;
        } else if (gpuData && sscanf(line, "mtllib %s", mtl) == 1) {
            loadMaterials(path, mtl, &gpuData->materials, &gpuData->nbMaterials);
        } else if (gpuData && sscanf(line, "usemtl %s", mtl) == 1) {
            uint currMtl = 0;
            while (currMtl < gpuData->nbMaterials && strcmp(gpuData->materials[currMtl].name, mtl))
                currMtl++;

            gpuData->meshes = realloc(gpuData->meshes, (gpuData->nbMeshes + 1) * sizeof(SubMesh));
            gpuData->meshes[gpuData->nbMeshes].material = &gpuData->materials[currMtl];
            gpuData->meshes[gpuData->nbMeshes].range.first = mesh.nbIndices;
            if (gpuData->nbMeshes > 0)
                gpuData->meshes[gpuData->nbMeshes - 1].range.count = mesh.nbIndices - gpuData->meshes[gpuData->nbMeshes - 1].range.first;
            gpuData->nbMeshes++;
        }

        if (readVertices >= 3) {
            for (uint i = 0; i < readVertices; i++) index[NO_INDICES][i] = ++nbElems[NO_INDICES];

            if (nbElems[NO_INDICES] + readVertices >= eSize) {
                eSize *= 2;
                mesh.vertices  = realloc(mesh.vertices , eSize * sizeof(float[3]));
                mesh.texCoords = realloc(mesh.texCoords, eSize * sizeof(float[2]));
                mesh.normals   = realloc(mesh.normals  , eSize * sizeof(float[3]));
                mesh.tangents  = realloc(mesh.tangents , eSize * sizeof(float[3]));
            }

            VERIFY(mesh.indices && mesh.vertices && mesh.texCoords && mesh.normals);

            mesh.indices[mesh.nbIndices++] = index[NO_INDICES][0] - 1;
            mesh.indices[mesh.nbIndices++] = index[NO_INDICES][1] - 1;
            mesh.indices[mesh.nbIndices++] = index[NO_INDICES][2] - 1;

            memcpy(&mesh.vertices[index[NO_INDICES][0] - 1], &indexedVertices[index[0][0] - 1], sizeof(float[3]));
            memcpy(&mesh.vertices[index[NO_INDICES][1] - 1], &indexedVertices[index[0][1] - 1], sizeof(float[3]));
            memcpy(&mesh.vertices[index[NO_INDICES][2] - 1], &indexedVertices[index[0][2] - 1], sizeof(float[3]));

            memcpy(&mesh.texCoords[index[NO_INDICES][0] - 1], &indexedTexCoords[index[1][0] - 1], sizeof(float[2]));
            memcpy(&mesh.texCoords[index[NO_INDICES][1] - 1], &indexedTexCoords[index[1][1] - 1], sizeof(float[2]));
            memcpy(&mesh.texCoords[index[NO_INDICES][2] - 1], &indexedTexCoords[index[1][2] - 1], sizeof(float[2]));

            memcpy(&mesh.normals[index[NO_INDICES][0] - 1], &indexedNormals[index[2][0] - 1], sizeof(float[3]));
            memcpy(&mesh.normals[index[NO_INDICES][1] - 1], &indexedNormals[index[2][1] - 1], sizeof(float[3]));
            memcpy(&mesh.normals[index[NO_INDICES][2] - 1], &indexedNormals[index[2][2] - 1], sizeof(float[3]));
        }

        if (readVertices >= 4) {
            mesh.indices[mesh.nbIndices++] = index[NO_INDICES][0] - 1;
            mesh.indices[mesh.nbIndices++] = index[NO_INDICES][2] - 1;
            mesh.indices[mesh.nbIndices++] = index[NO_INDICES][3] - 1;

            memcpy(&mesh.vertices [index[NO_INDICES][3] - 1], &indexedVertices [index[0][3] - 1], sizeof(float[3]));
            memcpy(&mesh.texCoords[index[NO_INDICES][3] - 1], &indexedTexCoords[index[1][3] - 1], sizeof(float[2]));
            memcpy(&mesh.normals  [index[NO_INDICES][3] - 1], &indexedNormals  [index[2][3] - 1], sizeof(float[3]));
        }
    }
    putchar('\n');

    if (gpuData && gpuData->nbMeshes > 0)
        gpuData->meshes[gpuData->nbMeshes - 1].range.count = mesh.nbIndices - gpuData->meshes[gpuData->nbMeshes - 1].range.first;

    free(indexedNormals  );
    free(indexedTexCoords);
    free(indexedVertices );
    fclose(file);

    const float lightweight = (flags & GEOMETRY_ONLY) != 0;
    mesh.nbElems = nbElems[NO_INDICES];
    if (!lightweight) {
        mesh.hasTexCoords = nbElems[TEXCOORD_INDEXED] > 0;
        mesh.hasNormals   = nbElems[NORMAL_INDEXED  ] > 0;
    }

    if (flags & INVERT_WINDING)
        for (uint i = 0; i < mesh.nbIndices; i += 3)
            SWAP(mesh.indices[i + 1], mesh.indices[i + 2]);

    if (mesh.normals && !lightweight && (!mesh.hasNormals || (flags & RECALC_NORMALS))) {
        mesh.hasNormals = true;
        memset(mesh.normals, 0, mesh.nbElems * sizeof(float[3]));

        for (uint i = 0; i < mesh.nbIndices; i += 3) {
            const float AB[3] = {
                mesh.vertices[mesh.indices[i + 1]][0] - mesh.vertices[mesh.indices[i]][0],
                mesh.vertices[mesh.indices[i + 1]][1] - mesh.vertices[mesh.indices[i]][1],
                mesh.vertices[mesh.indices[i + 1]][2] - mesh.vertices[mesh.indices[i]][2],
            };
            const float AC[3] = {
                mesh.vertices[mesh.indices[i + 2]][0] - mesh.vertices[mesh.indices[i]][0],
                mesh.vertices[mesh.indices[i + 2]][1] - mesh.vertices[mesh.indices[i]][1],
                mesh.vertices[mesh.indices[i + 2]][2] - mesh.vertices[mesh.indices[i]][2],
            };
            const float N[3] = {
                AB[1] * AC[2] - AB[2] * AC[1],
                AB[2] * AC[0] - AB[0] * AC[2],
                AB[0] * AC[1] - AB[1] * AC[0],
            };
            mesh.normals[mesh.indices[i    ]][0] += N[0]; mesh.normals[mesh.indices[i    ]][1] += N[1]; mesh.normals[mesh.indices[i    ]][2] += N[2];
            mesh.normals[mesh.indices[i + 1]][0] += N[0]; mesh.normals[mesh.indices[i + 1]][1] += N[1]; mesh.normals[mesh.indices[i + 1]][2] += N[2];
            mesh.normals[mesh.indices[i + 2]][0] += N[0]; mesh.normals[mesh.indices[i + 2]][1] += N[1]; mesh.normals[mesh.indices[i + 2]][2] += N[2];
        }

        for (uint i = 0; i < mesh.nbElems; i++) {
            const float d = inversesqrt(
                mesh.normals[i][0] * mesh.normals[i][0] +
                mesh.normals[i][1] * mesh.normals[i][1] +
                mesh.normals[i][2] * mesh.normals[i][2]);
            mesh.normals[i][0] *= d;
            mesh.normals[i][1] *= d;
            mesh.normals[i][2] *= d;
        }
    }

    if (flags & Y_UP) {
        for (uint i = 0; i < mesh.nbElems; i++) {
            const float x = mesh.vertices[i][0];
            const float y = mesh.vertices[i][1];
            const float z = mesh.vertices[i][2];
            mesh.vertices[i][0] = -z;
            mesh.vertices[i][1] = -x;
            mesh.vertices[i][2] = y;
        }

        if (mesh.hasNormals) {
            for (uint i = 0; i < mesh.nbElems; i++) {
                const float x = mesh.normals[i][0];
                const float y = mesh.normals[i][1];
                const float z = mesh.normals[i][2];
                mesh.normals[i][0] = -z;
                mesh.normals[i][1] = -x;
                mesh.normals[i][2] = y;
            }
        }
    }

    if ((flags & COMPUTE_TANGENTS) && mesh.hasTexCoords && mesh.hasNormals) {
        mesh.hasTangents = true;
        memset(mesh.tangents, 0, mesh.nbElems * sizeof(float[3]));

        for (uint i = 0; i < mesh.nbIndices; i += 3) {
            const float AB[3] = {
                mesh.vertices[mesh.indices[i + 1]][0] - mesh.vertices[mesh.indices[i]][0],
                mesh.vertices[mesh.indices[i + 1]][1] - mesh.vertices[mesh.indices[i]][1],
                mesh.vertices[mesh.indices[i + 1]][2] - mesh.vertices[mesh.indices[i]][2],
            };
            const float AC[3] = {
                mesh.vertices[mesh.indices[i + 2]][0] - mesh.vertices[mesh.indices[i]][0],
                mesh.vertices[mesh.indices[i + 2]][1] - mesh.vertices[mesh.indices[i]][1],
                mesh.vertices[mesh.indices[i + 2]][2] - mesh.vertices[mesh.indices[i]][2],
            };
            const float tAB[2] = {
                mesh.texCoords[mesh.indices[i + 1]][0] - mesh.texCoords[mesh.indices[i]][0],
                mesh.texCoords[mesh.indices[i + 1]][1] - mesh.texCoords[mesh.indices[i]][1],
            };
            const float tAC[2] = {
                mesh.texCoords[mesh.indices[i + 2]][0] - mesh.texCoords[mesh.indices[i]][0],
                mesh.texCoords[mesh.indices[i + 2]][1] - mesh.texCoords[mesh.indices[i]][1],
            };
            const float det = 1.0f / (tAB[0] * tAC[1] - tAB[1] * tAC[0]);
            const float T[3] = {
                det * (AB[0] * tAC[1] - AC[0] * tAB[1]),
                det * (AB[1] * tAC[1] - AC[1] * tAB[1]),
                det * (AB[2] * tAC[1] - AC[2] * tAB[1]),
            };
            mesh.tangents[mesh.indices[i    ]][0] += T[0]; mesh.tangents[mesh.indices[i    ]][1] += T[1]; mesh.tangents[mesh.indices[i    ]][2] += T[2];
            mesh.tangents[mesh.indices[i + 1]][0] += T[0]; mesh.tangents[mesh.indices[i + 1]][1] += T[1]; mesh.tangents[mesh.indices[i + 1]][2] += T[2];
            mesh.tangents[mesh.indices[i + 2]][0] += T[0]; mesh.tangents[mesh.indices[i + 2]][1] += T[1]; mesh.tangents[mesh.indices[i + 2]][2] += T[2];
        }

        for (uint i = 0; i < mesh.nbElems; i++) {
            const float d = inversesqrt(
                mesh.tangents[i][0] * mesh.tangents[i][0] +
                mesh.tangents[i][1] * mesh.tangents[i][1] +
                mesh.tangents[i][2] * mesh.tangents[i][2]);
            mesh.tangents[i][0] *= d;
            mesh.tangents[i][1] *= d;
            mesh.tangents[i][2] *= d;
        }
    }

    return mesh;
}

void deleteWavefrontMesh(WavefrontMesh *mesh) {
    if (!mesh) return;
    if (mesh->tangents ) free(mesh->tangents );
    if (mesh->normals  ) free(mesh->normals  );
    if (mesh->texCoords) free(mesh->texCoords);
    free(mesh->vertices);
    free(mesh->indices );
    memset(mesh, 0, sizeof(WavefrontMesh));
}

void deleteGPUMeshData(GPUMeshData *gpuData) {
    if (gpuData->materials) {
        for (uint i = 0; i < gpuData->nbMaterials; i++) {
            deleteTexture(&gpuData->materials[i].bump);
            deleteTexture(&gpuData->materials[i].diffuse);
        }
        free(gpuData->materials);
    }
    if (gpuData->meshes) free(gpuData->meshes);
    memset(gpuData, 0, sizeof(GPUMeshData));
}

Mesh createGPUMesh(const WavefrontMesh mesh) {
    Mesh gpuMesh = createMesh(PrimitiveType_Triangles);
    setMeshIndices(gpuMesh, mesh.indices, mesh.nbIndices);
    addMeshAttrib(gpuMesh, RGB32_FLOAT, false, mesh.vertices, mesh.nbElems);
    if (mesh.hasNormals  ) addMeshAttrib(gpuMesh, RGB32_FLOAT, false, mesh.normals  , mesh.nbElems);
    if (mesh.hasTexCoords) addMeshAttrib(gpuMesh, RG32_FLOAT , false, mesh.texCoords, mesh.nbElems);
    if (mesh.hasTangents ) addMeshAttrib(gpuMesh, RGB32_FLOAT, false, mesh.tangents , mesh.nbElems);
    return gpuMesh;
}

Mesh loadWavefront(const char *path, const char *fileName, const WavefrontFlag flags, GPUMeshData *gpuData) {
    SCOPED(WavefrontMesh) cpuMesh = loadWavefrontMesh(path, fileName, flags, gpuData);
    Mesh gpuMesh = createGPUMesh(cpuMesh);
    return gpuMesh;
}
