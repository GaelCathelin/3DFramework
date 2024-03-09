#include <engine.h>

int main(int, char**) {
    SCOPED(Application) app = initApplication("Hello Triangle", 1024, 768, SRGB_FLAG | VSYNC_FLAG);
    SCOPED(Shader) shader = loadShader(SHADER_DIR "shader");

    void myDraw() {
        useFramebuffer(getSwapchainFramebuffer());
        useShader(shader);
        drawSubMesh(NullMesh, 0, 3);
    }

    bool myEvents() {return KEY_PRESSED(ESCAPE);}
    launchApplication(myDraw, myEvents, NULL);
    return 0;
}
