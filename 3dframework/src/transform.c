#include <transform.h>
#include <math.h>
#include <string.h>

void pushMatrix(Transform *m) {
    Transform *mat = aligned_malloc(sizeof(Transform));
    *mat = *m;
    m->next = mat;
}

void popMatrix(Transform *m) {
    if (m->next) {
        Transform *mat = m->next;
        *m = *mat;
        aligned_free(mat);
    } else {
        setIdentity(m);
    }
}

void resetMatrix(Transform *m) {
    do {popMatrix(m);} while (m->next);
}

void setIdentity(Transform *m) {
    mat1(m->mat   , 1.0f);
    mat1(m->matinv, 1.0f);
}

void orthographic(Transform *m, const float l, const float r, const float b, const float t, const float n, const float f) {
    Mat4 tr = {
        {2.0f / (r - l)   , 0.0f             , 0.0f          , 0.0f},
        {0.0f             , 2.0f / (b - t)   , 0.0f          , 0.0f},
        {0.0f             , 0.0f             , 1.0f / (f - n), 0.0f},
        {(r + l) / (l - r), (t + b) / (t - b), f / (f - n)   , 1.0f},
    };

    multiplytm4(m->mat, tr, m->mat);

    Mat4 invtr;
    inversem4(invtr, tr);
    Mat4 invmat;
    memcpy(invmat, m->matinv, sizeof(Mat4));
    multiplytm4(m->matinv, invmat, invtr);
}

void perspective(Transform *m, const float fovy, const float ratio, const float n, const float f) {
    const float t = 1.0f / tanf(fovy * M_PI / 360.0f);
    Mat4 tr = {
        {t / ratio,  0.0f, 0.0f           ,  0.0f},
        {0.0f     , -t   , 0.0f           ,  0.0f},
        {0.0f     ,  0.0f, n / (f - n)    , -1.0f},
        {0.0f     ,  0.0f, f * n / (f - n),  0.0f},
    };

    multiplytm4(m->mat, tr, m->mat);

    Mat4 invtr;
    inversem4(invtr, tr);
    Mat4 invmat;
    memcpy(invmat, m->matinv, sizeof(Mat4));
    multiplytm4(m->matinv, invmat, invtr);
}

void perspectiveRTE(Transform *m, const float fovy, const float ratio, const float n, const float f) {
    const float t = 1.0f / tanf(fovy * M_PI / 360.0f);
    Mat4 tr = {
        {-t / ratio, 0.0f, 0.0f           , 0.0f},
        { 0.0f     , t   , 0.0f           , 0.0f},
        { 0.0f     , 0.0f, f / (f - n)    , 1.0f},
        { 0.0f     , 0.0f, f * n / (n - f), 0.0f},
    };

    multiplytm4(m->mat, tr, m->mat);

    Mat4 invtr;
    inversem4(invtr, tr);
    Mat4 invmat;
    memcpy(invmat, m->matinv, sizeof(Mat4));
    multiplytm4(m->matinv, invmat, invtr);
}

void perspective2(Transform *m, const float l, const float r, const float b, const float t, const float n, const float f) {
    Mat4 tr = {
        {2.0f * n / (r - l), 0.0f              , 0.0f           ,  0.0f},
        {0.0f              , 2.0f * n / (b - t), 0.0f           ,  0.0f},
        {(r + l) / (r - l) , (t + b) / (b - t) , n / (f - n)    , -1.0f},
        {0.0f              , 0.0f              , f * n / (f - n),  0.0f},
    };

    multiplytm4(m->mat, tr, m->mat);
    Mat4 invtr;
    inversem4(invtr, tr);
    multiplytm4(m->matinv, invtr, m->matinv);
}

void translate(Transform *m, const Float4 t) {
    Mat4 tr = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {t[0], t[1], t[2], 1.0f},
    };
    Mat4 invtr = {
        { 1.0f,  0.0f,  0.0f, 0.0f},
        { 0.0f,  1.0f,  0.0f, 0.0f},
        { 0.0f,  0.0f,  1.0f, 0.0f},
        {-t[0], -t[1], -t[2], 1.0f},
    };

    multiplytm4(m->mat, tr, m->mat);
    multiplytm4(m->matinv, invtr, m->matinv);
}

void rotate(Transform *m, const float angle, const Float4 axis) {
    const double theta = (double)angle * (M_PI_D / 180.0);
    rotate2(m, sin(theta), cos(theta), axis);
}

void rotate2(Transform *m, const float sinAngle, const float cosAngle, const Float4 axis) {
    Mat4 tr = {
        axis * float1(axis[0] - cosAngle * axis[0]) + float4( cosAngle          ,  axis[2] * sinAngle, -axis[1] * sinAngle),
        axis * float1(axis[1] - cosAngle * axis[1]) + float4(-axis[2] * sinAngle,  cosAngle          ,  axis[0] * sinAngle),
        axis * float1(axis[2] - cosAngle * axis[2]) + float4( axis[1] * sinAngle, -axis[0] * sinAngle,  cosAngle          ),
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
    Mat4 invtr;
    transposem4(invtr, tr);

    multiplytm4(m->mat, tr, m->mat);
    multiplytm4(m->matinv, invtr, m->matinv);
}

void scale(Transform *m, const Float4 s) {
    Mat4 tr = {
        {s[0], 0.0f, 0.0f, 0.0f},
        {0.0f, s[1], 0.0f, 0.0f},
        {0.0f, 0.0f, s[2], 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    };
    Mat4 invtr = {
        {1.0f / s[0], 0.0f       , 0.0f       , 0.0f},
        {0.0f       , 1.0f / s[1], 0.0f       , 0.0f},
        {0.0f       , 0.0f       , 1.0f / s[2], 0.0f},
        {0.0f       , 0.0f       , 0.0f       , 1.0f},
    };

    multiplytm4(m->mat, tr, m->mat);
    multiplytm4(m->matinv, invtr, m->matinv);
}

void lookAt(Transform *m, const Float4 eye, const Float4 center, const Float4 up) {
    Float4 eyeDir = normalize3(eye - center);
    Float4 normal = normalize3(cross3(up, eyeDir));
    Float4 conormal = normalize3(cross3(eyeDir, normal));

    // Rewrite this crap
    eyeDir[3] = -dot3(eyeDir, eye)[0];
    normal[3] = -dot3(normal, eye)[0];
    conormal[3] = -dot3(conormal, eye)[0];
    Mat4 trtr = {normal, conormal, eyeDir, W_AXIS};
    Mat4 tr;
    transposem4(tr, trtr);
    inversem4(trtr, tr);

    multiplytm4(m->mat, tr, m->mat);
    multiplytm4(m->matinv, trtr, m->matinv);
}

void multMatrix(Transform *m, Mat4 mat) {
    multiplytm4(m->mat, mat, m->mat);
    Mat4 inv;
    inversem4(inv, mat);
    multiplytm4(m->matinv, inv, m->matinv);
}
