#include <engine.h>
#include "private_log.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string.h>
//#include <SDL2/SDL_syswm.h>
//#include <windows.h>
//#include "sdlwindow.h"

static UserEventsFunc userEvents = NULL;
static UserDrawFunc userDrawFrame = NULL;
static UserDrawGuiFunc userDrawGui = NULL;
static char appName[64], customTitle[128] = "";
static uint t0, t, animt = 0, nbFrames = 0;
static float dt = 0.0f, fps = 1000.0f;
static bool paused = false, screenshotRequested = false;
static char requestedScreenshotName[256];

static Texture imguiFont;
static Shader imguiShader;
static Mesh imguiMesh;
static SDL_Cursor *mouseCursors[ImGuiMouseCursor_COUNT];

static void initImgui() {
//    SDL_SysWMinfo wmInfo;
//    SDL_VERSION(&wmInfo.version);
//    SDL_GetWindowWMInfo(getSDLWindow(), &wmInfo);

    igCreateContext(NULL);
    igStyleColorsDark(NULL);

    ImGuiStyle *style = igGetStyle();
    style->FrameRounding = 5.0f;
    style->GrabRounding = 4.0f;
    style->WindowBorderSize = 0.0f;
    style->WindowMenuButtonPosition = ImGuiDir_Left;
    style->WindowRounding = 0.0f;
    style->WindowTitleAlign.x = 0.5f;

    ImGuiIO *io = igGetIO();
    io->BackendFlags |= 0*ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_RendererHasVtxOffset;
//    io->ImeWindowHandle = wmInfo.info.win.window;

    mouseCursors[ImGuiMouseCursor_Arrow     ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW   );
    mouseCursors[ImGuiMouseCursor_TextInput ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM   );
    mouseCursors[ImGuiMouseCursor_ResizeAll ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL );
    mouseCursors[ImGuiMouseCursor_ResizeNS  ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS  );
    mouseCursors[ImGuiMouseCursor_ResizeEW  ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE  );
    mouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    mouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    mouseCursors[ImGuiMouseCursor_Hand      ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND    );

    io->KeyMap[ImGuiKey_Tab        ] = SDL_SCANCODE_TAB;
    io->KeyMap[ImGuiKey_LeftArrow  ] = SDL_SCANCODE_LEFT;
    io->KeyMap[ImGuiKey_RightArrow ] = SDL_SCANCODE_RIGHT;
    io->KeyMap[ImGuiKey_UpArrow    ] = SDL_SCANCODE_UP;
    io->KeyMap[ImGuiKey_DownArrow  ] = SDL_SCANCODE_DOWN;
    io->KeyMap[ImGuiKey_PageUp     ] = SDL_SCANCODE_PAGEUP;
    io->KeyMap[ImGuiKey_PageDown   ] = SDL_SCANCODE_PAGEDOWN;
    io->KeyMap[ImGuiKey_Home       ] = SDL_SCANCODE_HOME;
    io->KeyMap[ImGuiKey_End        ] = SDL_SCANCODE_END;
    io->KeyMap[ImGuiKey_Insert     ] = SDL_SCANCODE_INSERT;
    io->KeyMap[ImGuiKey_Delete     ] = SDL_SCANCODE_DELETE;
    io->KeyMap[ImGuiKey_Backspace  ] = SDL_SCANCODE_BACKSPACE;
    io->KeyMap[ImGuiKey_Space      ] = SDL_SCANCODE_SPACE;
    io->KeyMap[ImGuiKey_Enter      ] = SDL_SCANCODE_RETURN;
    io->KeyMap[ImGuiKey_Escape     ] = SDL_SCANCODE_ESCAPE;
    io->KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
    io->KeyMap[ImGuiKey_A          ] = SDL_SCANCODE_A;
    io->KeyMap[ImGuiKey_C          ] = SDL_SCANCODE_C;
    io->KeyMap[ImGuiKey_V          ] = SDL_SCANCODE_V;
    io->KeyMap[ImGuiKey_X          ] = SDL_SCANCODE_X;
    io->KeyMap[ImGuiKey_Y          ] = SDL_SCANCODE_Y;
    io->KeyMap[ImGuiKey_Z          ] = SDL_SCANCODE_Z;

    {
        uchar *fontData;
        int fontWidth, fontHeight;
        ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &fontData, &fontWidth, &fontHeight, &(int){0});
        imguiFont = createTexture2D(fontWidth, fontHeight, RGBA8_UNORM, 0);
        setTextureData(imguiFont, fontData);
        getSampler(imguiFont)->minFilter = getSampler(imguiFont)->magFilter = false;
    }

    imguiMesh = createMesh(PrimitiveType_Triangles);
    setMeshIndices16(imguiMesh, NULL, 1 << 16);
    addMeshAttrib(imguiMesh, RG32_FLOAT , false, NULL, 1 << 16);
    addMeshAttrib(imguiMesh, RG32_FLOAT , false, NULL, 1 << 16);
    addMeshAttrib(imguiMesh, RGBA8_UNORM, false, NULL, 1 << 16);

    static const uint vertSrc[] = {
        #include "imgui.vert.spv"
    }, fragSrc[] = {
        #include "imgui.frag.spv"
    };
    imguiShader = createGraphicsShader(vertSrc, sizeof(vertSrc), NULL, 0, NULL, 0, NULL, 0, fragSrc, sizeof(fragSrc));
}

