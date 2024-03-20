#include <shader.h>
#include <camera.h>
#include <file.h>
#include <framebuffer.h>
#include <functional>
#include <graphics_states.h>
#include <mesh.h>
#include <nvrhi/utils.h>
#include <spirv_cross/spirv_cross_c.h>
#include <stdarg.h>
#include "private_impl.h"
#include "private_log.h"

static const char SPIRV_EXTENSIONS[][11] = {
    ".task.spv", ".mesh.spv", ".vert.spv", ".tesc.spv", ".tese.spv", ".geom.spv", ".frag.spv", ".comp.spv"
};

static const nvrhi::ShaderType SHADER_STAGES[] = {
    nvrhi::ShaderType::Amplification,
    nvrhi::ShaderType::Mesh,
    nvrhi::ShaderType::Vertex,
    nvrhi::ShaderType::Hull,
    nvrhi::ShaderType::Domain,
    nvrhi::ShaderType::Geometry,
    nvrhi::ShaderType::Pixel,
    nvrhi::ShaderType::Compute,
};

struct Uniform {
    uint offset;
    size_t size;
};

struct UniformTexture {
    uint binding, mipmap, nbMipmaps, layer, nbLayers;
    nvrhi::ResourceType type;
    Texture texture;
};

struct UniformBuffer {
    uint binding;
    Buffer buffer;
};

struct UniformAccelerationStructure {
    uint binding;
    AccelerationStructure as;
};

struct Binding {
    uint binding, size;
    nvrhi::ResourceType type;
};

struct UInt4Hash {
    size_t operator()(const UInt4 &v) const {
        return v[0] | (v[1] << 10) | (v[2] << 20);
    }
};

struct UInt4Equal {
    bool operator()(const UInt4 &a, const UInt4 &b) const {
        return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
    }
};

static void renderStateHash(size_t &seed, const nvrhi::RenderState &d) {
    nvrhi::hash_combine(seed, d.blendState);
    nvrhi::hash_combine(seed, d.depthStencilState.backFaceStencil.depthFailOp);
    nvrhi::hash_combine(seed, d.depthStencilState.backFaceStencil.failOp);
    nvrhi::hash_combine(seed, d.depthStencilState.backFaceStencil.passOp);
    nvrhi::hash_combine(seed, d.depthStencilState.backFaceStencil.stencilFunc);
    nvrhi::hash_combine(seed, d.depthStencilState.depthFunc);
    nvrhi::hash_combine(seed, d.depthStencilState.depthTestEnable);
    nvrhi::hash_combine(seed, d.depthStencilState.depthWriteEnable);
    nvrhi::hash_combine(seed, d.depthStencilState.frontFaceStencil.depthFailOp);
    nvrhi::hash_combine(seed, d.depthStencilState.frontFaceStencil.failOp);
    nvrhi::hash_combine(seed, d.depthStencilState.frontFaceStencil.passOp);
    nvrhi::hash_combine(seed, d.depthStencilState.frontFaceStencil.stencilFunc);
    nvrhi::hash_combine(seed, d.depthStencilState.stencilEnable);
    nvrhi::hash_combine(seed, d.depthStencilState.stencilReadMask);
    nvrhi::hash_combine(seed, d.depthStencilState.stencilRefValue);
    nvrhi::hash_combine(seed, d.depthStencilState.stencilWriteMask);
    nvrhi::hash_combine(seed, d.depthStencilState.dynamicStencilRef);
    nvrhi::hash_combine(seed, d.rasterState.antialiasedLineEnable);
    nvrhi::hash_combine(seed, d.rasterState.conservativeRasterEnable);
    nvrhi::hash_combine(seed, d.rasterState.cullMode);
    nvrhi::hash_combine(seed, d.rasterState.depthBias);
    nvrhi::hash_combine(seed, d.rasterState.depthBiasClamp);
    nvrhi::hash_combine(seed, d.rasterState.depthClipEnable);
    nvrhi::hash_combine(seed, d.rasterState.fillMode);
    nvrhi::hash_combine(seed, d.rasterState.forcedSampleCount);
    nvrhi::hash_combine(seed, d.rasterState.frontCounterClockwise);
    nvrhi::hash_combine(seed, d.rasterState.rasterizerDiscard);
    nvrhi::hash_combine(seed, d.rasterState.sampleShadingEnable);
    nvrhi::hash_combine(seed, d.rasterState.programmableSamplePositionsEnable);
    nvrhi::hash_combine(seed, d.rasterState.quadFillEnable);
    for (uint i = 0; i < ARRAY_SIZE(d.rasterState.samplePositionsX); i++) {
        nvrhi::hash_combine(seed, d.rasterState.samplePositionsX[i]);
        nvrhi::hash_combine(seed, d.rasterState.samplePositionsY[i]);
    }
    nvrhi::hash_combine(seed, d.rasterState.scissorEnable);
    nvrhi::hash_combine(seed, d.rasterState.slopeScaledDepthBias);
}

static void framebufferInfoEx(size_t &seed, const nvrhi::FramebufferInfoEx &d) {
    for (uint i = 0; i < d.colorFormats.size(); i++) nvrhi::hash_combine(seed, d.colorFormats[i]);
    nvrhi::hash_combine(seed, d.depthFormat);
    nvrhi::hash_combine(seed, d.sampleCount);
    nvrhi::hash_combine(seed, d.sampleQuality);
    nvrhi::hash_combine(seed, d.width);
    nvrhi::hash_combine(seed, d.height);
}

struct GraphicsPipeKey {
    nvrhi::GraphicsPipelineDesc pipeDesc;
    nvrhi::FramebufferInfoEx fbDesc;
};

