#include <texture.h>
#include <context.h>
#include <float.h>
#include <framebuffer.h>
#include <mesh.h>
#include <nvrhi/nvrhi.h>
#include <SDL2/SDL_image.h>
#include <shader.h>
#include <vector.h>
#include "private_impl.h"
#include "private_log.h"

static_assert(sizeof(SamplerState) == sizeof(nvrhi::SamplerDesc));

static void convertPictureToRGBA8(SDL_Surface **surface) {
    SDL_PixelFormat format = *((*surface)->format);
    format.BitsPerPixel = 32;
    format.BytesPerPixel = 4;
    format.Rmask = 0x000000ff;
    format.Gmask = 0x0000ff00;
    format.Bmask = 0x00ff0000;
    format.Amask = 0xff000000;
    SDL_Surface *data = SDL_ConvertSurface(*surface, &format, 0);
    if (!data) logError(SDL_GetError());

    SDL_FreeSurface(*surface);
    *surface = data;
}

static SDL_Surface* loadSurface(const char *filename) {
    SDL_Surface *surface = IMG_Load(filename);
    if (!surface) {
        logError("Texture %s not found", filename);
        logError(IMG_GetError());
        return nullptr;
    }

    const uint sdlFormat = surface->format->format;
    if (sdlFormat != SDL_PIXELFORMAT_ABGR8888 && sdlFormat != SDL_PIXELFORMAT_XBGR8888 && sdlFormat != SDL_PIXELFORMAT_RGBA8888 && sdlFormat != SDL_PIXELFORMAT_RGBX8888) {
        logPerfWarning("%s: format convertion from %s to RGBA8", filename, SDL_GetPixelFormatName(sdlFormat));
        convertPictureToRGBA8(&surface);
    }

    return surface;
}