static void closeImgui() {
    deleteShader(&imguiShader);
    deleteMesh(&imguiMesh);
    deleteTexture(&imguiFont);
    for (int i = 0; i < ImGuiMouseCursor_COUNT; i++)
        SDL_FreeCursor(mouseCursors[i]);
    igDestroyContext(igGetCurrentContext());
}

Application initApplication(const char *title, const int width, const int height, const ulong flags) {
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    loadConfiguration();

    ulong newFlags = flags;
    if (getItem("Fullscreen", false)) newFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (getItem("HDR10"     , false)) newFlags |= HDR10_FLAG;
    if (getItem("HDR16"     , false)) newFlags |= HDR16_FLAG;
    strncpy(appName, getItem("WindowTitle", title), 63);
    initContext(appName, getItem("WindowWidth", width), getItem("WindowHeight", height), newFlags);

    resetRenderState();
    t0 = SDL_GetTicks();
    beginFrame();
    initImgui();
    clearTexture(getSwapchainTexture(), float1(0.0f));

    return (Application){};
}

void deleteApplication(Application *app) {
    UNUSED(app);
    closeImgui();
    deleteContext();
}

static bool processEvents() {
    bool finished = pumpEvents();
    SDL_SetCursor(mouseCursors[igGetMouseCursor()]);

    if (userEvents)
        finished |= userEvents();

    if (userDrawFrame)
        getCamera()->update(getCamera(), dt);

    return finished;
}