struct GraphicsDescHash {
    size_t operator()(const GraphicsPipeKey &d) const {
        size_t seed = 205;
        renderStateHash(seed, d.pipeDesc.renderState);
        if (d.pipeDesc.inputLayout) {
            for (uint i = 0; i < d.pipeDesc.inputLayout->getNumAttributes(); i++) {
                const nvrhi::VertexAttributeDesc *desc = d.pipeDesc.inputLayout->getAttributeDesc(i);
                nvrhi::hash_combine(seed, desc->bufferIndex);
                nvrhi::hash_combine(seed, desc->elementStride);
                nvrhi::hash_combine(seed, desc->format);
                nvrhi::hash_combine(seed, desc->isInstanced);
            }
        }
        nvrhi::hash_combine(seed, d.pipeDesc.primType);
        nvrhi::hash_combine(seed, d.pipeDesc.patchControlPoints);
        nvrhi::hash_combine(seed, d.pipeDesc.shadingRateState.enabled);
        nvrhi::hash_combine(seed, d.pipeDesc.shadingRateState.imageCombiner);
        nvrhi::hash_combine(seed, d.pipeDesc.shadingRateState.pipelinePrimitiveCombiner);
        nvrhi::hash_combine(seed, d.pipeDesc.shadingRateState.shadingRate);
        framebufferInfoEx(seed, d.fbDesc);
        return seed;
    }
};

struct GraphicsDescEqual {
    bool operator()(const GraphicsPipeKey &a, const GraphicsPipeKey &b) const {
        return
            a.pipeDesc.primType == b.pipeDesc.primType &&
            a.pipeDesc.patchControlPoints == b.pipeDesc.patchControlPoints &&
            !memcmp(&a.pipeDesc.renderState, &b.pipeDesc.renderState, sizeof(BlendState) + sizeof(DepthStencilState) + sizeof(RasterState)) &&
            a.pipeDesc.shadingRateState == b.pipeDesc.shadingRateState &&
            a.fbDesc == b.fbDesc;
    }
};

struct MeshletPipeKey {
    UInt4 groupSize;
    nvrhi::MeshletPipelineDesc pipeDesc;
    nvrhi::FramebufferInfoEx fbDesc;
};

struct MeshletDescHash {
    size_t operator()(const MeshletPipeKey &d) const {
        size_t seed = 205;
        nvrhi::hash_combine(seed, UInt4Hash()(d.groupSize));
        nvrhi::hash_combine(seed, d.pipeDesc.primType);
        renderStateHash(seed, d.pipeDesc.renderState);
        framebufferInfoEx(seed, d.fbDesc);
        return seed;
    }
};

struct MeshletDescEqual {
    bool operator()(const MeshletPipeKey &a, const MeshletPipeKey &b) const {
        const bool groupSizeEqual = UInt4Equal()(a.groupSize, b.groupSize);
        const bool primTypeEqual = a.pipeDesc.primType == b.pipeDesc.primType;
        const bool renderStateEqual = !memcmp(&a.pipeDesc.renderState, &b.pipeDesc.renderState, sizeof(BlendState) + sizeof(DepthStencilState) + sizeof(RasterState));
        const bool fbDescEqual = a.fbDesc == b.fbDesc;
        return groupSizeEqual && primTypeEqual && renderStateEqual && fbDescEqual;
    }
};

enum PipelineType {
    PipelineType_Graphics,
    PipelineType_Compute,
    PipelineType_Meshlet,
    PipelineType_Invalid
};

struct ShaderImpl {
    char name[256];
    std::unordered_multimap<std::string, Uniform> uniforms;
    std::unordered_multimap<std::string, UniformTexture> textures;
    std::unordered_multimap<std::string, UniformBuffer> buffers;
    UniformAccelerationStructure accelerationStructure;
    std::vector<Binding> bindings;
    nvrhi::BindingSetHandle bindingSet;
    nvrhi::GraphicsPipelineDesc graphicsDesc;
    nvrhi::ComputePipelineDesc computeDesc;
    nvrhi::MeshletPipelineDesc meshletDesc;
    UInt4 computeGroupSize;
    void *stagingUniforms;
    uint stagingSize;
    nvrhi::BufferHandle uniformBuffer;
    bool uniformsDirty;
    PipelineType pipeType;
    std::unordered_map<GraphicsPipeKey, nvrhi::GraphicsPipelineHandle, GraphicsDescHash, GraphicsDescEqual> graphicsPipeCache;
    std::unordered_map<UInt4, nvrhi::ComputePipelineHandle, UInt4Hash, UInt4Equal> computePipeCache;
    std::unordered_map<MeshletPipeKey, nvrhi::MeshletPipelineHandle, MeshletDescHash, MeshletDescEqual> meshletPipeCache;
};

static const nvrhi::VulkanBindingOffsets bindingOffsets = {0, 0, 0, 0};

static ShaderImpl *current = nullptr;

static uint padUniformBufferSize(const uint size) {
    return (size + nvrhi::c_ConstantBufferOffsetSizeAlignment - 1) / nvrhi::c_ConstantBufferOffsetSizeAlignment * nvrhi::c_ConstantBufferOffsetSizeAlignment;
}

static void merge(nvrhi::BindingLayoutDesc &dest, const nvrhi::BindingLayoutDesc &src) {
    for (const nvrhi::BindingLayoutItem &item : src.bindings)
        dest.bindings.push_back(item);
}