extern "C" {

Texture createTexture1D(const uint size, const Format format, const ulong flags) {
    return createTexture(Texture1D, size, 1, 1, 1, format, flags);
}

Texture createTexture2D(const uint width, const uint height, const Format format, const ulong flags) {
    TextureType type = (flags & MSAA_MASK) ? Texture2DMS : Texture2D;
    return createTexture(type, width, height, 1, 1, format, flags);
}

Texture createTexture3D(const uint width, const uint height, const uint depth, const Format format, const ulong flags) {
    return createTexture(Texture3D, width, height, depth, 1, format, flags);
}

Texture createTexture(const TextureType type, const uint width, const uint height, const uint depth, const uint layers, const Format format, const ulong flags) {
    TextureImpl *tex = new TextureImpl();
    tex->desc = nvrhi::TextureDesc()
        .setDimension((nvrhi::TextureDimension)type)
        .setWidth(width).setHeight(height).setDepth(depth).setArraySize(layers)
        .setFormat((nvrhi::Format)format)
        .setMipLevels((flags & MIPMAPS_FLAG) ? 32 - __builtin_clz(MAX(MAX(width, height), 1)) : 1)
        .setSampleCount(MSAA_SAMPLES(flags)).setSampleQuality((flags & SAMPLES_RELOCATION_FLAG) != 0)
        .setIsUAV(format != SRGBA8_UNORM && format != SBGRA8_UNORM && !isDepthFormat(format) && !(flags & MSAA_MASK))
        .setIsRenderTarget(true)
        .setInitialState(nvrhi::ResourceStates::ShaderResource).setKeepInitialState(true);
    tex->state.borderColor = 0.0f;
    tex->texture = getDevice()->createTexture(tex->desc);
    return Texture{tex};
}

Texture loadTexture(const char *filename, const ulong flags) {
    const bool srgb = (flags & SRGB_FLAG) != 0, snorm = (flags & SNORM_FLAG) != 0;
    SDL_Surface *surface = loadSurface(filename);
    Texture tex = createTexture2D(surface->w, surface->h, srgb ? SRGBA8_UNORM : snorm ? RGBA8_SNORM : RGBA8_UNORM, flags);
    setTextureData(tex, surface->pixels);
    SDL_FreeSurface(surface);
    return tex;
}

void deleteTexture(Texture *tex) {
    if (!tex || !tex->impl)
        return;

    TextureImpl *impl = (TextureImpl*)tex->impl;
    impl->texture.Reset();
    if (impl->staging) impl->staging.Reset();
    delete impl;
    tex->impl = nullptr;
}

void setTextureData(Texture tex, const void *data) {
    setTextureLayerData(tex, 0, data);
    if (getNvTexture(tex)->getDesc().mipLevels > 1) updateMipmaps(tex);
}

void setTextureLayerData(Texture tex, const uint layer, const void *data) {
    nvrhi::ITexture *texture = getNvTexture(tex);
    getCommandList()->writeTexture(texture, layer, 0, data, texture->getDesc().width * getFormatInfo((Format)texture->getDesc().format).size);
}

void clearTexture(Texture tex, Float4 color) {
    clearTextureLayers(tex, color, 0, nvrhi::TextureSubresourceSet::AllArraySlices);
}

void clearTextureLayers(Texture tex, Float4 color, const uint first, const uint count) {
    const nvrhi::TextureSubresourceSet subresource(0, nvrhi::TextureSubresourceSet::AllMipLevels, first, count);
    isDepthFormat((Format)getNvTexture(tex)->getDesc().format) ?
        getCommandList()->clearDepthStencilTexture(getNvTexture(tex), subresource, true, color[0], false, 0) :
        getCommandList()->clearTextureFloat(getNvTexture(tex), subresource, nvrhi::Color(color[0], color[1], color[2], color[3]));
}

void copyTexture(Texture dest, Texture src) {
    nvrhi::TextureSubresourceSet view = nvrhi::TextureSubresourceSet(0, 1, 0, 1);
    getNvTexture(src)->getDesc().sampleCount != getNvTexture(dest)->getDesc().sampleCount ?
        getCommandList()->resolveTexture(getNvTexture(dest), view, getNvTexture(src), view) :
        getCommandList()->copyTexture(getNvTexture(dest), nvrhi::TextureSlice(), getNvTexture(src), nvrhi::TextureSlice());
}

void updateMipmaps(Texture tex) {
    static const uint vertSrc[] = {
        #include "mipmap.vert.spv"
    }, fragSrc[] = {
        #include "mipmap.frag.spv"
    };

    const nvrhi::TextureDesc &desc = getNvTexture(tex)->getDesc();
    SCOPED(Shader) mipmapShader = createGraphicsShader(vertSrc, sizeof(vertSrc), nullptr, 0, nullptr, 0, nullptr, 0, fragSrc, sizeof(fragSrc));
    useShader(mipmapShader);
    uint width = MAX(desc.width >> 1, 1), height = MAX(desc.height >> 1, 1);
    for (uint i = 1; i < desc.mipLevels; i++) {
        useFramebuffer(createFramebufferMip(&tex, 1, NullTexture, i));
        setUniformTextureMip(tex, i - 1, "Mip0");
        setUniform2F(1.0f / float4((float)width, (float)height), "invSize");
        drawSubMesh(NullMesh, 0, 3);

        width = MAX(width >> 1, 1), height = MAX(height >> 1, 1);
    }
}

void *mapTexture(Texture tex) {
    TextureImpl *impl = (TextureImpl*)tex.impl;
    impl->staging = getDevice()->createStagingTexture(impl->desc, nvrhi::CpuAccessMode::Read);
    getCommandList()->copyTexture(impl->staging, nvrhi::TextureSlice(), impl->texture, nvrhi::TextureSlice());
    size_t rowPitch;
    return getDevice()->mapStagingTexture(impl->staging, nvrhi::TextureSlice(), nvrhi::CpuAccessMode::Read, &rowPitch);
}

void unmapTexture(Texture tex) {
    TextureImpl *impl = (TextureImpl*)tex.impl;
    getDevice()->unmapStagingTexture(impl->staging);
    impl->staging.Reset();
}

SamplerState* getSampler(Texture tex) {
    TextureImpl* impl = (TextureImpl*)tex.impl;
    if (!impl)
        return nullptr;

    return (SamplerState*)&impl->state;
}

bool isDepthFormat(const Format format) {return format >= D16 && format <= X32G8_UINT;}

HDRSurface loadSurfacePFM(const char *name) {
    HDRSurface surface = {};

    FILE *texFile = fopen(name, "rb");
    if (!texFile) {
        logError("Can't open picture '%s'", name);
        return surface;
    }

    char buff[3];
    fscanf(texFile, "%2s", buff);
    uint channels = 0;
    if (buff[1] == 'G') {
        channels = 4;
    } else if (buff[1] == 'F') {
        channels = 3;
    } else if (buff[1] == 'f') {
        channels = 1;
    } else {
        logError("Picture '%s' has wrong format", name);
        fclose(texFile);
        return surface;
    }

    float exposition;
    fscanf(texFile, "%u %u\n", &surface.w, &surface.h);
    fscanf(texFile, "%f\n", &exposition);

    surface.data = (Float4*)aligned_malloc(surface.w * surface.h * sizeof(Float4));
    if (channels == 4) {
        fread(surface.data, sizeof(Float4) * surface.w * surface.h, 1, texFile);
    } else for (uint i = 0; i < surface.w * surface.h; i++) {
		fread(&surface.data[i], sizeof(float), channels, texFile);
		surface.data[i] = min4(surface.data[i], float1(FLT_MAX));
		surface.data[i][3] = 1.0f;
    }

    fclose(texFile);

    if (exposition > 0.0f) {
        uchar *buff = (uchar*)surface.data;
        for (uint i = 0; i < surface.w * surface.h * sizeof(Float4); i += 4) {
            SWAP(buff[i    ], buff[i + 3]);
            SWAP(buff[i + 1], buff[i + 2]);
        }
    }

    if (fabsf(exposition) != 1.0f)
        for (uint i = 0; i < surface.w * surface.h; i++)
            surface.data[i] *= fabsf(exposition);

    return surface;
}

void deleteHDRSurface(HDRSurface *surface) {
    if (surface && surface->data) {
        aligned_free(surface->data);
        memset(surface, 0, sizeof(HDRSurface));
    }
}

Texture loadTexturePFM(const char *filename, const ulong flags) {
    SCOPED(HDRSurface) surface = loadSurfacePFM(filename);
    if (!surface.data)
        return Texture{nullptr};

    Texture tex = createTexture2D(surface.w, surface.h, RGBA32_FLOAT, flags);
    setTextureData(tex, surface.data);
    return tex;
}

}

nvrhi::ITexture* getNvTexture(Texture &tex) {
    TextureImpl *impl = (TextureImpl*)tex.impl;
    return impl ? impl->texture : nullptr;
}

nvrhi::ISampler* getNvSampler(Texture &tex) {
    TextureImpl *impl = (TextureImpl*)tex.impl;
    if (!impl) return nullptr;

    if (impl->samplerCache.find(impl->state) == impl->samplerCache.end())
        impl->samplerCache[impl->state] = getDevice()->createSampler(impl->state);

    return impl->samplerCache[impl->state];
}
