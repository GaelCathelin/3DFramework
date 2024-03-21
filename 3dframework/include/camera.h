#pragma once

#include <transform.h>

typedef struct Camera Camera;
typedef void (*CameraUpdateFunc)(Camera*, const uint);
struct Camera {
    bool isMoving;
    float near, far, fovy, yaw, pitch, speed, inertia;
    Float4 position, lookAt, inertiaPos;
    CameraUpdateFunc update;
    Transform projection, view;
};

#ifdef __cplusplus
extern "C" {
#endif

Camera* getCamera();
void setCamera(const Camera *cam);

void flyCamera(Camera *cam, const uint dt);

#ifdef __cplusplus
}
#endif
