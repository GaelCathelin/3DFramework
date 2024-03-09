#pragma once

#include <texture.h>

DECL_OPAQUE_TYPE(Framebuffer);

#ifdef __cplusplus
extern "C" {
#endif

Framebuffer createEmptyFramebuffer(const uint width, const uint height) WARN_UNUSED_RESULT;
Framebuffer createFramebuffer(Texture *colors, const uint nbColors, Texture depth) WARN_UNUSED_RESULT;
Framebuffer createFramebufferMip(Texture *colors, const uint nbColors, Texture depth, const uint mip) WARN_UNUSED_RESULT;
void deleteFramebuffer(Framebuffer *fb);

void useFramebuffer(Framebuffer fb);

#ifdef __cplusplus
}
#endif
