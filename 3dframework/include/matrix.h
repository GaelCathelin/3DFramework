#pragma once

#include <vector.h>

typedef Float4 Mat4[4];

static inline void mat4(Mat4 m, Float4 a, Float4 b, Float4 c, Float4 d) {
    m[0] = a; m[1] = b; m[2] = c; m[3] = d;
}

static inline void matt4(Mat4 m, Float4 a, Float4 b, Float4 c, Float4 d) {
#ifdef __SSE__
    _MM_TRANSPOSE4_PS(a, b, c, d);
    mat4(m, a, b, c, d);
#else
    m[0][0] = a[0]; m[0][1] = b[0]; m[0][2] = c[0]; m[0][3] = d[0];
    m[1][0] = a[1]; m[1][1] = b[1]; m[1][2] = c[1]; m[1][3] = d[1];
    m[2][0] = a[2]; m[2][1] = b[2]; m[2][2] = c[2]; m[2][3] = d[2];
    m[3][0] = a[3]; m[3][1] = b[3]; m[3][2] = c[3]; m[3][3] = d[3];
#endif
}

static inline void mat1(Mat4 m, float x) {
    mat4(m,
        float4(x   , 0.0f, 0.0f, 0.0f),
        float4(0.0f, x   , 0.0f, 0.0f),
        float4(0.0f, 0.0f, x   , 0.0f),
        float4(0.0f, 0.0f, 0.0f, x   )
    );
}

static inline void copym4(Mat4 dst, Mat4 src) {
    dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
}

#ifdef __SSE__
static inline void transposem4(Mat4 r, Mat4 m) {
    Float4 a = _mm_unpacklo_ps(m[0], m[1]);
    Float4 b = _mm_unpacklo_ps(m[2], m[3]);
    Float4 c = _mm_unpackhi_ps(m[0], m[1]);
    Float4 d = _mm_unpackhi_ps(m[2], m[3]);
    r[0] = _mm_movelh_ps(a, b);
    r[1] = _mm_movehl_ps(b, a);
    r[2] = _mm_movelh_ps(c, d);
    r[3] = _mm_movehl_ps(d, c);
}

static inline void fromQuaternionstm4(Mat4 m, const float *q) {
	m[0][0] = 1.0f - 2.0f * q[1] * q[1] - 2.0f * q[2] * q[2];
	m[0][1] = 2.0f * q[0] * q[1] - 2.0f * q[3] * q[2];
	m[0][2] = 2.0f * q[2] * q[0] + 2.0f * q[3] * q[1];
    m[0][3] = 0.0f;

	m[1][0] = 2.0f * q[0] * q[1] + 2.0f * q[3] * q[2];
	m[1][1] = 1.0f - 2.0f * q[0] * q[0] - 2.0f * q[2] * q[2];
	m[1][2] = 2.0f * q[1] * q[2] - 2.0f * q[3] * q[0];
    m[1][3] = 0.0f;

	m[2][0] = 2.0f * q[2] * q[0] - 2.0f * q[3] * q[1];
	m[2][1] = 2.0f * q[1] * q[2] + 2.0f * q[3] * q[0];
	m[2][2] = 1.0f - 2.0f * q[0] * q[0] - 2.0f * q[1] * q[1];
    m[2][3] = 0.0f;

	m[3] = W_AXIS;
}

static inline Float4 transformtm4(Mat4 m, Float4 v) {
    return
        m[0] * _mm_shuffle_ps(v, v, 0x00) +
        m[1] * _mm_shuffle_ps(v, v, 0x55) +
        m[2] * _mm_shuffle_ps(v, v, 0xAA) +
        m[3] * _mm_shuffle_ps(v, v, 0xFF);
}

// r and b must not alias
static inline void multiplym4(Mat4 r, Mat4 a, Mat4 b) {
    r[0] = transformtm4(b, a[0]);
    r[1] = transformtm4(b, a[1]);
    r[2] = transformtm4(b, a[2]);
    r[3] = transformtm4(b, a[3]);
}

// r and a must not alias
static inline void multiplytm4(Mat4 r, Mat4 a, Mat4 b) {
    r[0] = transformtm4(a, b[0]);
    r[1] = transformtm4(a, b[1]);
    r[2] = transformtm4(a, b[2]);
    r[3] = transformtm4(a, b[3]);
}

static inline void multiplystorem4(float r[16], Mat4 a, Mat4 b) {
    streamstore(r     , transformtm4(b, a[0]));
    streamstore(r +  4, transformtm4(b, a[1]));
    streamstore(r +  8, transformtm4(b, a[2]));
    streamstore(r + 12, transformtm4(b, a[3]));
}

static inline void multiplystoretm4(float r[16], Mat4 a, Mat4 b) {
    streamstore(r     , transformtm4(a, b[0]));
    streamstore(r +  4, transformtm4(a, b[1]));
    streamstore(r +  8, transformtm4(a, b[2]));
    streamstore(r + 12, transformtm4(a, b[3]));
}