static nvrhi::BindingLayoutDesc reflectShader(ShaderImpl *shader, const nvrhi::ShaderType stage, const void *binary, const size_t size) {
    nvrhi::BindingLayoutDesc bindingLayoutDesc = nvrhi::BindingLayoutDesc();

    if (!binary || size == 0)
        return bindingLayoutDesc;

    nvrhi::ShaderHandle handle = getDevice()->createShader(nvrhi::ShaderDesc(stage), binary, size);
    switch (stage) {
        case nvrhi::ShaderType::Compute      : shader->computeDesc .setComputeShader      (handle); break;
        case nvrhi::ShaderType::Amplification: shader->meshletDesc .setAmplificationShader(handle); break;
        case nvrhi::ShaderType::Mesh         : shader->meshletDesc .setMeshShader         (handle); break;
        case nvrhi::ShaderType::Vertex       : shader->graphicsDesc.setVertexShader       (handle); break;
        case nvrhi::ShaderType::Hull         : shader->graphicsDesc.setHullShader         (handle); break;
        case nvrhi::ShaderType::Domain       : shader->graphicsDesc.setDomainShader       (handle); break;
        case nvrhi::ShaderType::Geometry     : shader->graphicsDesc.setGeometryShader     (handle); break;
        case nvrhi::ShaderType::Pixel        :
            shader->graphicsDesc.setFragmentShader(handle);
            shader->meshletDesc .setFragmentShader(handle);
            break;
        default:;
    }

    spvc_context context;
    spvc_parsed_ir parsed;
    spvc_compiler compiler;
    spvc_resources resources;
    VERIFY(SPVC_SUCCESS == spvc_context_create(&context));
    VERIFY(SPVC_SUCCESS == spvc_context_parse_spirv(context, (SpvId*)binary, size / 4, &parsed));
    VERIFY(SPVC_SUCCESS == spvc_context_create_compiler(context, SPVC_BACKEND_GLSL, parsed, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler));
    VERIFY(SPVC_SUCCESS == spvc_compiler_create_shader_resources(compiler, &resources));

    auto parse = [&](const spvc_resource_type resType, std::function<void (const spvc_reflected_resource, const uint)> const &parseFunc) {
        const spvc_reflected_resource *list;
        size_t nbResources;
        spvc_resources_get_resource_list_for_type(resources, resType, &list, &nbResources);

        for (uint j = 0; j < nbResources; j++) {
            const uint binding = spvc_compiler_get_decoration(compiler, list[j].id, SpvDecorationBinding);
            Binding bindingInfo = {binding, shader->bindings.empty() ? 0 : shader->bindings.back().size};
            shader->bindings.push_back(bindingInfo);
            parseFunc(list[j], binding);
        }
    };

    auto uniformBufferParsing = [&](const spvc_reflected_resource resource, const uint binding) {
        shader->bindings.back().type = nvrhi::ResourceType::VolatileConstantBuffer;
        bindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(binding));
        spvc_type resourceType = spvc_compiler_get_type_handle(compiler, resource.base_type_id);
        const uint nbMembers = spvc_type_get_num_member_types(resourceType);
        for (uint k = 0; k < nbMembers; k++) {
            spvc_type memberType = spvc_compiler_get_type_handle(compiler, spvc_type_get_member_type(resourceType, k));
            if (spvc_type_get_num_array_dimensions(memberType) > 0) {
                const uint arraySize = spvc_type_get_array_dimension(memberType, 0);
                uint stride;
                spvc_compiler_type_struct_member_array_stride(compiler, resourceType, k, &stride);

                for (uint l = 0; l < arraySize; l++) {
                    Uniform uniform = {};
                    spvc_compiler_type_struct_member_offset(compiler, resourceType, k, &uniform.offset);
                    uniform.offset += shader->bindings.back().size + l * stride;
                    uniform.size = stride;
                    char name[64];
                    sprintf(name, "%s[%u]", spvc_compiler_get_member_name(compiler, resource.base_type_id, k), l);
                    shader->uniforms.insert(std::make_pair(name, uniform));
                }
            } else {
                Uniform uniform = {};
                spvc_compiler_type_struct_member_offset(compiler, resourceType, k, &uniform.offset);
                uniform.offset += shader->bindings.back().size;
                spvc_compiler_get_declared_struct_member_size(compiler, resourceType, k, &uniform.size);
                shader->uniforms.insert(std::make_pair(spvc_compiler_get_member_name(compiler, resource.base_type_id, k), uniform));
            }
        }

        size_t paddedSize;
        spvc_compiler_get_declared_struct_size(compiler, resourceType, &paddedSize);
        shader->bindings.back().size += padUniformBufferSize(paddedSize);
    };

    auto storageBufferParsing = [&](const spvc_reflected_resource resource, const uint binding) {
        shader->bindings.back().type = nvrhi::ResourceType::RawBuffer_UAV;
        bindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::RawBuffer_UAV(binding));
        shader->buffers.insert(std::make_pair(resource.name, UniformBuffer{binding}));
    };

    auto textureParsing = [&](const spvc_reflected_resource resource, const nvrhi::ResourceType type, const uint binding) {
        shader->bindings.back().type = type;
        spvc_type resourceType = spvc_compiler_get_type_handle(compiler, resource.type_id);
        if (spvc_type_get_num_array_dimensions(resourceType) == 0) {
            bindingLayoutDesc.addItem(type == nvrhi::ResourceType::Texture_UAV ?
                nvrhi::BindingLayoutItem::Texture_UAV(binding) :
                nvrhi::BindingLayoutItem::Texture_SRV(binding));
            shader->textures.insert(std::make_pair(resource.name, UniformTexture{binding, 0, ~0u, 0, ~0u, type}));
        } else {
            const uint arraySize = spvc_type_get_array_dimension(resourceType, 0);
            for (uint k = 0; k < arraySize; k++) {
                char name[64];
                sprintf(name, "%s[%u]", resource.name, k);
                bindingLayoutDesc.addItem(type == nvrhi::ResourceType::Texture_UAV ?
                    nvrhi::BindingLayoutItem::Texture_UAV(binding) :
                    nvrhi::BindingLayoutItem::Texture_SRV(binding));
                shader->textures.insert(std::make_pair(name, UniformTexture{binding, 0, ~0u, 0, ~0u, type}));
            }
        }
    };

    auto combinedImageSamplerParsing = [&](const spvc_reflected_resource resource, const uint binding) {textureParsing(resource, nvrhi::ResourceType::Texture_SRV, binding);};
    auto storageImageParsing         = [&](const spvc_reflected_resource resource, const uint binding) {textureParsing(resource, nvrhi::ResourceType::Texture_UAV, binding);};

    auto accelerationStructureParsing = [&](const spvc_reflected_resource resource, const uint binding) {
        UNUSED(resource);
        shader->bindings.back().type = nvrhi::ResourceType::RayTracingAccelStruct;
        bindingLayoutDesc.addItem(nvrhi::BindingLayoutItem::RayTracingAccelStruct(binding));
        shader->accelerationStructure.binding = binding;
        shader->accelerationStructure.as.impl = nullptr;
    };

    parse(SPVC_RESOURCE_TYPE_UNIFORM_BUFFER        , uniformBufferParsing        );
    parse(SPVC_RESOURCE_TYPE_STORAGE_BUFFER        , storageBufferParsing        );
    parse(SPVC_RESOURCE_TYPE_SAMPLED_IMAGE         , combinedImageSamplerParsing );
    parse(SPVC_RESOURCE_TYPE_STORAGE_IMAGE         , storageImageParsing         );
    parse(SPVC_RESOURCE_TYPE_ACCELERATION_STRUCTURE, accelerationStructureParsing);

    spvc_context_destroy(context);
    return bindingLayoutDesc;
}

