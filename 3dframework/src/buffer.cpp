#include <buffer.h>
#include <nvrhi/nvrhi.h>
#include "private_log.h"
#include "private_impl.h"

extern "C" {

Buffer createBuffer(const ResourceType type, const uint size) {
    BufferImpl *buffer = new BufferImpl();
    buffer->buffer = getDevice()->createBuffer(getBufferDesc(type, size));
	buffer->mapped = false;
    return Buffer{buffer};
}

Buffer createStagingBuffer(const uint size) {
    BufferImpl *buffer = new BufferImpl();
    nvrhi::BufferDesc desc = nvrhi::BufferDesc()
        .setByteSize(size)
        .setCpuAccess(nvrhi::CpuAccessMode::Read)
        .setKeepInitialState(true);
    buffer->buffer = getDevice()->createBuffer(desc);
	buffer->mapped = false;
    return Buffer{buffer};
}

void deleteBuffer(Buffer *buffer) {
    if (!buffer || !buffer->impl)
        return;

    BufferImpl *impl = (BufferImpl*)buffer->impl;
	if (impl->mapped) unmapBuffer(*buffer);
    impl->buffer.Reset();
    delete impl;
    buffer->impl = nullptr;
}

void setBufferData(Buffer buffer, const void *data, const uint size) {
    nvrhi::IBuffer *buf = getNvBuffer(buffer);
    getCommandList()->writeBuffer(buf, data, size);
}

void copyBuffer(Buffer dest, Buffer src) {
    getCommandList()->copyBuffer(getNvBuffer(dest), 0, getNvBuffer(src), 0, getNvBuffer(dest)->getDesc().byteSize);
}

void* mapBuffer(Buffer buffer) {
    return getDevice()->mapBuffer(getNvBuffer(buffer), nvrhi::CpuAccessMode::Read);
}

void unmapBuffer(Buffer buffer) {
    return getDevice()->unmapBuffer(getNvBuffer(buffer));
}

}

nvrhi::BufferDesc getBufferDesc(ResourceType type, const uint size) {
    nvrhi::BufferDesc desc = nvrhi::BufferDesc()
        .setByteSize(size).setKeepInitialState(true)
        .setCanHaveUAVs(true).setCanHaveRawViews(true).setIsAccelStructBuildInput(true);

    switch (type) {
        case ResourceType_ConstantBuffer  : desc.setIsConstantBuffer(true)  ; break;
        case ResourceType_VertexBuffer    : desc.setIsVertexBuffer(true)    ; break;
        case ResourceType_IndexBuffer     : desc.setIsIndexBuffer(true)     ; break;
        case ResourceType_IndirectArgument: desc.setIsDrawIndirectArgs(true); break;
        case ResourceType_UnorderedAccess :                                 ; break;
        default: assert(false);
    }

    return desc.setInitialState((nvrhi::ResourceStates)type);
}

nvrhi::IBuffer* getNvBuffer(Buffer &buffer) {
    BufferImpl *impl = (BufferImpl*)buffer.impl;
    return impl ? impl->buffer : nullptr;
}
