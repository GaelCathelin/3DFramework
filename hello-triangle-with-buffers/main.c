#include <engine.h>

static Mesh WARN_UNUSED_RESULT createHelloTriangleMesh() {
    const float vertices[][2] = {{-0.5f, 0.5f     }, {0.0f, -0.5f     }, {0.5f, 0.5f      }};
    const float colors  [][3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};

    Mesh mesh = createMesh(PrimitiveType_Triangles);
    addMeshAttrib(mesh, RG32_FLOAT , false, vertices, ARRAY_SIZE(vertices));
    addMeshAttrib(mesh, RGB32_FLOAT, false, colors  , ARRAY_SIZE(colors  ));
    return mesh;
}

int main(int, char**) {
    SCOPED(Application) app = initApplication("Hello Triangle with Buffers", 1024, 768, SRGB_FLAG | VSYNC_FLAG);
    SCOPED(Shader) shader = loadShader(SHADER_DIR "shader");
    SCOPED(Mesh) mesh = createHelloTriangleMesh();

    void myDraw() {
        useFramebuffer(getSwapchainFramebuffer());
        useShader(shader);
        drawMesh(mesh);
    }

    bool myEvents() {return KEY_PRESSED(ESCAPE);}
    launchApplication(myDraw, myEvents, NULL);
    return 0;
}
