#ifndef HEADER_C7E589FFD8F6E60A
#define HEADER_C7E589FFD8F6E60A

#pragma once

#include <global_defs.h>

typedef enum : uchar {
    Format_Unknown,

    R8_UINT,      R8_SINT,      R8_UNORM,     R8_SNORM,
    RG8_UINT,     RG8_SINT,     RG8_UNORM,    RG8_SNORM,
    R16_UINT,     R16_SINT,     R16_UNORM,    R16_SNORM,    R16_FLOAT,
    BGRA4_UNORM,  B5G6R5_UNORM, B5G5R5A1_UNORM,
    RGBA8_UINT,   RGBA8_SINT,   RGBA8_UNORM,  RGBA8_SNORM,  BGRA8_UNORM,
    SRGBA8_UNORM, SBGRA8_UNORM,
    R10G10B10A2_UNORM,
    R11G11B10_FLOAT,
    RG16_UINT,    RG16_SINT,    RG16_UNORM,   RG16_SNORM,   RG16_FLOAT,
    R32_UINT,     R32_SINT,     R32_FLOAT,
    RGBA16_UINT,  RGBA16_SINT,  RGBA16_FLOAT, RGBA16_UNORM, RGBA16_SNORM,
    RG32_UINT,    RG32_SINT,    RG32_FLOAT,
    RGB32_UINT,   RGB32_SINT,   RGB32_FLOAT,
    RGBA32_UINT,  RGBA32_SINT,  RGBA32_FLOAT,

    D16,  D24S8,  X24G8_UINT,   D32,  D32S8,  X32G8_UINT,

    BC1_UNORM,    BC1_UNORM_SRGB,
    BC2_UNORM,    BC2_UNORM_SRGB,
    BC3_UNORM,    BC3_UNORM_SRGB,
    BC4_UNORM,    BC4_SNORM,
    BC5_UNORM,    BC5_SNORM,
    BC6H_UFLOAT,  BC6H_SFLOAT,
    BC7_UNORM,    BC7_UNORM_SRGB,

    Format_Count,
} Format;

typedef struct {
    const char* name;
    uint size;
} FormatInfo;

typedef enum : uchar {
    CompareFunc_Never          = 1,
    CompareFunc_Less           = 2,
    CompareFunc_Equal          = 3,
    CompareFunc_LessOrEqual    = 4,
    CompareFunc_Greater        = 5,
    CompareFunc_NotEqual       = 6,
    CompareFunc_GreaterOrEqual = 7,
    CompareFunc_Always         = 8
} CompareFunc;

#ifdef __cplusplus
extern "C" {
#endif

FormatInfo getFormatInfo(const Format format);

#ifdef __cplusplus
}
#endif
#endif // header guard 

