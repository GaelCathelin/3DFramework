#include <engine.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

static void screenshot() {
    static char filename[64];
    sprintf(filename, "screenshot%u.png", getNbFrames());
    takeScreenshot(filename);
}

int main(int, char**) {
    SCOPED(Application) app = initApplication("Mandelbrot", 1024, 768, SRGB_FLAG | 0*VSYNC_FLAG | 1*RDNA_FLAG);
    SCOPED(Shader) shader = loadShader(SHADER_DIR "mandelbrot");

    enum : int {FP16, FP32, FP64, INT32, INT64} version = FP32;
    int maxIters = 300, aa = 2;
    double offset[2] = {-0.75, 0.0};
    double zoom = 2.5;

    void myDraw() {
        useFramebuffer(getSwapchainFramebuffer());
        useShader(shader);
        setUniform2d(offset[0], offset[1], "offset");
        setUniform1d(zoom, "zoom");
        setUniform2f(getWidth(), getHeight(), "resolution");
        setUniform1i(maxIters, "maxIters");
        setUniform1f(1.0f / aa, "subPixel");
        setUniform1i(version, "version");
        drawSubMesh(NullMesh, 0, 3);
    }

    bool myEvents() {
        if (!igGetIO()->WantCaptureMouse){
            if (MOUSE_CLICK(LEFT) && getMouseState()->hasMoved) {
                offset[0] -= getMouseState()->motion[0] * zoom / getHeight();
                offset[1] -= getMouseState()->motion[1] * zoom / getHeight();
            }
            ON_WHEEL(
                zoom *= pow(0.8, getMouseState()->scroll[0]);
                zoom /= pow(0.8, getMouseState()->scroll[1]);
            );
        }
        ON_KEY_ONCE(F12, screenshot());
        return KEY_PRESSED(ESCAPE);
    }

    void myGui() {
        igSetNextWindowPos((ImVec2){getWidth(), 0}, ImGuiCond_Always, (ImVec2){1.0f, 0.0f});
        igSetNextWindowSize((ImVec2){250.0f, getHeight()}, ImGuiCond_Always);
        igSetNextWindowCollapsed(true, ImGuiCond_Appearing);
        igBegin("Settings", NULL, ImGuiWindowFlags_NoResize);

        igPushItemWidth(150.0f);
        igInputDouble("X", &offset[0], 0.0, 0.0, "%.20g", 0);
        igInputDouble("Y", &offset[1], 0.0, 0.0, "%.20g", 0);
        igInputDouble("Zoom", &zoom, 0.0, 0.0, "%g", 0);
        igInputInt("Max Iters", &maxIters, 6, 36, 0);
        igPopItemWidth();

        igSpacing(); igSeparator(); igSpacing();

        igTextUnformatted("Precision:", NULL);
        igRadioButtonIntPtr("FP16" , &version, FP16 ); igSameLine(0.0f, 10.0f);
        igRadioButtonIntPtr("FP32" , &version, FP32 ); igSameLine(0.0f, 10.0f);
        igRadioButtonIntPtr("FP64" , &version, FP64 );
        igRadioButtonIntPtr("INT32", &version, INT32); igSameLine(0.0f, 10.0f);
        igRadioButtonIntPtr("INT64", &version, INT64);

        igSpacing(); igSeparator(); igSpacing();

        igTextUnformatted("Anti-aliasing:", NULL);
        igRadioButtonIntPtr("1x" , &aa, 1); igSameLine(0.0f, 10.0f);
        igRadioButtonIntPtr("4x" , &aa, 2); igSameLine(0.0f, 10.0f);
        igRadioButtonIntPtr("16x", &aa, 4);

        igSpacing(); igSeparator(); igSpacing();

        if (igButton("Take screenshot (F12)", (ImVec2){})) screenshot();

        igEnd();
    }

    launchApplication(myDraw, myEvents, myGui);
    return 0;
}
