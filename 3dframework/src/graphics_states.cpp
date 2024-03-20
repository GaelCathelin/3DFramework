#include <graphics_states.h>
#include <context.h>
#include "private_impl.h"

static_assert(sizeof(BlendFactor             ) == sizeof(nvrhi::BlendFactor                     ));
static_assert(sizeof(BlendOp                 ) == sizeof(nvrhi::BlendOp                         ));
static_assert(sizeof(ColorMask               ) == sizeof(nvrhi::ColorMask                       ));
static_assert(sizeof(RenderTargetBlendState  ) == sizeof(nvrhi::BlendState::RenderTarget        ));
static_assert(sizeof(BlendState              ) == sizeof(nvrhi::BlendState                      ));
static_assert(sizeof(FillMode                ) == sizeof(nvrhi::RasterFillMode                  ));
static_assert(sizeof(CullMode                ) == sizeof(nvrhi::RasterCullMode                  ));
static_assert(sizeof(RasterState             ) == sizeof(nvrhi::RasterState                     ));
static_assert(sizeof(StencilOp               ) == sizeof(nvrhi::StencilOp                       ));
static_assert(sizeof(CompareFunc             ) == sizeof(nvrhi::ComparisonFunc                  ));
static_assert(sizeof(StencilOpDesc           ) == sizeof(nvrhi::DepthStencilState::StencilOpDesc));
static_assert(sizeof(DepthStencilState       ) == sizeof(nvrhi::DepthStencilState               ));
static_assert(sizeof(ViewportState           ) == sizeof(nvrhi::Viewport                        ));
static_assert(sizeof(VariableRateShadingState) == sizeof(nvrhi::VariableRateShadingState        ));
static_assert(offsetof(RenderState, blendState       ) == offsetof(nvrhi::RenderState, blendState       ));
static_assert(offsetof(RenderState, depthStencilState) == offsetof(nvrhi::RenderState, depthStencilState));
static_assert(offsetof(RenderState, rasterState      ) == offsetof(nvrhi::RenderState, rasterState      ));
static_assert(sizeof(BlendState) + sizeof(DepthStencilState) + sizeof(RasterState) == sizeof(nvrhi::BlendState) + sizeof(nvrhi::DepthStencilState) + sizeof(nvrhi::RasterState));
static_assert(nvrhi::c_HeaderVersion == 14);

static std::vector<RenderState> states;

extern "C" {

RenderState* getRenderState() {
    if (states.empty()) resetRenderState();
    return &states.back();
}

void pushRenderState() {
    states.push_back(states.back());
}

void popRenderState() {
    if (states.size() <= 1) resetRenderState();
    else states.pop_back();
}

void setDefaultRenderState() {
    static nvrhi::RenderState defaultState = nvrhi::RenderState();
    static nvrhi::VariableRateShadingState defaultVRS = nvrhi::VariableRateShadingState();
    if (states.empty()) states.push_back(RenderState{});
    memcpy(&states.back(), &defaultState, sizeof(nvrhi::BlendState) + sizeof(nvrhi::DepthStencilState) + sizeof(nvrhi::RasterState));
    memset(&states.back().viewportState, 0, sizeof(ViewportState));
    memset(&states.back().scissorState, 0, sizeof(ViewportState));
    memcpy(&states.back().vrsState, &defaultVRS, sizeof(nvrhi::VariableRateShadingState));
    states.back().viewportState.maxX = states.back().scissorState.maxX = getWidth();
    states.back().viewportState.maxY = states.back().scissorState.maxY = getHeight();
    states.back().viewportState.maxZ = 1.0f;
    getRenderState()->depthStencilState.depthFunc = CompareFunc_GreaterOrEqual;
    getRenderState()->rasterState.frontCounterClockwise = true;
    getRenderState()->rasterState.cullMode = CullMode_None;
}

void resetRenderState() {
    states.clear();
    setDefaultRenderState();
}

void setBlendFactors(const uint target, const BlendFactor srcBlend, const BlendFactor srcBlendAlpha, const BlendFactor destBlend, const BlendFactor destBlendAlpha) {
    if (target >= 8) return;
    states.back().blendState.targets[target].srcBlend = srcBlend;
    states.back().blendState.targets[target].srcBlendAlpha = srcBlendAlpha;
    states.back().blendState.targets[target].destBlend = destBlend;
    states.back().blendState.targets[target].destBlendAlpha = destBlendAlpha;
}

void setSampleLocation(const uint sample, const float x, const float y) {
    if (sample >= 16) return;
    states.back().rasterState.sampleLocationsX[sample] = float2int(roundf(x * 16.0f));
    states.back().rasterState.sampleLocationsY[sample] = float2int(roundf(y * 16.0f));
}

}
