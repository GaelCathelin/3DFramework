#pragma once

#include <global_defs.h>

#define KEY_PRESSED(keyCode) getKeyState()[SDL_SCANCODE_##keyCode]

#define ON_KEY(keyCode, ...) \
    if (KEY_PRESSED(keyCode)) {__VA_ARGS__;}

#define ON_KEY_ONCE(keyCode, ...) \
    if (KEY_PRESSED(keyCode)) {KEY_PRESSED(keyCode) = false; __VA_ARGS__;}

#define MOUSE_CLICK(buttonCode) \
    getMouseState()->button & SDL_BUTTON(SDL_BUTTON_##buttonCode)

#define ON_CLICK(buttonCode, ...) \
    if (MOUSE_CLICK(buttonCode)) {__VA_ARGS__;}

#define ON_CLICK_ONCE(buttonCode, ...) \
    if (MOUSE_CLICK(buttonCode)) {getMouseState()->button &= ~SDL_BUTTON(SDL_BUTTON_##buttonCode); __VA_ARGS__;}

#define ON_WHEEL(...) \
    if (getMouseState()->scroll[0] || getMouseState()->scroll[1]) { \
        __VA_ARGS__; \
        getMouseState()->scroll[0] = getMouseState()->scroll[1] = 0; \
    }

typedef struct {
    bool hasMoved;
    uchar button;
    int position[2], motion[2], scroll[2];
} MouseState;

#ifdef __cplusplus
extern "C" {
#endif

void flushEvents();
bool pumpEvents();
void grabInput(bool grab);

bool* getKeyState();
MouseState* getMouseState();
bool isInputGrabbed();

#ifdef __cplusplus
}
#endif
