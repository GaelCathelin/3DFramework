#include <framebuffer.h>
#include <graphics_states.h>
#include "private_impl.h"

static FramebufferImpl *current = nullptr;

extern "C" {

Framebuffer createFramebuffer(Texture *colors, const uint nbColors, Texture depth) {
    return createFramebufferMip(colors, nbColors, depth, 0);
}

Framebuffer createFramebufferMip(Texture *colors, const uint nbColors, Texture depth, const uint mip) {
    nvrhi::FramebufferDesc framebufferDesc = nvrhi::FramebufferDesc();
    for (uint i = 0; i < nbColors; i++) {
        nvrhi::FramebufferAttachment attachment = nvrhi::FramebufferAttachment()
            .setTexture(getNvTexture(colors[i]))
            .setMipLevel(mip).setArraySliceRange(0, getNvTexture(colors[i])->getDesc().arraySize);
        framebufferDesc.addColorAttachment(attachment);
    }
    if (getNvTexture(depth)) {
        nvrhi::FramebufferAttachment attachment = nvrhi::FramebufferAttachment()
            .setTexture(getNvTexture(depth))
            .setMipLevel(mip).setArraySliceRange(0, getNvTexture(depth)->getDesc().arraySize);
        framebufferDesc.setDepthAttachment(attachment);
    }

    FramebufferImpl *fb = new FramebufferImpl();
    fb->framebuffer = getDevice()->createFramebuffer(framebufferDesc);
    fb->framebufferDepthReadOnly = nullptr;
    if (getNvTexture(depth)) {
        framebufferDesc.depthAttachment.setReadOnly(true);
        fb->framebufferDepthReadOnly = getDevice()->createFramebuffer(framebufferDesc);
    }
    return Framebuffer{fb};
}

Framebuffer createEmptyFramebuffer(const uint width, const uint height) {
    FramebufferImpl *fb = new FramebufferImpl();
    fb->framebuffer = getDevice()->createFramebuffer(nvrhi::FramebufferDesc().setDefaultSize(width, height));
    return Framebuffer{fb};
}

void deleteFramebuffer(Framebuffer *fb) {
    if (!fb || !fb->impl)
        return;

    if (current == fb->impl) current = nullptr;

    FramebufferImpl* impl = (FramebufferImpl*)fb->impl;
    impl->framebufferDepthReadOnly.Reset();
    impl->framebuffer.Reset();
    delete impl;
    fb->impl = nullptr;
}

void useFramebuffer(Framebuffer fb) {
    current = (FramebufferImpl*)fb.impl;
    getRenderState()->viewportState.maxX = current->framebuffer->getFramebufferInfo().width;
    getRenderState()->viewportState.maxY = current->framebuffer->getFramebufferInfo().height;
    getRenderState()->scissorState.maxX = current->framebuffer->getFramebufferInfo().width;
    getRenderState()->scissorState.maxY = current->framebuffer->getFramebufferInfo().height;
}

}

nvrhi::IFramebuffer* getCurrentFramebuffer() {
    if (!current)
        return nullptr;

    if (current->framebufferDepthReadOnly && !getRenderState()->depthStencilState.depthWriteEnable)
        return current->framebufferDepthReadOnly;

    return current->framebuffer;
}
