#include <engine.h>

int main(int, char**) {
    SCOPED(Application) app = initApplication("Sampling", 320, 240, SRGB_FLAG | VSYNC_FLAG);
    SCOPED(Shader) shader = loadShader(SHADER_DIR "shader");

	getCamera()->position = float4(2.0f, 0.0f, 0.0f);

    void myDraw() {
		clearTexture(getSwapchainTexture(), float1(0.1f));
        useFramebuffer(getSwapchainFramebuffer());
        useShader(shader);
		for (int i = 0; i < 16; i++) {
			setUniform3F(float4(), "pos");
			drawSubMesh(NullMesh, 0, 1);
		}
    }

    bool myEvents() {
		ON_CLICK_ONCE(LEFT , grabInput(true ));
		ON_CLICK_ONCE(RIGHT, grabInput(false));
		return KEY_PRESSED(ESCAPE);
	}
	
    launchApplication(myDraw, myEvents, NULL);
    return 0;
}