static Shader finalizeShader(ShaderImpl *shader, nvrhi::BindingLayoutDesc &bindingLayoutDesc) {
    std::sort(bindingLayoutDesc.bindings.begin(), bindingLayoutDesc.bindings.end(), [](const nvrhi::BindingLayoutItem &a, const nvrhi::BindingLayoutItem &b) {return a.slot < b.slot;});

    switch (shader->pipeType) {
        case PipelineType_Graphics:
            shader->graphicsDesc.setPatchControlPoints(4);
            shader->graphicsDesc.addBindingLayout(getDevice()->createBindingLayout(bindingLayoutDesc));
            break;

        case PipelineType_Compute:
            shader->computeGroupSize = uint4(8, 8, 1);
            shader->computeDesc.addBindingLayout(getDevice()->createBindingLayout(bindingLayoutDesc));
            break;

        case PipelineType_Meshlet:
            shader->computeGroupSize = uint4(64, 1, 1);
            shader->meshletDesc.addBindingLayout(getDevice()->createBindingLayout(bindingLayoutDesc));
            break;

        default:;
    }

    shader->stagingSize = shader->bindings.empty() ? 0 : padUniformBufferSize(shader->bindings.back().size);
    if (!shader->bindings.empty() && shader->stagingSize > 0) {
        nvrhi::BufferDesc desc = nvrhi::BufferDesc()
            .setIsConstantBuffer(true).setIsVolatile(true)
            .setByteSize(shader->stagingSize).setMaxVersions((1 << 22) / shader->stagingSize);
        shader->uniformBuffer = getDevice()->createBuffer(desc);
        shader->stagingUniforms = malloc(shader->stagingSize);
        shader->uniformsDirty = true;
    }

    return Shader{shader};
}

static nvrhi::Viewport flipViewport(const ViewportState &viewport) {
    return nvrhi::Viewport(viewport.minX, viewport.maxX, viewport.maxY, viewport.minY, viewport.minZ, viewport.maxZ);
}