static inline void inversem4(Mat4 r, Mat4 m) {
    Float4
        A = _mm_movelh_ps(m[0], m[1]),
        B = _mm_movehl_ps(m[1], m[0]),
        C = _mm_movelh_ps(m[2], m[3]),
        D = _mm_movehl_ps(m[3], m[2]),
        iA, iB, iC, iD, DC, AB,
        dA, dB, dC, dD, det, d, d1, d2, rd;

    AB  = _mm_mul_ps(_mm_shuffle_ps(A, A, 0x0F), B);
    AB -= _mm_mul_ps(_mm_shuffle_ps(A, A, 0xA5), _mm_shuffle_ps(B, B, 0x4E));
    DC  = _mm_mul_ps(_mm_shuffle_ps(D, D, 0x0F), C);
    DC -= _mm_mul_ps(_mm_shuffle_ps(D, D, 0xA5), _mm_shuffle_ps(C, C, 0x4E));

    dA = _mm_mul_ps(_mm_shuffle_ps(A, A, 0x5F), A);
    dA = _mm_sub_ss(dA, _mm_movehl_ps(dA, dA));
    dB = _mm_mul_ps(_mm_shuffle_ps(B, B, 0x5F), B);
    dB = _mm_sub_ss(dB, _mm_movehl_ps(dB, dB));
    dC = _mm_mul_ps(_mm_shuffle_ps(C, C, 0x5F), C);
    dC = _mm_sub_ss(dC, _mm_movehl_ps(dC, dC));
    dD = _mm_mul_ps(_mm_shuffle_ps(D, D, 0x5F), D);
    dD = _mm_sub_ss(dD, _mm_movehl_ps(dD, dD));

    d = _mm_mul_ps(_mm_shuffle_ps(DC, DC, 0xD8), AB);

    iD  = _mm_mul_ps(_mm_shuffle_ps(C, C, 0xA0), _mm_movelh_ps(AB, AB));
    iD += _mm_mul_ps(_mm_shuffle_ps(C, C, 0xF5), _mm_movehl_ps(AB, AB));
    iA  = _mm_mul_ps(_mm_shuffle_ps(B, B, 0xA0), _mm_movelh_ps(DC, DC));
    iA += _mm_mul_ps(_mm_shuffle_ps(B, B, 0xF5), _mm_movehl_ps(DC, DC));

    d = _mm_add_ps(d, _mm_movehl_ps(d, d));
    d = _mm_add_ss(d, _mm_shuffle_ps(d, d, 0x01));
    d1 = dA * dD;
    d2 = dB * dC;

    iD = D * _mm_shuffle_ps(dA, dA, 0x00) - iD;
    iA = A * _mm_shuffle_ps(dD, dD, 0x00) - iA;

    det = d1 + d2 - d;
    rd = 1.0f / det;
    rd = _mm_and_ps(_mm_cmpneq_ss(det, _mm_setzero_ps()), rd);

    iB  = _mm_mul_ps(D, _mm_shuffle_ps(AB, AB, 0x33));
    iB -= _mm_mul_ps(_mm_shuffle_ps(D, D, 0xB1), _mm_shuffle_ps(AB, AB, 0x66));
    iC  = _mm_mul_ps(A, _mm_shuffle_ps(DC, DC, 0x33));
    iC -= _mm_mul_ps(_mm_shuffle_ps(A, A, 0xB1), _mm_shuffle_ps(DC, DC, 0x66));

    rd = _mm_shuffle_ps(rd, rd, 0x00);
    rd = (Float4)((UInt4)rd ^ uint4(0x00000000, 0x80000000, 0x80000000, 0x00000000));

    iB = C * _mm_shuffle_ps(dB, dB, 0x00) - iB;
    iC = B * _mm_shuffle_ps(dC, dC, 0x00) - iC;

    iA *= rd;
    iB *= rd;
    iC *= rd;
    iD *= rd;

    r[0] = _mm_shuffle_ps(iA, iB, 0x77);
    r[1] = _mm_shuffle_ps(iA, iB, 0x22);
    r[2] = _mm_shuffle_ps(iC, iD, 0x77);
    r[3] = _mm_shuffle_ps(iC, iD, 0x22);
}
#endif

#if defined(__SSE4_1__)
static inline Float4 transformm4(Mat4 m, Float4 v) {
    return
        _mm_dp_ps(m[0], v, 0xF1) +
        _mm_dp_ps(m[1], v, 0xF2) +
        _mm_dp_ps(m[2], v, 0xF4) +
        _mm_dp_ps(m[3], v, 0xF8);
}
#elif defined (__SSE__)
static inline Float4 transformm4(Mat4 m, Float4 v) {
    Float4 a = dot4(m[0], v);
    Float4 b = dot4(m[1], v);
    Float4 c = dot4(m[2], v);
    Float4 d = dot4(m[3], v);
    a = _mm_unpacklo_ps(a, b);
    c = _mm_unpacklo_ps(c, d);
    return _mm_movelh_ps(a, c);
}
#endif

#define printm4(m_) ({Mat4 m; copym4(m, m_); print4(m[0]); putchar('\n'); print4(m[1]); putchar('\n'); print4(m[2]); putchar('\n'); print4(m[3]); putchar('\n');})
