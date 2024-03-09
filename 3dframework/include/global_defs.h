#ifndef HEADER_9344ED236567DA57
#define HEADER_9344ED236567DA57

#pragma once

#include <stdbool.h>
typedef unsigned long long ulong;
typedef unsigned int       uint;
typedef unsigned short     ushort;
typedef unsigned char      uchar;

#define M_PI            3.1415927f
#define M_PI_D          3.1415926535897932384626433832795
#define M_GOLDEN_RATIO  1.618034f

#define MIN(a, b) ({__typeof__(a) _a = a, _b = b; (_a) < (_b) ? (_a) : (_b);})
#define MAX(a, b) ({__typeof__(a) _a = a, _b = b; (_a) > (_b) ? (_a) : (_b);})
#define CLAMP(x, min, max) MIN(max, MAX(min, x))
#define MOD(val, mod) (((val) + (mod)) % (mod))
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))
#ifdef NDEBUG
#define VERIFY(expr) (void)(expr)
#else
#define VERIFY(expr) assert(expr)
#endif

#define CAST(type, var) *(type*)&var
#define UNUSED(param) (void)param
#define CONCAT(name1, name2) name1##name2
#define _STRFY(x) #x
#define STRFY(x) _STRFY(x)

#define SWAP(a, b) { \
    __typeof__(a) c = a; \
    a = b; \
    b = c; \
}

#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define FALLTHROUGH __attribute__((fallthrough))
#define SCOPED(Type) __attribute__((__cleanup__(delete##Type))) Type
#define DECL_OPAQUE_TYPE(Type) typedef struct {void *impl;} Type;

#define MIPMAPS_FLAG    (1ull       << 47)
#define MSAA_FLAG(x)    ((ulong)(x) << 48)
#define MSAA_SAMPLES(x) (1ull << ((x >> 48) & 3))
#define MSAA_MASK       MSAA_FLAG(3ull)
#define GCN_FLAG        (1ull       << 50)
#define RDNA_FLAG       (1ull       << 51)
#define NV_MAXWELL_FLAG (1ull       << 52)
#define NV_PASCAL_FLAG  (1ull       << 53)
#define NV_TURING_FLAG  (1ull       << 54)
#define RAYTRACING_FLAG (1ull       << 55)
#define HDR10_FLAG      (1ull       << 56)
#define HDR16_FLAG      (1ull       << 57)
#define VSYNC_FLAG      (1ull       << 58)
#define STENCIL_FLAG    (1ull       << 59)
#define SNORM_FLAG      (1ull       << 60)
#define SRGB_FLAG       (1ull       << 61)
#define DEPTH_FLAG      (1ull       << 62)
#define DEPTH16_FLAG    (1ull       << 63)
#define DEPTH_MASK      (DEPTH_FLAG | DEPTH16_FLAG)
#endif // header guard

