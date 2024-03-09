#pragma once

#include <acceleration_structure.h>
#include <buffer.h>
#include <matrix.h>
#include <texture.h>

#define setUniform1(x, ...) _Generic(x, \
    short : setUniform1s, \
    ushort: setUniform1s, \
    bool  : setUniform1i, \
    int   : setUniform1i, \
    uint  : setUniform1i, \
    long  : setUniform1l, \
    ulong : setUniform1l, \
    float : setUniform1f, \
    double: setUniform1d)(x, __VA_ARGS__)

#define setUniform2(x, ...) _Generic(x, \
    short : setUniform2s, \
    ushort: setUniform2s, \
    bool  : setUniform2i, \
    int   : setUniform2i, \
    uint  : setUniform2i, \
    long  : setUniform2l, \
    ulong : setUniform2l, \
    float : setUniform2f, \
    double: setUniform2d)(x, __VA_ARGS__)

#define setUniform3(x, ...) _Generic(x, \
    short  : setUniform3s, \
    ushort : setUniform3s, \
    bool   : setUniform3i, \
    int    : setUniform3i, \
    uint   : setUniform3i, \
    long   : setUniform3l, \
    ulong  : setUniform3l, \
    float  : setUniform3f, \
    double : setUniform3d)(x, __VA_ARGS__)

#define setUniform4(x, ...) _Generic(x, \
    short  : setUniform4s, \
    ushort : setUniform4s, \
    bool   : setUniform4i, \
    int    : setUniform4i, \
    uint   : setUniform4i, \
    long   : setUniform4l, \
    ulong  : setUniform4l, \
    float  : setUniform4f, \
    double : setUniform4d)(x, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

DECL_OPAQUE_TYPE(Shader);

Shader createGraphicsShader(
    const void *vertBin, const size_t vertSize,
    const void *tescBin, const size_t tescSize,
    const void *teseBin, const size_t teseSize,
    const void *geomBin, const size_t geomSize,
    const void *fragBin, const size_t fragSize);
Shader createComputeShader(const void *binary, const size_t size) WARN_UNUSED_RESULT;
Shader loadShader(const char *shaderName) WARN_UNUSED_RESULT;
void deleteShader(Shader *shader);

void useShader(Shader shader);

void setUniform1f(const float x, const char *name, ...);
void setUniform2f(const float x, const float y, const char *name, ...);
void setUniform3f(const float x, const float y, const float z, const char *name, ...);
void setUniform4f(const float x, const float y, const float z, const float w, const char *name, ...);

void setUniform1d(const double x, const char *name, ...);
void setUniform2d(const double x, const double y, const char *name, ...);
void setUniform3d(const double x, const double y, const double z, const char *name, ...);
void setUniform4d(const double x, const double y, const double z, const double w, const char *name, ...);

void setUniform1s(const short x, const char *name, ...);
void setUniform2s(const short x, const short y, const char *name, ...);
void setUniform3s(const short x, const short y, const short z, const char *name, ...);
void setUniform4s(const short x, const short y, const short z, const short w, const char *name, ...);

void setUniform1i(const int x, const char *name, ...);
void setUniform2i(const int x, const int y, const char *name, ...);
void setUniform3i(const int x, const int y, const int z, const char *name, ...);
void setUniform4i(const int x, const int y, const int z, const int w, const char *name, ...);

void setUniform1l(const long x, const char *name, ...);
void setUniform2l(const long x, const long y, const char *name, ...);
void setUniform3l(const long x, const long y, const long z, const char *name, ...);
void setUniform4l(const long x, const long y, const long z, const long w, const char *name, ...);

void setUniform1F(const Float4 v, const char *name, ...);
void setUniform2F(const Float4 v, const char *name, ...);
void setUniform3F(const Float4 v, const char *name, ...);
void setUniform4F(const Float4 v, const char *name, ...);

void setUniform1I(const Int4 v, const char *name, ...);
void setUniform2I(const Int4 v, const char *name, ...);
void setUniform3I(const Int4 v, const char *name, ...);
void setUniform4I(const Int4 v, const char *name, ...);

void setUniformMat3(const Mat4 m, const char *name, ...);
void setUniformMat4(const Mat4 m, const char *name, ...);

void setUniformTextureView(Texture texture, const uint mipmap, const uint nbMipmaps, const uint layer, const uint nbLayers, const char *name, ...);
#define setUniformTextureMip(texture, mipmap, ...) setUniformTextureView(texture, mipmap, 1, 0, -1, __VA_ARGS__)
#define setUniformTexture(texture, ...) setUniformTextureView(texture, 0, -1, 0, -1, __VA_ARGS__)

void setUniformBuffer(Buffer buffer, const char *name, ...);

void setUniformAccelerationStructure(AccelerationStructure as);

void setTessellationControlPoints(const uint nbPoints);

void setGroupSize1D(const uint nbElemsX);
void setGroupSize2D(const uint nbElemsX, const uint nbElemsY);
void setGroupSize3D(const uint nbElemsX, const uint nbElemsY, const uint nbElemsZ);

void dispatch1D(const uint nbElemsX);
void dispatch2D(const uint nbElemsX, const uint nbElemsY);
void dispatch3D(const uint nbElemsX, const uint nbElemsY, const uint nbElemsZ);

void drawMesh(Mesh mesh);
void drawMeshInstanced(Mesh mesh, const uint nbInstances);
void drawMeshInstancedBaseInstance(Mesh mesh, const uint nbInstances, const uint baseInstance);
void drawMeshIndirect(Mesh mesh, Buffer indirect);
void drawSubMesh(Mesh mesh, const uint first, const uint count);
void drawSubMeshInstanced(Mesh mesh, const uint first, const uint count, const uint nbInstances);
void drawSubMeshOffsetInstanced(Mesh mesh, const uint first, const uint count, const uint offset, const uint nbInstances, const uint baseInstance);

#ifdef __cplusplus
}
#endif
