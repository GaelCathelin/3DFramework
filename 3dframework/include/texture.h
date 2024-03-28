#pragma once

#include <format.h>
#include <vector.h>

#define NullTexture (Texture){0}

DECL_OPAQUE_TYPE(Texture);

typedef enum : uchar {
    TextureType_Unknown,
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    TextureCube,
    TextureCubeArray,
    Texture2DMS,
    Texture2DMSArray,
    Texture3D
} TextureType;

typedef enum : uchar {
    WrapMode_Clamp,
    WrapMode_Wrap,
    WrapMode_Border,
    WrapMode_Mirror,
    WrapMode_MirrorOnce,
    WrapMode_ClampToEdge = WrapMode_Clamp,
    WrapMode_Repeat = WrapMode_Wrap,
    WrapMode_ClampToBorder = WrapMode_Border,
    WrapMode_MirroredRepeat = WrapMode_Mirror,
    WrapMode_MirrorClampToEdge = WrapMode_MirrorOnce
} WrapMode;

typedef enum : uchar {
    FilterFunc_Standard,
    FilterFunc_Comparison,
    FilterFunc_Minimum,
    FilterFunc_Maximum
} FilterFunc;

typedef struct {
    float borderColor[4];
    float anisotropy;
    float mipBias;
    bool minFilter, magFilter, mipFilter;
    WrapMode wrapU, wrapV, wrapW;
    FilterFunc filterFunc;
    CompareFunc compareFunc;
    float minLod, maxLod;
} SamplerState;

#ifdef __cplusplus
extern "C" {
#endif

Texture createTexture1D(const uint size, const Format format, const ulong flags) WARN_UNUSED_RESULT;
Texture createTexture2D(const uint width, const uint height, const Format format, const ulong flags) WARN_UNUSED_RESULT;
Texture createTexture3D(const uint width, const uint height, const uint depth, const Format format, const ulong flags) WARN_UNUSED_RESULT;
Texture createTexture(const TextureType type, const uint width, const uint height, const uint depth, const uint layers, const Format format, const ulong flags) WARN_UNUSED_RESULT;

Texture loadTexture(const char *filename, const ulong flags) WARN_UNUSED_RESULT;
Texture loadTexturePFM(const char *filename, const ulong flags) WARN_UNUSED_RESULT;

void deleteTexture(Texture *tex);

void setTextureData(Texture tex, const void *data);
void setTextureLayerData(Texture tex, const uint layer, const void *data);
void clearTexture(Texture tex, Float4 color);
void clearTextureLayers(Texture tex, Float4 color, const uint first, const uint count);
void copyTexture(Texture dest, Texture src);
void updateMipmaps(Texture tex);
void* mapTexture(Texture tex);
void unmapTexture(Texture tex);

SamplerState* getSampler(Texture tex);
bool isDepthFormat(const Format format);

typedef struct {
    uint w, h;
    Float4 *data;
} HDRSurface;

HDRSurface loadSurfacePFM(const char *name) WARN_UNUSED_RESULT;
void deleteHDRSurface(HDRSurface *surface);

#ifdef __cplusplus
}
#endif