static void drawGui() {
    igGetIO()->DisplaySize.x = getWidth();
    igGetIO()->DisplaySize.y = getHeight();
    igNewFrame();
    userDrawGui();
    igEndFrame();
    igRender();

    const ImDrawData *drawData = igGetDrawData();
    if (drawData->TotalIdxCount > 0) {
        {   // Upload UI geometry
            if (drawData->TotalIdxCount > 1 << 16 || drawData->TotalVtxCount > 1 << 16)
                return;

            int vtxOffset = 0, idxOffset = 0;
            ImDrawIdx gpuIndices[1 << 16];
            ImVec2 gpuVertices[1 << 16], gpuTexCoords[1 << 16];
            ImU32 gpuColors[1 << 16];
            for (int i = 0; i < drawData->CmdListsCount; i++) {
                const ImDrawList *cmdList = drawData->CmdLists[i];
                memcpy(&gpuIndices[idxOffset], cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

                const ImDrawVert *buffer = cmdList->VtxBuffer.Data;
                for (int j = 0; j < cmdList->VtxBuffer.Size; j++) {
                    gpuVertices [vtxOffset + j] = buffer[j].pos;
                    gpuTexCoords[vtxOffset + j] = buffer[j].uv;
                    gpuColors   [vtxOffset + j] = buffer[j].col;
                }

                idxOffset += cmdList->IdxBuffer.Size;
                vtxOffset += cmdList->VtxBuffer.Size;
            }

            setBufferData(getIndexBuffer (imguiMesh   ), gpuIndices  , idxOffset * sizeof(ImDrawIdx));
            setBufferData(getAttribBuffer(imguiMesh, 0), gpuVertices , vtxOffset * sizeof(ImVec2   ));
            setBufferData(getAttribBuffer(imguiMesh, 1), gpuTexCoords, vtxOffset * sizeof(ImVec2   ));
            setBufferData(getAttribBuffer(imguiMesh, 2), gpuColors   , vtxOffset * sizeof(ImU32    ));
        }
        {   // Draw UI
            resetRenderState();
            getRenderState()->blendState.targets[0].blendEnable = true;
            getRenderState()->blendState.targets[0].srcBlend = BlendFactor_One;
            getRenderState()->blendState.targets[0].srcBlendAlpha = BlendFactor_OneMinusDstAlpha;
            getRenderState()->blendState.targets[0].destBlend = BlendFactor_OneMinusSrcAlpha;
            getRenderState()->blendState.targets[0].destBlendAlpha = BlendFactor_One;

            useFramebuffer(getSwapchainFramebuffer());
            useShader(imguiShader);
            setUniformTexture(imguiFont, "Texture");
            setUniform2f(2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y, "uScale");
            setUniform2f(-2.0f * drawData->DisplayPos.x / drawData->DisplaySize.x - 1.0f, -2.0f * drawData->DisplayPos.y / drawData->DisplaySize.y - 1.0f, "uTranslate");

            int vtxOffset = 0, idxOffset = 0;
            for (int i = 0; i < drawData->CmdListsCount; i++) {
                const ImDrawList *cmdList = drawData->CmdLists[i];
                for (int j = 0; j < cmdList->CmdBuffer.Size; j++) {
                    const ImDrawCmd *drawCmd = &cmdList->CmdBuffer.Data[j];
                    getRenderState()->scissorState = (ViewportState){drawCmd->ClipRect.x, drawCmd->ClipRect.z, drawCmd->ClipRect.y, drawCmd->ClipRect.w};
                    drawSubMeshOffsetInstanced(imguiMesh, idxOffset + drawCmd->IdxOffset, drawCmd->ElemCount, vtxOffset + drawCmd->VtxOffset, 1, 0);
                }
                idxOffset += cmdList->IdxBuffer.Size;
                vtxOffset += cmdList->VtxBuffer.Size;
            }
        }
    }
}

static void refreshTitle() {
    static char title[256];
    sprintf(title, "FPS:%d - %s%s", getFPS(), appName, customTitle);
    SDL_SetWindowTitle(getWindow(), title);
}

static void updateCounters() {
    dt = SDL_GetTicks() - t - t0;
    t = SDL_GetTicks() - t0;
    fps = lerp(200.0f / (200.0f + fps), dt, fps);
    if (!paused) {
        animt += dt;
        nbFrames++;
    }

    igGetIO()->DeltaTime = 1e-3f * fps;

    static uint timer = 0;
    timer += MIN(0x40, dt);
    if (timer > 0x40) {
        timer = 0;
        refreshTitle();
    }
}

static void saveScreen(const char *filename) {
    if (swapchainFormat() != BGRA8_UNORM && swapchainFormat() != SBGRA8_UNORM) {
        logError("Can't save screen. Swapchain format unsupported.");
        return;
    }

    SCOPED(Texture) tex = createTexture2D(getWidth(), getHeight(), swapchainFormat(), 0);
    copyTexture(tex, getSwapchainTexture()); // TODO: it shouldn't work for sRGB swapchain
    void *pixels = mapTexture(tex);
    flush(); waitGPUIdle();
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels, getWidth(), getHeight(), 32, 4 * getWidth(), 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    IMG_SavePNG(surface, filename);
    SDL_FreeSurface(surface);
    unmapTexture(tex);
}

void launchApplication(UserDrawFunc drawFunc, UserEventsFunc eventsFunc, UserDrawGuiFunc drawGuiFunc) {
    static bool firstFrame = true;
    userDrawFrame = drawFunc;
    userEvents = eventsFunc;
    userDrawGui = drawGuiFunc;
    t = SDL_GetTicks() - t0;
    flushEvents();
    flushAndGarbageCollect();

    bool finished = false;
    while (!finished) {
        endFrame();
        beginFrame();

        finished = processEvents();
        updateCounters();

        setLogActive(firstFrame);
        if (userDrawFrame) {
            resetRenderState();
            userDrawFrame();
        }

        if (screenshotRequested) {
            screenshotRequested = false;
            saveScreen(requestedScreenshotName);
        }
        setLogActive(true);

        if (userDrawGui) drawGui();

        firstFrame = false;
    }

    endFrame();
    waitGPUIdle();
}

void setEventFunc  (UserEventsFunc  f) {userEvents    = f;}
void setDrawFunc   (UserDrawFunc    f) {userDrawFrame = f;}
void setDrawGuiFunc(UserDrawGuiFunc f) {userDrawGui   = f;}
void setCustomTitle(const char *title) {strncpy(customTitle, title, sizeof(customTitle) - 1);}

void pauseAnimations(const bool pause) {paused = pause;}
bool animationsPaused() {return paused;}

void takeScreenshot(const char *filename) {
    screenshotRequested = true;
    strncpy(requestedScreenshotName, filename, 255);
}

uint getAppTime()   {return t;                   }
uint getAnimTime()  {return animt;               }
uint getFrameTime() {return dt;                  }
int getNbFrames()   {return nbFrames;            }
int getFPS()        {return 1000.0f / fps + 0.5f;}
int getNbOfCPUs()   {return SDL_GetCPUCount();   }
