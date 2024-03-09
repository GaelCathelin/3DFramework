#pragma once

#include <matrix.h>

typedef struct Transform Transform;
struct Transform {
    Mat4 mat;
    Mat4 matinv;
    Transform *next;
};

void pushMatrix(Transform *m);
void popMatrix(Transform *m);
void resetMatrix(Transform *m);

void setIdentity(Transform *m);
void orthographic(Transform *m, const float l, const float r, const float b, const float t, const float n, const float f);
void perspective(Transform *m, const float fovy, const float ratio, const float n, const float f);
void perspectiveRTE(Transform *m, const float fovy, const float ratio, const float n, const float f);
void perspective2(Transform *m, const float l, const float r, const float b, const float t, const float n, const float f);
void translate(Transform *m, const Float4 t);
void rotate(Transform *m, const float angle, const Float4 axis);
void rotate2(Transform *m, const float sinAngle, const float cosAngle, const Float4 axis);
void scale(Transform *m, const Float4 s);
void lookAt(Transform *m, const Float4 eye, const Float4 center, const Float4 up);
void multMatrix(Transform *m, Mat4 mat);
