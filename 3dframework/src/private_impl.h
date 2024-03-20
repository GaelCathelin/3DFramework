#pragma once

#include <acceleration_structure.h>
#include <buffer.h>
#include <mesh.h>
#include <nvrhi/nvrhi.h>
#include <texture.h>
#include <unordered_map>

nvrhi::IDevice* getDevice();
nvrhi::ICommandList* getCommandList();
extern "C" bool raytracingEnabled();

typedef struct {
    nvrhi::BufferHandle buffer;
	bool mapped;
} BufferImpl;

Buffer createBuffer(const nvrhi::BufferDesc &desc);
nvrhi::BufferDesc getBufferDesc(ResourceType type, const uint size);
nvrhi::IBuffer* getNvBuffer(Buffer &buffer);

typedef struct {
    PrimitiveType primitiveType;
    uint nbIndices, nbVertices;
    std::vector<Buffer> buffers;
    std::vector<nvrhi::VertexAttributeDesc> attributes;
    Buffer indices;
    nvrhi::Format indicesFormat;
} MeshImpl;

MeshImpl* getMesh(Mesh mesh);

typedef struct {
    nvrhi::rt::AccelStructHandle blas, tlas;
} AccelerationStructureImpl;

AccelerationStructureImpl* getAccelerationStructure(AccelerationStructure as);

struct SamplerDescHash {
    size_t operator()(const nvrhi::SamplerDesc &s) const {
        size_t seed = 205;
        nvrhi::hash_combine(seed, s.addressU);
        nvrhi::hash_combine(seed, s.addressV);
        nvrhi::hash_combine(seed, s.addressW);
        nvrhi::hash_combine(seed, s.borderColor.r);
        nvrhi::hash_combine(seed, s.borderColor.g);
        nvrhi::hash_combine(seed, s.borderColor.b);
        nvrhi::hash_combine(seed, s.borderColor.a);
        nvrhi::hash_combine(seed, s.magFilter);
        nvrhi::hash_combine(seed, s.maxAnisotropy);
        nvrhi::hash_combine(seed, s.minFilter);
        nvrhi::hash_combine(seed, s.mipBias);
        nvrhi::hash_combine(seed, s.mipFilter);
        nvrhi::hash_combine(seed, s.reductionType);
        nvrhi::hash_combine(seed, s.comparisonFunc);
        nvrhi::hash_combine(seed, s.minLod);
        nvrhi::hash_combine(seed, s.maxLod);
        return seed;
    }
};

struct SamplerDescEqual {
    bool operator()(const nvrhi::SamplerDesc &a, const nvrhi::SamplerDesc &b) const {
        return !memcmp(&a, &b, sizeof(a));
    }
};

typedef struct {
    nvrhi::TextureHandle texture;
    nvrhi::SamplerDesc state;
    std::unordered_map<nvrhi::SamplerDesc, nvrhi::SamplerHandle, SamplerDescHash, SamplerDescEqual> samplerCache;
    nvrhi::TextureDesc desc;
    nvrhi::StagingTextureHandle staging;
} TextureImpl;

nvrhi::ITexture* getNvTexture(Texture &tex);
nvrhi::ISampler* getNvSampler(Texture &tex);

typedef struct {
    nvrhi::FramebufferHandle framebuffer, framebufferDepthReadOnly;
} FramebufferImpl;

nvrhi::IFramebuffer* getCurrentFramebuffer();
