#pragma once

#include <global_defs.h>
#include <math.h>
#include <immintrin.h>

#define aligned_malloc(size) _mm_malloc(size, 32)
#define aligned_free(p) _mm_free(p)
#define prefetch(p) _mm_prefetch(p, _MM_HINT_T0)
#define streamstore(p, v) _mm_stream_ps(p, v)

typedef int   Int4   __attribute__((vector_size(16)));
typedef uint  UInt4  __attribute__((vector_size(16)));
typedef float Float4 __attribute__((vector_size(16)));

static const Float4 __attribute__((aligned(16)))
    X_AXIS = {1.0f, 0.0f, 0.0f, 0.0f},
    Y_AXIS = {0.0f, 1.0f, 0.0f, 0.0f},
    Z_AXIS = {0.0f, 0.0f, 1.0f, 0.0f},
    W_AXIS = {0.0f, 0.0f, 0.0f, 1.0f};

#define float4(...) ({Float4 _v = (Float4){__VA_ARGS__}; _v;})
#define int4(...) ({Int4 _v = (Int4){__VA_ARGS__}; _v;})
#define uint4(...) ({UInt4 _v = (UInt4){__VA_ARGS__}; _v;})
#define float1(x) ({float _x = x; float4(_x, _x, _x, _x);})
#define int1(x) ({int _x = x; int4(_x, _x, _x, _x);})
#define uint1(x) ({uint _x = x; uint4(_x, _x, _x, _x);})

#define V_(v_, ...) ({Float4 v = v_; __VA_ARGS__;})
#define VV_(a_, b_, ...) ({Float4 a = a_, b = b_; __VA_ARGS__;})

#if defined(__SSE__)
    #define abs4(v_) V_(v_, (Float4)((Int4)v & int1(0x7FFFFFFF)))
    #define min4(a_, b_) VV_(a_, b_, _mm_min_ps(a, b))
    #define max4(a_, b_) VV_(a_, b_, _mm_max_ps(a, b))
    #define equal4(a_, b_) (_mm_movemask_ps(a_ == b_) == 15)
    #define equal3(a_, b_) ((_mm_movemask_ps(a_ == b_) & 7) == 7)
    #define equal2(a_, b_) ((_mm_movemask_ps(a_ == b_) & 3) == 3)
    #define normalize4(v_) V_(v_, v / _mm_sqrt_ps(dot4(v, v)))
    #define normalize3(v_) V_(v_, v / _mm_sqrt_ps(dot3(v, v)))
    #define normalize2(v_) V_(v_, v / _mm_sqrt_ps(dot2(v, v)))
    #define fastnormalize4(v_) V_(v_, v * _mm_rsqrt_ps(dot4(v, v)))
    #define fastnormalize3(v_) V_(v_, v * _mm_rsqrt_ps(dot3(v, v)))
    #define fastnormalize2(v_) V_(v_, v * _mm_rsqrt_ps(dot2(v, v)))
    #define length4(v_) V_(v_, _mm_sqrt_ss(dot4(v, v))[0])
    #define length3(v_) V_(v_, _mm_sqrt_ss(dot3(v, v))[0])
    #define length2(v_) V_(v_, _mm_sqrt_ss(dot2(v, v))[0])
    #define sqlength4(v_) V_(v_, dot4(v, v)[0])
    #define sqlength3(v_) V_(v_, dot3(v, v)[0])
    #define sqlength2(v_) V_(v_, dot2(v, v)[0])
    #define distance4(a_, b_) VV_(a_, b_, length4(a - b))
    #define distance3(a_, b_) VV_(a_, b_, length3(a - b))
    #define distance2(a_, b_) VV_(a_, b_, length2(a - b))
    #define sqdistance4(a_, b_) VV_(a_, b_, sqlength4(a - b))
    #define sqdistance3(a_, b_) VV_(a_, b_, sqlength3(a - b))
    #define sqdistance2(a_, b_) VV_(a_, b_, sqlength2(a - b))
    #define cross3(a_, b_) VV_(a_, b_, \
        _mm_shuffle_ps(a, a, 0xC9) * \
        _mm_shuffle_ps(b, b, 0xD2) - \
        _mm_shuffle_ps(a, a, 0xD2) * \
        _mm_shuffle_ps(b, b, 0xC9))
    #define project4(a_, b_) VV_(a_, b_, dot4(a, b) / dot4(b, b) * b)
    #define project3(a_, b_) VV_(a_, b_, dot3(a, b) / dot3(b, b) * b)
    #define project2(a_, b_) VV_(a_, b_, dot2(a, b) / dot2(b, b) * b)
    #define projectn4(a_, b_) VV_(a_, b_, dot4(a, b) * b)
    #define projectn3(a_, b_) VV_(a_, b_, dot3(a, b) * b)
    #define projectn2(a_, b_) VV_(a_, b_, dot2(a, b) * b)
    #define print4(v_) V_(v_, printf("%g %g %g %g\n", (double)v[0], (double)v[1], (double)v[2], (double)v[3]))
#endif

#if defined(__SSE2__)
    #define mod4(a_, b_) VV_(a_, b_, a - b * _mm_cvtepi32_ps(_mm_cvttps_epi32(a / b)))
#elif defined (__SSE__)
    #define mod4(a_, b_) VV_(a_, b_, \
        Int4 t = int4(a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3]); \
        a - b * float4(t[0], t[1], t[2], t[3]))
#endif

