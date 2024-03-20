#include <camera.h>
#include <context.h>
#include <input.h>
#include <SDL2/SDL.h>

static Camera current = {
    .yaw = M_PI,
    .speed = 0.005f, .inertia = 0.005f,
    .near = 1e-1f, .far = 1e3f, .fovy = 60.0f,
    .update = flyCamera
};

Camera* getCamera() {return &current;}
void setCamera(const Camera *cam) {current = *cam;}

void flyCamera(Camera *cam, const uint dt) {
    cam->isMoving = false;
    Float4 newPos = {0.0f};

    if (isInputGrabbed() && getMouseState()->hasMoved) {
        cam->isMoving = true;
        cam->yaw   -= getMouseState()->motion[0] * 0.002f;
        cam->pitch -= getMouseState()->motion[1] * 0.002f;
        if (cam->pitch >  1.57f) cam->pitch =  1.57f;
        if (cam->pitch < -1.57f) cam->pitch = -1.57f;
    }

    if (isInputGrabbed()) {
        if (KEY_PRESSED(W)) {
            newPos[0] += cosf(cam->yaw) * cosf(cam->pitch);
            newPos[1] += sinf(cam->yaw) * cosf(cam->pitch);
            newPos[2] += sinf(cam->pitch);
        }

        if (KEY_PRESSED(A)) {
            newPos[0] -= sinf(cam->yaw);
            newPos[1] += cosf(cam->yaw);
        }

        if (KEY_PRESSED(S)) {
            newPos[0] -= cosf(cam->yaw) * cosf(cam->pitch);
            newPos[1] -= sinf(cam->yaw) * cosf(cam->pitch);
            newPos[2] -= sinf(cam->pitch);
        }

        if (KEY_PRESSED(D)) {
            newPos[0] += sinf(cam->yaw);
            newPos[1] -= cosf(cam->yaw);
        }

        if (KEY_PRESSED(LSHIFT)) {
            newPos[2] += 1.0f;
        }

        if (KEY_PRESSED(LCTRL)) {
            newPos[2] -= 1.0f;
        }
    }

    if (sqlength4(newPos) > 1.0f)
        newPos = normalize4(newPos);

    cam->inertiaPos += float1(MIN(cam->inertia * dt, 1.0f)) * (newPos - cam->inertiaPos);
    cam->position += cam->speed * dt * cam->inertiaPos;
    cam->lookAt = cam->position + float4(cosf(cam->yaw) * cosf(cam->pitch), sinf(cam->yaw) * cosf(cam->pitch), sinf(cam->pitch));

    float inertia = sqlength4(cam->inertiaPos);
    if (inertia < 1e-5f)
        cam->inertiaPos = float1(0.0f);
    if (inertia > 0.0f)
        cam->isMoving = true;

    resetMatrix(&cam->projection);
    perspective(&cam->projection, cam->fovy, (float)getWidth() / getHeight(), cam->near, cam->far);
    resetMatrix(&cam->view);
    lookAt(&cam->view, cam->position, cam->lookAt, Z_AXIS);
}