extern "C" {

Shader createGraphicsShader(
    const void *vertBin, const size_t vertSize,
    const void *tescBin, const size_t tescSize,
    const void *teseBin, const size_t teseSize,
    const void *geomBin, const size_t geomSize,
    const void *fragBin, const size_t fragSize)
{
    ShaderImpl *shader = new ShaderImpl();
    shader->pipeType = PipelineType_Graphics;

    nvrhi::BindingLayoutDesc bindingLayoutDesc = nvrhi::BindingLayoutDesc().setVisibility(nvrhi::ShaderType::All).setBindingOffsets(bindingOffsets);
    merge(bindingLayoutDesc, reflectShader(shader, nvrhi::ShaderType::Vertex  , vertBin, vertSize));
    merge(bindingLayoutDesc, reflectShader(shader, nvrhi::ShaderType::Hull    , tescBin, tescSize));
    merge(bindingLayoutDesc, reflectShader(shader, nvrhi::ShaderType::Domain  , teseBin, teseSize));
    merge(bindingLayoutDesc, reflectShader(shader, nvrhi::ShaderType::Geometry, geomBin, geomSize));
    merge(bindingLayoutDesc, reflectShader(shader, nvrhi::ShaderType::Pixel   , fragBin, fragSize));
    return finalizeShader(shader, bindingLayoutDesc);
}

Shader createComputeShader(const void *binary, const size_t size) {
    ShaderImpl *shader = new ShaderImpl();
    shader->pipeType = PipelineType_Compute;
    nvrhi::BindingLayoutDesc bindingLayoutDesc = nvrhi::BindingLayoutDesc().setVisibility(nvrhi::ShaderType::Compute).setBindingOffsets(bindingOffsets);
    merge(bindingLayoutDesc, reflectShader(shader, nvrhi::ShaderType::Compute, binary, size));
    return finalizeShader(shader, bindingLayoutDesc);
}

Shader loadShader(const char *shaderName) {
    ShaderImpl *shader = new ShaderImpl();
    strcpy(shader->name, shaderName);
    shader->pipeType = PipelineType_Invalid;

    nvrhi::BindingLayoutDesc bindingLayoutDesc = nvrhi::BindingLayoutDesc().setVisibility(nvrhi::ShaderType::All).setBindingOffsets(bindingOffsets);
    for (uint i = 0; i < ARRAY_SIZE(SHADER_STAGES); i++) {
        char filename[256];
        sprintf(filename, "%s%s", shaderName, SPIRV_EXTENSIONS[i]);
        if (!fileExists(filename))
            continue;

        if (SHADER_STAGES[i] == nvrhi::ShaderType::Compute)
            shader->pipeType = PipelineType_Compute;
        else if (SHADER_STAGES[i] == nvrhi::ShaderType::Amplification || SHADER_STAGES[i] == nvrhi::ShaderType::Mesh)
            shader->pipeType = PipelineType_Meshlet;
        else if (SHADER_STAGES[i] != nvrhi::ShaderType::Pixel)
            shader->pipeType = PipelineType_Graphics;

        SCOPED(File) shaderFile = loadFile(filename);
        merge(bindingLayoutDesc, reflectShader(shader, SHADER_STAGES[i], shaderFile.data, shaderFile.size));
    }

    if (shader->pipeType == PipelineType_Invalid) {
        logError("No shader found with name '%s'", shaderName);
        delete shader;
        return Shader();
    }

    return finalizeShader(shader, bindingLayoutDesc);
}

void deleteShader(Shader *shader) {
    if (!shader || !shader->impl)
        return;

    if (current == shader->impl) current = nullptr;

    ShaderImpl* impl = (ShaderImpl*)shader->impl;
    if (impl->stagingUniforms) {
        impl->uniformBuffer.Reset();
        free(impl->stagingUniforms);
    }
    delete impl;
    shader->impl = nullptr;
}

void useShader(Shader shader) {
    current = (ShaderImpl*)shader.impl;

    INHIBIT_LOG;
    setUniformMat4(getCamera()->projection.mat, "ProjectionMatrix");
    setUniformMat4(getCamera()->view.mat, "ViewMatrix");
}

static void setUniformGeneric(const void *data, const uint size, const char *name) {
    if (!current) {
        logError("Setting an uniform to an invalid shader");
        return;
    }

    auto range = current->uniforms.equal_range(name);
    if (range.first == range.second) {
        logWarning("Uniform \"%s\" not found", name);
        return;
    }

    current->uniformsDirty = true;
    for (auto it = range.first; it != range.second; it++) {
        if (size != it->second.size) {
            logError("Uniform size mismatch");
            continue;
        }

        memcpy(current->stagingUniforms + it->second.offset, data, size);
    }
}

#define GET_FULLNAME \
    char fullname[64]; \
    va_list args; \
    va_start(args, name); \
    vsprintf(fullname, name, args); \
    va_end(args); \

void setUniform1f(const float  x,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&x, sizeof(float), fullname);}
void setUniform2f(const float  x, const float  y,                                 const char *name, ...) {GET_FULLNAME; float v[] = {x, y}; setUniformGeneric(v, sizeof(float[2]), fullname);}
void setUniform3f(const float  x, const float  y, const float  z,                 const char *name, ...) {GET_FULLNAME; float v[] = {x, y, z}; setUniformGeneric(v, sizeof(float[3]), fullname);}
void setUniform4f(const float  x, const float  y, const float  z, const float  w, const char *name, ...) {GET_FULLNAME; float v[] = {x, y, z, w}; setUniformGeneric(v, sizeof(float[4]), fullname);}
void setUniform1d(const double x,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&x, sizeof(double), fullname);}
void setUniform2d(const double x, const double y,                                 const char *name, ...) {GET_FULLNAME; double v[] = {x, y}; setUniformGeneric(v, sizeof(double[2]), fullname);}
void setUniform3d(const double x, const double y, const double z,                 const char *name, ...) {GET_FULLNAME; double v[] = {x, y, z}; setUniformGeneric(v, sizeof(double[3]), fullname);}
void setUniform4d(const double x, const double y, const double z, const double w, const char *name, ...) {GET_FULLNAME; double v[] = {x, y, z, w}; setUniformGeneric(v, sizeof(double[4]), fullname);}
void setUniform1s(const short  x,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&x, sizeof(short), fullname);}
void setUniform2s(const short  x, const short  y,                                 const char *name, ...) {GET_FULLNAME; short v[] = {x, y}; setUniformGeneric(v, sizeof(short[2]), fullname);}
void setUniform3s(const short  x, const short  y, const short  z,                 const char *name, ...) {GET_FULLNAME; short v[] = {x, y, z}; setUniformGeneric(v, sizeof(short[3]), fullname);}
void setUniform4s(const short  x, const short  y, const short  z, const short  w, const char *name, ...) {GET_FULLNAME; short v[] = {x, y, z, w}; setUniformGeneric(v, sizeof(short[4]), fullname);}
void setUniform1i(const int    x,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&x, sizeof(int), fullname);}
void setUniform2i(const int    x, const int    y,                                 const char *name, ...) {GET_FULLNAME; int v[] = {x, y}; setUniformGeneric(v, sizeof(int[2]), fullname);}
void setUniform3i(const int    x, const int    y, const int    z,                 const char *name, ...) {GET_FULLNAME; int v[] = {x, y, z}; setUniformGeneric(v, sizeof(int[3]), fullname);}
void setUniform4i(const int    x, const int    y, const int    z, const int    w, const char *name, ...) {GET_FULLNAME; int v[] = {x, y, z, w}; setUniformGeneric(v, sizeof(int[4]), fullname);}
void setUniform1l(const long   x,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&x, sizeof(long), fullname);}
void setUniform2l(const long   x, const long   y,                                 const char *name, ...) {GET_FULLNAME; long v[] = {x, y}; setUniformGeneric(v, sizeof(long[2]), fullname);}
void setUniform3l(const long   x, const long   y, const long   z,                 const char *name, ...) {GET_FULLNAME; long v[] = {x, y, z}; setUniformGeneric(v, sizeof(long[3]), fullname);}
void setUniform4l(const long   x, const long   y, const long   z, const long   w, const char *name, ...) {GET_FULLNAME; long v[] = {x, y, z, w}; setUniformGeneric(v, sizeof(long[4]), fullname);}
void setUniform1F(const Float4 v,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&v, sizeof(float), fullname);}
void setUniform2F(const Float4 v,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&v, sizeof(float[2]), fullname);}
void setUniform3F(const Float4 v,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&v, sizeof(float[3]), fullname);}
void setUniform4F(const Float4 v,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&v, sizeof(float[4]), fullname);}
void setUniform1I(const Int4   v,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&v, sizeof(int), fullname);}
void setUniform2I(const Int4   v,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&v, sizeof(int[2]), fullname);}
void setUniform3I(const Int4   v,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&v, sizeof(int[3]), fullname);}
void setUniform4I(const Int4   v,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(&v, sizeof(int[4]), fullname);}
void setUniformMat4(const Mat4 m,                                                 const char *name, ...) {GET_FULLNAME; setUniformGeneric(m, sizeof(float[16]), fullname);}
void setUniformMat3(const Mat4 m,                                                 const char *name, ...) {GET_FULLNAME;
    const float f[9] = {m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2],};
    setUniformGeneric(f, sizeof(float[9]), fullname);
}

