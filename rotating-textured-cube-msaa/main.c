#include <engine.h>

#define AA_LEVEL 3 // 0 -> no AA, 1 -> AA 2x, 2 -> AA 4x, 3 -> AA 8x

static Mesh WARN_UNUSED_RESULT createTexturedUnitCubeMesh() {
    const float vertices[][3] = {
        {-1, -1, -1}, { 1, -1, -1}, { 1, -1,  1}, {-1, -1, -1}, { 1, -1,  1}, {-1, -1,  1},
        { 1, -1, -1}, { 1,  1, -1}, { 1,  1,  1}, { 1, -1, -1}, { 1,  1,  1}, { 1, -1,  1},
        { 1,  1, -1}, {-1,  1, -1}, {-1,  1,  1}, { 1,  1, -1}, {-1,  1,  1}, { 1,  1,  1},
        {-1,  1, -1}, {-1, -1, -1}, {-1, -1,  1}, {-1,  1, -1}, {-1, -1,  1}, {-1,  1,  1},
        {-1, -1, -1}, {-1,  1, -1}, { 1,  1, -1}, {-1, -1, -1}, { 1,  1, -1}, { 1, -1, -1},
        {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1, -1,  1}, { 1,  1,  1}, {-1,  1,  1},
    };
    const float texCoords[][2] = {
        {0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1},
        {0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1},
        {0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1},
        {0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1},
        {0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1},
        {0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1},
    };

    Mesh mesh = createMesh(PrimitiveType_Triangles);
    addMeshAttrib(mesh, RGB32_FLOAT, false, vertices , ARRAY_SIZE(vertices ));
    addMeshAttrib(mesh, RG32_FLOAT , false, texCoords, ARRAY_SIZE(texCoords));
    return mesh;
}

int main(int, char**) {
    SCOPED(Application) app = initApplication("Rotating Textured Cube MSAA", 1024, 768, SRGB_FLAG | VSYNC_FLAG);

    SCOPED(Texture) msaaTex = createTexture2D(getWidth(), getHeight(), swapchainFormat(), MSAA_FLAG(AA_LEVEL));
    SCOPED(Texture) msaaDepth = createTexture2D(getWidth(), getHeight(), D32, MSAA_FLAG(AA_LEVEL));
    SCOPED(Framebuffer) msaaFB = createFramebuffer(&msaaTex, 1, msaaDepth);

    SCOPED(Texture) texture = loadTexture("../textures/logo.png", SRGB_FLAG | MIPMAPS_FLAG);
    getSampler(texture)->anisotropy = 16.0f;
    getSampler(texture)->magFilter = false;
    SCOPED(Shader) shader = loadShader(SHADER_DIR "shader");

    SCOPED(Mesh) mesh = createTexturedUnitCubeMesh();

    // Free fly default camera, with WASD/ZQSD, left Maj/Ctrl, and mouse
    getCamera()->position = float4(4.0f, 0.0f, 0.0f);

    Transform tr = {};
    setIdentity(&tr);

    void myDraw() {
        pushRenderState();
        getRenderState()->rasterState.cullMode = CullMode_Back;
        getRenderState()->rasterState.sampleShadingEnable = true; // run pixel shader once per AA sample instead of once per pixel

        pushMatrix(&tr);
        rotate(&tr, 2.9e-2f * getAnimTime(), X_AXIS);
        rotate(&tr, 3.1e-2f * getAnimTime(), Y_AXIS);
        rotate(&tr, 3.7e-2f * getAnimTime(), Z_AXIS);

        clearTexture(msaaTex, float4());
        clearTexture(msaaDepth, float4());
        useFramebuffer(msaaFB);

        useShader(shader);
        setUniformTexture(texture, "Texture");
        setUniformMat4(tr.mat, "ModelMatrix");

        drawMesh(mesh);

        popMatrix(&tr);
        popRenderState();

        copyTexture(getSwapchainTexture(), msaaTex); // AA resolve
    }

    bool myEvents() {
        ON_CLICK_ONCE(LEFT , grabInput(true ));
        ON_CLICK_ONCE(RIGHT, grabInput(false));
        ON_KEY_ONCE(SPACE, pauseAnimations(!animationsPaused()));
        return KEY_PRESSED(ESCAPE);
    }

    launchApplication(myDraw, myEvents, NULL);
    return 0;
}
