#pragma once

#include <framebuffer.h>

typedef enum {
    ColorSpace_Linear,
    ColorSpace_sRGB,
    ColorSpace_HDR10,
} ColorSpace;

#ifdef __cplusplus
extern "C" {
#endif

void initContext(const char *title, const uint width, const uint height, const ulong flags);
void deleteContext();

void beginFrame();
void endFrame();

void flush();
void flushAndGarbageCollect();
void waitGPUIdle();

uint getWidth();
uint getHeight();
Texture getSwapchainTexture();
Texture getSwapchainDepth();
Framebuffer getSwapchainFramebuffer();
Format swapchainFormat();
ColorSpace swapchainColorSpace();
struct SDL_Window* getWindow();
const char* getDeviceName();
bool raytracingEnabled();

void beginTimerQuery();
void endTimerQuery();
float timerQueryResult();

#ifdef __cplusplus
}
#endif