void setUniformTextureView(Texture texture, const uint mipmap, const uint nbMipmaps, const uint layer, const uint nbLayers, const char *name, ...) {
    GET_FULLNAME;

    if (!texture.impl) {
        logWarning("Setting an invalid uniform texture (\"%s\")", fullname);
        return;
    }

    if (!current) {
        logError("Setting an uniform texture to an invalid shader");
        return;
    }

    auto range = current->textures.equal_range(fullname);
    if (range.first == range.second) {
        logWarning("Uniform \"%s\" not found", fullname);
        return;
    }

    for (auto it = range.first; it != range.second; it++) {
        it->second.texture = texture;
        it->second.mipmap = mipmap;
        it->second.nbMipmaps = nbMipmaps;
        it->second.layer = layer;
        it->second.nbLayers = nbLayers;
    }

    current->bindingSet.Reset();
}

void setUniformBuffer(Buffer buffer, const char *name, ...) {
    GET_FULLNAME;

    if (!buffer.impl) {
        logWarning("Setting an invalid uniform buffer (\"%s\")", fullname);
        return;
    }

    if (!current) {
        logError("Setting an uniform buffer to an invalid shader");
        return;
    }

    auto range = current->buffers.equal_range(fullname);
    if (range.first == range.second) {
        logWarning("Uniform \"%s\" not found", fullname);
        return;
    }

    for (auto it = range.first; it != range.second; it++)
        it->second.buffer = buffer;

    current->bindingSet.Reset();
}

void setUniformAccelerationStructure(AccelerationStructure as) {
    if (!as.impl) {
        logWarning("Setting an invalid acceleration structure");
        return;
    }

    if (!current) {
        logError("Setting an acceleration structure to an invalid shader");
        return;
    }

    current->accelerationStructure.as = as;
    current->bindingSet.Reset();
}

void setTessellationControlPoints(const uint nbPoints) {
    if (!current) {
        logError("Setting patch control points of an invalid shader");
        return;
    }

    if (current->pipeType != PipelineType_Graphics) logWarning("Setting patch control points of an non graphics shader");
    current->graphicsDesc.setPatchControlPoints(nbPoints);
}

void setGroupSize1D(const uint nbElemsX) {setGroupSize3D(nbElemsX, 1, 1);}
void setGroupSize2D(const uint nbElemsX, const uint nbElemsY) {setGroupSize3D(nbElemsX, nbElemsY, 1);}
void setGroupSize3D(const uint nbElemsX, const uint nbElemsY, const uint nbElemsZ) {
    if (!current) {
        logError("Setting compute group size of an invalid shader");
        return;
    }

    if (current->pipeType == PipelineType_Graphics) logWarning("Setting compute group size of an non compute shader");
    current->computeGroupSize = uint4(nbElemsX, nbElemsY, nbElemsZ);
}

static nvrhi::IBindingSet* bindUniforms(nvrhi::IBindingLayout *bindingLayout) {
    if (!current) {
        logError("Submit uniforms to an invalid shader");
        return nullptr;
    }

    if (current->uniformsDirty && current->stagingSize > 0) {
        current->uniformsDirty = false;
        getCommandList()->writeBuffer(current->uniformBuffer, current->stagingUniforms, current->stagingSize);
    }

    if (!current->bindingSet) {
        nvrhi::BindingSetDesc bindingSetDesc = nvrhi::BindingSetDesc();

        for (uint i = 0; i < current->bindings.size(); i++) {
            const Binding &b = current->bindings[i];
            if (b.type == nvrhi::ResourceType::VolatileConstantBuffer) {
                const uint64_t offset = i == 0 ? 0 : current->bindings[i - 1].size;
                bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(b.binding, current->uniformBuffer, nvrhi::BufferRange(offset, b.size - offset)));
            }
        }

        for (auto &it : current->textures) {
            if (it.second.type == nvrhi::ResourceType::Texture_UAV) {
                nvrhi::TextureSubresourceSet view = nvrhi::TextureSubresourceSet(it.second.mipmap, 1, it.second.layer, it.second.nbLayers);
                bindingSetDesc.addItem(nvrhi::BindingSetItem::Texture_UAV(it.second.binding, getNvTexture(it.second.texture), nvrhi::Format::UNKNOWN, view));
            } else {
                nvrhi::TextureSubresourceSet view = nvrhi::TextureSubresourceSet(it.second.mipmap, it.second.nbMipmaps, it.second.layer, it.second.nbLayers);
                bindingSetDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(it.second.binding, getNvTexture(it.second.texture), getNvSampler(it.second.texture), nvrhi::Format::UNKNOWN, view));
            }
        }

        for (auto &it : current->buffers)
            bindingSetDesc.addItem(nvrhi::BindingSetItem::RawBuffer_UAV(it.second.binding, getNvBuffer(it.second.buffer)));

        if (current->accelerationStructure.as.impl)
            bindingSetDesc.addItem(nvrhi::BindingSetItem::RayTracingAccelStruct(current->accelerationStructure.binding, getAccelerationStructure(current->accelerationStructure.as)->tlas));

        std::sort(bindingSetDesc.bindings.begin(), bindingSetDesc.bindings.end(), [](const nvrhi::BindingSetItem &a, const nvrhi::BindingSetItem &b) {return a.slot < b.slot;});
        current->bindingSet = getDevice()->createBindingSet(bindingSetDesc, bindingLayout);
    }

    return current->bindingSet;
}