#if defined(__SSE4_1__)
    #define dot4(a_, b_) VV_(a_, b_, _mm_dp_ps(a, b, 0xFF))
    #define dot3(a_, b_) VV_(a_, b_, _mm_dp_ps(a, b, 0x7F))
    #define dot2(a_, b_) VV_(a_, b_, _mm_dp_ps(a, b, 0x3F))
#elif defined(__SSE3__)
    #define dot4(a_, b_) VV_(a_, b_, \
        Float4 v = a * b; \
        v = _mm_hadd_ps(v, v); \
        _mm_hadd_ps(v, v))
    #define dot3(a_, b_) VV_(a_, b_, \
        Float4 v = _mm_and_ps(a * b, (Float4)int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)); \
        v = _mm_hadd_ps(v, v); \
        _mm_hadd_ps(v, v))
    #define dot2(a_, b_) VV_(a_, b_, \
        Float4 v = _mm_and_ps(a * b, (Float4)int4(0xFFFFFFFF, 0xFFFFFFFF)); \
        v = _mm_hadd_ps(v, v); \
        _mm_hadd_ps(v, v))
#elif defined(__SSE__)
    #define dot4(a_, b_) VV_(a_, b_, \
        Float4 v = a * b; \
        v += _mm_shuffle_ps(v, v, 0x4E); \
        v + _mm_shuffle_ps(v, v, 0x71))
    #define dot3(a_, b_) VV_(a_, b_, \
        Float4 v = _mm_and_ps(a * b, (Float4)int4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)); \
        v += _mm_shuffle_ps(v, v, 0x4E); \
        v + _mm_shuffle_ps(v, v, 0x71))
    #define dot2(a_, b_) VV_(a_, b_, \
        Float4 v = _mm_and_ps(a * b, (Float4)int4(0xFFFFFFFF, 0xFFFFFFFF)); \
        v += _mm_shuffle_ps(v, v, 0x4E); \
        v + _mm_shuffle_ps(v, v, 0x71))
#endif

#define lerp(t, a, b) ((a) * (1.0f - (t)) + (b) * (t))

static inline float fract(const float x) {
    return x - floorf(x);
}

static inline float smoothstep(const float a, const float b, const float x) {
    float t = CLAMP((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
/*
vec3 hsv_to_rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );
    return c.z * mix( vec3(1.0), rgb, c.y);
}
*/
static inline Float4 hsv2rgb(Float4 hsv) {
    char h = hsv[0] * 6.0f;
    float f = hsv[0] * 6.0f - h;
    float l = hsv[2] * (1.0f - hsv[1]);
    float m = hsv[2] * (1.0f - f * hsv[1]);
    float n = hsv[2] * (1.0f - (1.0f - f) * hsv[1]);

    switch(h) {
        case 0:  return float4(hsv[2], n, l);
        case 1:  return float4(m, hsv[2], l);
        case 2:  return float4(l, hsv[2], n);
        case 3:  return float4(l, m, hsv[2]);
        case 4:  return float4(n, l, hsv[2]);
        default: return float4(hsv[2], l, m);
    }
}

static inline float inversesqrt(const float x) {
#ifdef __SSE__
    float f = _mm_rsqrt_ss(_mm_load_ss(&x))[0];
    f *= 1.5f - 0.5f * f * f * x;
    return f;
#else
    union {float f; uint i;} u = {x};
    u.i = 0x5F376000 - (u.i >> 1);
    u.f *= 1.5f - 0.5f * u.f * u.f * x;
    return u.f;
#endif
}

static inline void halfs2floats(const void *src, float *dst, const int nbElems) {
    const ushort *half = (ushort*)src;
    for (int i = 0; i < nbElems; i++) {
        union {uint i; float f;} u = {
            ((uint)(half[i] & 0x8000u) << 16) |
            ((uint)(half[i] & 0x7FFFu) + 0x1C000u) << 13
        };
        dst[i] = u.f;
    }
}

static inline void floats2halfs(const void *src, void *dst, const int nbFloats) {
    const __m256 *_src = (const __m256*)src;
    __m128i *_dst = (__m128i*)dst;
    for (int i = 0; i < nbFloats / 8; i++)
        _dst[i] = _mm256_cvtps_ph(_src[i], _MM_FROUND_TO_ZERO);
}

static inline float uint2float(const uint i) {
    union {uint i; float f;} u = {0x3F800000u | (i >> 9u)};
    return u.f - 1.0f;
}

static inline int float2int(const float f) {
#ifdef __SSE__
    return _mm_cvttss_si32(_mm_load_ss(&f));
#else
    return f;
#endif
}

static inline int signf(const float x) {
#ifdef __SSE__
    int s = _mm_movemask_ps(_mm_load_ss(&x));
    return 1 - s - s;
#else
    union {float f; uint i;} u = {x};
    u.i >>= 31;
    return 1 - u.i - u.i;
#endif
}

static inline ulong mortonEncode(uint x, uint y) {
    return _pdep_u32(x, 0x55555555u) | _pdep_u32(y, 0xAAAAAAAAu);
}

static inline UInt4 mortonDecode(ulong m) {
    return uint4((uint)_pext_u64(m, 0x5555555555555555ull), (uint)_pext_u64(m, 0xAAAAAAAAAAAAAAAAull));
}

static inline int ceilLog2(const uint i) {
    return 32 - __builtin_clz(i - 1);
}

static inline int ceilPow2(const uint i) {
    return 1 << ceilLog2(i);
}
