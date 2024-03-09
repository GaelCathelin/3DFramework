#pragma once

#include <camera.h>
#include <config_file.h>
#include <context.h>
#include <file.h>
#include <framebuffer.h>
#include <graphics_states.h>
#include <input.h>
#include <mesh.h>
#include <random.h>
#include <shader.h>
#include <SDL2/SDL.h>

#define SHADER_DIR  "../spv/"

typedef bool (*UserEventsFunc )();
typedef void (*UserDrawFunc   )();
typedef void (*UserDrawGuiFunc)();
typedef struct {} Application;

Application initApplication(const char *title, const int width, const int height, const ulong flags);
void launchApplication(UserDrawFunc drawFunc, UserEventsFunc eventsFunc, UserDrawGuiFunc drawGuiFunc);
void deleteApplication(Application *app);

void setEventFunc(UserEventsFunc f);
void setDrawFunc(UserDrawFunc f);
void setDrawGuiFunc(UserDrawGuiFunc f);
void setCustomTitle(const char *title);

void pauseAnimations(const bool pause);
bool animationsPaused();

void takeScreenshot(const char *filename);

uint getAppTime();
uint getAnimTime();
uint getFrameTime();
int getNbFrames();
int getFPS();
int getNbOfCPUs();