static void dispatchCompute(const uint nbElemsX, const uint nbElemsY, const uint nbElemsZ) {
    const auto &pipeIt = current->computePipeCache.find(current->computeGroupSize);
    if (pipeIt == current->computePipeCache.end()) {
        nvrhi::ShaderSpecialization computeGroupSize[3] = {
            nvrhi::ShaderSpecialization::UInt32(0, current->computeGroupSize[0]),
            nvrhi::ShaderSpecialization::UInt32(1, current->computeGroupSize[1]),
            nvrhi::ShaderSpecialization::UInt32(2, current->computeGroupSize[2]),
        };

        nvrhi::ComputePipelineDesc specializedPipeDesc = current->computeDesc;
        specializedPipeDesc.setComputeShader(getDevice()->createShaderSpecialization(current->computeDesc.CS, computeGroupSize, 3));
        current->computePipeCache[current->computeGroupSize] = getDevice()->createComputePipeline(specializedPipeDesc);
    }

    getCommandList()->setComputeState(nvrhi::ComputeState()
        .setPipeline  (current->computePipeCache[current->computeGroupSize])
        .addBindingSet(bindUniforms(current->computeDesc.bindingLayouts[0])));
    getCommandList()->dispatch(
        (nbElemsX + current->computeGroupSize[0] - 1) / current->computeGroupSize[0],
        (nbElemsY + current->computeGroupSize[1] - 1) / current->computeGroupSize[1],
        (nbElemsZ + current->computeGroupSize[2] - 1) / current->computeGroupSize[2]);
}

static void dispatchMeshlet(const uint nbElemsX, const uint nbElemsY, const uint nbElemsZ) {
    pushRenderState();

    if (!getCurrentFramebuffer()->getDesc().depthAttachment.valid())
        getRenderState()->depthStencilState.depthTestEnable = false;

    nvrhi::RenderState nvrhiRenderState = nvrhi::RenderState();
    memcpy((void*)&nvrhiRenderState, getRenderState(), sizeof(BlendState) + sizeof(DepthStencilState) + sizeof(RasterState));

    nvrhi::MeshletPipelineDesc pipelineDesc = current->meshletDesc;
    pipelineDesc.setRenderState(nvrhiRenderState);
    if (getRenderState()->rasterState.rasterizerDiscard)
        pipelineDesc.setPixelShader(nullptr);

    const MeshletPipeKey key = {current->computeGroupSize, pipelineDesc, getCurrentFramebuffer()->getFramebufferInfo()};
    const auto &pipeIt = current->meshletPipeCache.find(key);
    if (pipeIt == current->meshletPipeCache.end()) {
        nvrhi::ShaderSpecialization meshletGroupSize[3] = {
            nvrhi::ShaderSpecialization::UInt32(0, current->computeGroupSize[0]),
            nvrhi::ShaderSpecialization::UInt32(1, current->computeGroupSize[1]),
            nvrhi::ShaderSpecialization::UInt32(2, current->computeGroupSize[2]),
        };

        pipelineDesc.setMeshShader(getDevice()->createShaderSpecialization(pipelineDesc.MS, meshletGroupSize, 3));
        current->meshletPipeCache[key] = getDevice()->createMeshletPipeline(pipelineDesc, getCurrentFramebuffer());
    }

    nvrhi::MeshletState state = nvrhi::MeshletState()
        .setPipeline(current->meshletPipeCache[key])
        .setFramebuffer(getCurrentFramebuffer())
        .setViewport(nvrhi::ViewportState().addViewport(flipViewport(getRenderState()->viewportState)).addScissorRect(nvrhi::Rect(*(nvrhi::Viewport*)&getRenderState()->scissorState)))
        .addBindingSet(bindUniforms(current->meshletDesc.bindingLayouts[0]));

    getCommandList()->setMeshletState(state);
    getCommandList()->dispatchMesh(
        (nbElemsX + current->computeGroupSize[0] - 1) / current->computeGroupSize[0],
        (nbElemsY + current->computeGroupSize[1] - 1) / current->computeGroupSize[1],
        (nbElemsZ + current->computeGroupSize[2] - 1) / current->computeGroupSize[2]);

    popRenderState();
}

