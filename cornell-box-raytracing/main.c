#include <engine.h>
#include "wavefront.h"

#define SPP  1
#define AA_X 1
#define AA_Y 1

static Buffer WARN_UNUSED_RESULT createSamplingBuffer(const int maxDims) {
    const uint nbCoeffs = maxDims * (maxDims + 1) >> 1;
    uint coeffs[nbCoeffs];

	for (int dim = 1, i = 0; dim <= maxDims; dim++) {
		double r = 1.0, r2 = 0.0;
		while (r2 != r) {
			r2 = r;
			r = pow(1.0 + r, 1.0 / (1.0 + dim));
		}

		for (int c = 1; c <= dim; c++, i++)
            coeffs[i] = round((1ull << 32) * pow(r, -c));
	}

    Buffer buffer = createBuffer(ResourceType_UnorderedAccess, sizeof(coeffs));
    setBufferData(buffer, coeffs, sizeof(coeffs));
    return buffer;
}

int main(int, char**) {
    SCOPED(Application) app = initApplication("Cornell Box", 768, 768, SRGB_FLAG | RAYTRACING_FLAG);
    SCOPED(Texture) bluenoise = loadTexture("../textures/bluenoise256.png", 0);
    SCOPED(Buffer) samplingBuffer = createSamplingBuffer(20);
    SCOPED(Shader) shader = loadShader(SHADER_DIR "shader");
    SCOPED(Shader) resolve = loadShader(SHADER_DIR "resolve");

    SCOPED(Texture) msaaTex = createTexture2D(getWidth(), getHeight(), RGBA32_FLOAT, MSAA_FLAG(log2(AA_X * AA_Y)));
    SCOPED(Texture) msaaDepth = createTexture2D(getWidth(), getHeight(), D32, MSAA_FLAG(log2(AA_X * AA_Y)) | SAMPLES_RELOCATION_FLAG);
    SCOPED(Framebuffer) msaaFB = createFramebuffer(&msaaTex, 1, msaaDepth);

    SCOPED(GPUMeshData) meshData = {};
    SCOPED(Mesh) mesh = loadWavefront("../models/", "cornell.obj", Y_UP, &meshData);
    SCOPED(AccelerationStructure) accel = {};
    SCOPED(Buffer) materials = {};
    {
        struct {Float4 Kd, Ke;} mats[meshData.nbMeshes];
        Range ranges[meshData.nbMeshes];
        for (uint i = 0; i < meshData.nbMeshes; i++) {
            mats[i].Kd = meshData.meshes[i].material->Kd;
            mats[i].Ke = meshData.meshes[i].material->Ke;
            ranges[i] = meshData.meshes[i].range;
        }
        accel = createMultiAccelerationStructure(mesh, ranges, meshData.nbMeshes, false);
        materials = createBuffer(ResourceType_UnorderedAccess, sizeof(mats));
        setBufferData(materials, mats, sizeof(mats));
    }

    getCamera()->position = float4(-3.75f, 0.0f, 1.0f);
    getCamera()->yaw = 0.0f;
    getCamera()->fovy = 40.0f;

    uint frameId = 0;

    void myDraw() {
        if (getCamera()->isMoving) {
            frameId = 0;
            clearTexture(msaaTex, float4());
            clearTexture(msaaDepth, float4());
        }

        pushRenderState();
            getRenderState()->rasterState.cullMode = CullMode_Back;
            getRenderState()->rasterState.sampleShadingEnable = true;
            getRenderState()->rasterState.programmableSampleLocationsEnable = true;
            for (int y = 0, i = 0; y < AA_Y; y++)
            for (int x = 0; x < AA_X; x++, i++)
                setSampleLocation(i, (x + 0.5f) / AA_X, (y + 0.5f) / AA_Y);
            if (frameId > 0) {
                getRenderState()->depthStencilState.depthWriteEnable = false;
                getRenderState()->blendState.targets[0].blendEnable = true;
                setBlendFactors(0, BlendFactor_One, BlendFactor_One, BlendFactor_One, BlendFactor_One);
            }

            useFramebuffer(msaaFB);
            useShader(shader);
            setUniformAccelerationStructure(accel);
            setUniformBuffer(materials, "Materials");
            setUniformTexture(bluenoise, "Bluenoise");
            setUniformBuffer(samplingBuffer, "Sampling");
            setUniform1i(frameId, "frameId");
            setUniform1i(SPP / (AA_X * AA_Y), "spp");
            setUniform2i(AA_X, AA_Y, "aa");
            setUniform2i(AA_X * getWidth(), AA_Y * getHeight(), "resolution");
            setUniform3F(meshData.materials[0].Ke, "LightKe");
            for (uint i = 0; i < meshData.nbMeshes; i++) {
                const Material *mat = meshData.meshes[i].material;
                setUniform3F(mat->Kd, "Kd");
                setUniform3F(mat->Ke, "Ke");
                drawSubMesh(mesh, meshData.meshes[i].range.first, meshData.meshes[i].range.count);
            }
        popRenderState();

        useFramebuffer(getSwapchainFramebuffer());
        useShader(resolve);
        setUniformTexture(msaaTex, "Texture");
        drawSubMesh(NullMesh, 0, 3);

        frameId++;
//        SDL_Delay(50);
    }

    bool myEvents() {
        ON_CLICK_ONCE(LEFT , grabInput(true ));
        ON_CLICK_ONCE(RIGHT, grabInput(false));
        return KEY_PRESSED(ESCAPE);
    }

    launchApplication(myDraw, myEvents, NULL);
    return 0;
}
