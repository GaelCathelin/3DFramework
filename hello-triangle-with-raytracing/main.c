#include <engine.h>

static Mesh WARN_UNUSED_RESULT createHelloTriangleMesh() {
    const float vertices[][2] = {{-0.5f, 0.5f}, {0.0f, -0.5f}, {0.5f, 0.5f}};
    Mesh mesh = createMesh(PrimitiveType_Triangles);
    addMeshAttrib(mesh, RG32_FLOAT, false, vertices, ARRAY_SIZE(vertices));
    return mesh;
}

int main(int, char**) {
    SCOPED(Application) app = initApplication("Hello Triangle Raytracing", 1024, 768, VSYNC_FLAG | RAYTRACING_FLAG);
    SCOPED(Shader) shader = loadShader(SHADER_DIR "shader");
    SCOPED(Mesh) mesh = createHelloTriangleMesh();
    SCOPED(AccelerationStructure) as = createAccelerationStructure(mesh, false);

    void myDraw() {
        useShader(shader);
        setUniformAccelerationStructure(as);
        setUniformTexture(getSwapchainTexture(), "Framebuffer");
        dispatch2D(getWidth(), getHeight());
    }

    bool myEvents() {return KEY_PRESSED(ESCAPE);}
    launchApplication(myDraw, myEvents, NULL);
    return 0;
}