void dispatch1D(const uint nbElemsX) {dispatch3D(nbElemsX, 1, 1);}
void dispatch2D(const uint nbElemsX, const uint nbElemsY) {dispatch3D(nbElemsX, nbElemsY, 1);}
void dispatch3D(const uint nbElemsX, const uint nbElemsY, const uint nbElemsZ) {
    if (!current) {
        logError("Dispatch with an invalid shader");
        return;
    }

    if (current->pipeType == PipelineType_Compute)
        dispatchCompute(nbElemsX, nbElemsY, nbElemsZ);
    else if (current->pipeType == PipelineType_Meshlet)
        dispatchMeshlet(nbElemsX, nbElemsY, nbElemsZ);
}

static void drawSubMeshOffsetInstancedIndirect(Mesh mesh, const uint first, const uint count, const uint offset, const uint nbInstances, const uint baseInstance, Buffer indirect) {
    if (!current) {
        logError("Draw with an invalid shader");
        return;
    }

    MeshImpl *meshimpl = getMesh(mesh);

    pushRenderState();

    if (!getCurrentFramebuffer()->getDesc().depthAttachment.valid())
        getRenderState()->depthStencilState.depthTestEnable = false;

    nvrhi::RenderState nvrhiRenderState = nvrhi::RenderState();
    memcpy((void*)&nvrhiRenderState, getRenderState(), sizeof(BlendState) + sizeof(DepthStencilState) + sizeof(RasterState));

    nvrhi::GraphicsPipelineDesc pipelineDesc = current->graphicsDesc;
    pipelineDesc
        .setRenderState(nvrhiRenderState)
        .setPrimType(count < 3 ? nvrhi::PrimitiveType::PointList : nvrhi::PrimitiveType::TriangleList)
        .setVariableRateShadingState(*(nvrhi::VariableRateShadingState*)&getRenderState()->vrsState);

    if (meshimpl) {
        pipelineDesc
            .setPrimType((nvrhi::PrimitiveType)meshimpl->primitiveType)
            .setInputLayout(getDevice()->createInputLayout(meshimpl->attributes.data(), meshimpl->attributes.size(), pipelineDesc.VS));
    }

    if (getRenderState()->rasterState.rasterizerDiscard)
        pipelineDesc.setPixelShader(nullptr);

    const GraphicsPipeKey key = {pipelineDesc, getCurrentFramebuffer()->getFramebufferInfo()};
    if (current->graphicsPipeCache.find(key) == current->graphicsPipeCache.end())
        current->graphicsPipeCache[key] = getDevice()->createGraphicsPipeline(pipelineDesc, getCurrentFramebuffer());

    nvrhi::GraphicsState state = nvrhi::GraphicsState()
        .setPipeline(current->graphicsPipeCache[key])
        .setFramebuffer(getCurrentFramebuffer())
        .setViewport(nvrhi::ViewportState().addViewport(flipViewport(getRenderState()->viewportState)).addScissorRect(nvrhi::Rect(*(nvrhi::Viewport*)&getRenderState()->scissorState)))
        .setShadingRateState(*(nvrhi::VariableRateShadingState*)&getRenderState()->vrsState)
        .addBindingSet(bindUniforms(current->graphicsDesc.bindingLayouts[0]));

    if (meshimpl) {
        state.setIndexBuffer(nvrhi::IndexBufferBinding().setBuffer(getNvBuffer(meshimpl->indices)).setFormat(meshimpl->indicesFormat));
        for (uint i = 0; i < meshimpl->buffers.size(); i++)
            state.addVertexBuffer(nvrhi::VertexBufferBinding().setBuffer(getNvBuffer(meshimpl->buffers[i])).setSlot(i));
    }

    if (indirect.impl)
        state.setIndirectParams(getNvBuffer(indirect));

    getCommandList()->setGraphicsState(state);

    indirect.impl ?
        (meshimpl && meshimpl->nbIndices > 0 ?
            getCommandList()->drawIndexedIndirect(0, 1) :
            getCommandList()->drawIndirect       (0, 1)) :
        (meshimpl && meshimpl->nbIndices > 0 ?
            getCommandList()->drawIndexed(nvrhi::DrawArguments{count, nbInstances, first, offset, baseInstance}) :
            getCommandList()->draw       (nvrhi::DrawArguments{count, nbInstances, 0, first, baseInstance}));

    popRenderState();
}

void drawMesh(Mesh mesh) {drawMeshInstanced(mesh, 1);}
void drawMeshInstanced(Mesh mesh, const uint nbInstances) {drawSubMeshOffsetInstancedIndirect(mesh, 0, getIndexBuffer(mesh).impl ? getNbIndices(mesh) : getNbVertices(mesh), 0, nbInstances, 0, NullBuffer);}
void drawMeshInstancedBaseInstance(Mesh mesh, const uint nbInstances, const uint baseInstance) {drawSubMeshOffsetInstancedIndirect(mesh, 0, getIndexBuffer(mesh).impl ? getNbIndices(mesh) : getNbVertices(mesh), 0, nbInstances, baseInstance, NullBuffer);}
void drawMeshIndirect(Mesh mesh, Buffer indirect) {drawSubMeshOffsetInstancedIndirect(mesh, 0, getIndexBuffer(mesh).impl ? getNbIndices(mesh) : getNbVertices(mesh), 0, 1, 0, indirect);}
void drawSubMesh(Mesh mesh, const uint first, const uint count) {drawSubMeshOffsetInstancedIndirect(mesh, first, count, 0, 1, 0, NullBuffer);}
void drawSubMeshInstanced(Mesh mesh, const uint first, const uint count, const uint nbInstances) {drawSubMeshOffsetInstancedIndirect(mesh, first, count, 0, nbInstances, 0, NullBuffer);}
void drawSubMeshOffsetInstanced(Mesh mesh, const uint first, const uint count, const uint offset, const uint nbInstances, const uint baseInstance) {drawSubMeshOffsetInstancedIndirect(mesh, first, count, offset, nbInstances, baseInstance, NullBuffer);}

}
