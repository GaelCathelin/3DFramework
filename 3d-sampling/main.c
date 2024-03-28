#include <engine.h>

int main(int, char**) {
    SCOPED(Application) app = initApplication("Sampling", 512, 512, SRGB_FLAG | VSYNC_FLAG);
    SCOPED(Shader) shader = loadShader(SHADER_DIR "shader");

    getCamera()->position = float4(-2.0f, 0.0f, 0.5f);
    getCamera()->yaw = 0.0f;
    getCamera()->pitch = -0.1f;
    getCamera()->near = 1e-2f;
    getCamera()->far = 1e2f;

    void myDraw() {
        getRenderState()->blendState.targets[0].blendEnable = true;
        setBlendFactors(0, BlendFactor_SrcAlpha, BlendFactor_Zero, BlendFactor_OneMinusSrcAlpha, BlendFactor_Zero);

		clearTexture(getSwapchainTexture(), float1(0.0f));
        useFramebuffer(getSwapchainFramebuffer());
        useShader(shader);
        setUniform1(getNbFrames(), "frame");
        drawSubMeshInstanced(NullMesh, 0, 1, 10 * getNbFrames());
    }

    bool myEvents() {
		ON_CLICK_ONCE(LEFT , grabInput(true ));
		ON_CLICK_ONCE(RIGHT, grabInput(false));
		return KEY_PRESSED(ESCAPE);
	}

    launchApplication(myDraw, myEvents, NULL);
    return 0;
}
