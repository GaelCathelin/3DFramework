#include <input.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>
#include <SDL2/SDL.h>

static MouseState mouse;
static bool inputGrabbed;

bool* getKeyState() {
    return (bool*)SDL_GetKeyboardState(NULL);
}

MouseState* getMouseState() {
    return &mouse;
}

bool isInputGrabbed() {
    return inputGrabbed;
}

void flushEvents() {
    SDL_PumpEvents();
    SDL_GetRelativeMouseState(NULL, NULL);
    SDL_GetMouseState(NULL, NULL);
    memset(&mouse, 0, sizeof(mouse));
}

bool pumpEvents() {
    static SDL_Event event;
    ImGuiIO *io = igGetIO();
    bool finished = false;

    SDL_PumpEvents();
    mouse.scroll[0] = mouse.scroll[1] = 0;
    SDL_GetRelativeMouseState(&mouse.motion[0], &mouse.motion[1]);
    SDL_GetMouseState(&mouse.position[0], &mouse.position[1]);
    mouse.hasMoved = mouse.motion[0] || mouse.motion[1];

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT  ) mouse.button &= ~SDL_BUTTON(SDL_BUTTON_LEFT  );
                if (event.button.button == SDL_BUTTON_MIDDLE) mouse.button &= ~SDL_BUTTON(SDL_BUTTON_MIDDLE);
                if (event.button.button == SDL_BUTTON_RIGHT ) mouse.button &= ~SDL_BUTTON(SDL_BUTTON_RIGHT );
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT  ) mouse.button |= SDL_BUTTON(SDL_BUTTON_LEFT  );
                if (event.button.button == SDL_BUTTON_MIDDLE) mouse.button |= SDL_BUTTON(SDL_BUTTON_MIDDLE);
                if (event.button.button == SDL_BUTTON_RIGHT ) mouse.button |= SDL_BUTTON(SDL_BUTTON_RIGHT );
                break;

            case SDL_MOUSEWHEEL:
                mouse.scroll[0] += event.wheel.y;
                mouse.scroll[1] -= event.wheel.x;
                break;

            case SDL_TEXTINPUT: ImGuiIO_AddInputCharactersUTF8(io, event.text.text); break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                io->KeysDown[event.key.keysym.scancode] = event.type == SDL_KEYDOWN;
                io->KeyShift = (SDL_GetModState() & KMOD_SHIFT) != 0;
                io->KeyCtrl = (SDL_GetModState() & KMOD_CTRL) != 0;
                io->KeyAlt = (SDL_GetModState() & KMOD_ALT) != 0;
                break;

            case SDL_QUIT: finished = true; break;
        }
    }

    if (!inputGrabbed) {
        io->MousePos.x = mouse.position[0];
        io->MousePos.y = mouse.position[1];
        io->MouseDown[0] = MOUSE_CLICK(LEFT);
        io->MouseDown[1] = MOUSE_CLICK(RIGHT);
        io->MouseWheel = mouse.scroll[0];
        io->MouseWheelH = mouse.scroll[1];
    }

    return finished;
}

void grabInput(bool grab) {
    inputGrabbed = grab;
    SDL_SetRelativeMouseMode(grab);
}
