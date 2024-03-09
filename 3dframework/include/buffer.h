#pragma once

#include <global_defs.h>

#define NullBuffer (Buffer){0}

DECL_OPAQUE_TYPE(Buffer);

typedef enum : uint {
    ResourceType_Unknown                   = 0,
    ResourceType_Common                    = 0x00000001,
    ResourceType_ConstantBuffer            = 0x00000002,
    ResourceType_VertexBuffer              = 0x00000004,
    ResourceType_IndexBuffer               = 0x00000008,
    ResourceType_IndirectArgument          = 0x00000010,
    ResourceType_ShaderResource            = 0x00000020,
    ResourceType_UnorderedAccess           = 0x00000040,
    ResourceType_RenderTarget              = 0x00000080,
    ResourceType_DepthWrite                = 0x00000100,
    ResourceType_DepthRead                 = 0x00000200,
    ResourceType_StreamOut                 = 0x00000400,
    ResourceType_CopyDest                  = 0x00000800,
    ResourceType_CopySource                = 0x00001000,
    ResourceType_ResolveDest               = 0x00002000,
    ResourceType_ResolveSource             = 0x00004000,
    ResourceType_Present                   = 0x00008000,
    ResourceType_AccelStructRead           = 0x00010000,
    ResourceType_AccelStructWrite          = 0x00020000,
    ResourceType_AccelStructBuildInput     = 0x00040000,
    ResourceType_AccelStructBuildBlas      = 0x00080000,
    ResourceType_ShadingRateSurface        = 0x00100000,
    ResourceType_OpacityMicromapWrite      = 0x00200000,
    ResourceType_OpacityMicromapBuildInput = 0x00400000,
} ResourceType;

#ifdef __cplusplus
extern "C" {
#endif

Buffer createBuffer(const ResourceType type, const uint size) WARN_UNUSED_RESULT;
Buffer createStagingBuffer(const uint size) WARN_UNUSED_RESULT;
void deleteBuffer(Buffer *buffer);

void setBufferData(Buffer buffer, const void *data, const uint size);
void copyBuffer(Buffer dest, Buffer src);
void* mapBuffer(Buffer buffer);
void unmapBuffer(Buffer buffer);

#ifdef __cplusplus
}
#endif
