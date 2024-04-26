#include <engine.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

int main(int, char**) {
    SCOPED(Application) app = initApplication("Remnants - Alcatraz", 1024, 576, VSYNC_FLAG);
    SCOPED(Shader) shader = loadShader(SHADER_DIR "shader");
    getCamera()->position = float4(-23.0f, 69.0f, -6.25f);
    getCamera()->yaw = 0.0f * M_PI;
    getCamera()->speed = 5e-2f;
    getCamera()->fovy = 90.0f;

    float A = 0.4167f, B = 0.79f, C = 0.2f;

    void myDraw() {
        useShader(shader);
        setUniformTexture(getSwapchainTexture(), "Framebuffer");
        setUniformMat4(getCamera()->projection.matinv, "InverseProjectionMatrix");
        setUniformMat4(getCamera()->view.matinv, "InverseViewMatrix");
        setUniform3F(getCamera()->position, "CameraPosition");
        setUniform1f(A, "A");
        setUniform1f(B, "B");
        setUniform1f(C, "C");
        dispatch2D(getWidth(), getHeight());
    }

    bool myEvents() {
        print4(getCamera()->position); putchar('\r');
        if (!igGetIO()->WantCaptureMouse) {
            ON_CLICK_ONCE(LEFT , grabInput(true ));
            ON_CLICK_ONCE(RIGHT, grabInput(false));
        }
        return KEY_PRESSED(ESCAPE);
    }

    void myGui() {
        igBegin("Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize);

        igPushItemWidth(150.0f);
        igSliderFloat("A", &A, 0.410f, 0.490f, "%.4g", 1.0f);
        igSliderFloat("B", &B, 0.785f, 0.795f, "%.4g", 1.0f);
        igSliderFloat("C", &C, 0.195f, 0.215f, "%.4g", 1.0f);
        igPopItemWidth();

        igEnd();
    }

    launchApplication(myDraw, myEvents, myGui);
    return 0;
}
